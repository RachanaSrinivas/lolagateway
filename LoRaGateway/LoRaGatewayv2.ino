#include "config.h"
#include "MessageQueue.h"
#include "Conversion.h"

volatile bool ConnectedToWifi = false;

String WifiIpAddress;
unsigned long WifiConnectionSlot;
volatile bool IsLoraInReceivingMode = false;

volatile bool ReadReceivedData = false;
volatile bool ToPublish = false;
String AckDataToWeb;

WiFiClient EspClient;
PubSubClient MqttClient(EspClient);

MessageQueue Mq;
Conversion Conv;

void loRaSetLedStatus(bool state) {
  if (state) {
    digitalWrite(LORA_STATUS_FAIL, LOW);
    digitalWrite(MQTT_STATUS, LOW);
    digitalWrite(LORA_STATUS_OK, HIGH);
  } else {
    digitalWrite(LORA_STATUS_OK, LOW);
    digitalWrite(MQTT_STATUS, LOW);
    digitalWrite(LORA_STATUS_FAIL, HIGH);
  }
}

void mqttSetLedStatus(bool state) {
  if (state) {
    digitalWrite(LORA_STATUS_OK, LOW);
    digitalWrite(LORA_STATUS_FAIL, LOW);
    digitalWrite(MQTT_STATUS, HIGH);
  } else {
    loRaSetLedStatus(true);
  }
}

void initWiFi() {
  if (WifiPassword == "")
    WiFi.begin(WifiSsid.c_str());
  else
    WiFi.begin(WifiSsid.c_str(), WifiPassword.c_str());
}

void initMqtt() {
  MqttClient.setServer(MqttServer, MQTT_PORT);
  MqttClient.setCallback(subscribeCallback);

  if (MqttClient.connect(MQTT_CLIENT_ID)) {
    mqttSetLedStatus(true);

    Serial.println("Connected to MQTT server");
    MqttClient.subscribe(MQTT_SUB_TOPIC);
  }
  else {
    Serial.printf("Failed to connect to MQTT Server - %d\n", MqttClient.state());
  }
}

void reconnectToMqtt() {
  if (MqttClient.connect(MQTT_CLIENT_ID)) {
    mqttSetLedStatus(true);

    Serial.println("Reconnected to MQTT");

    MqttClient.unsubscribe(MQTT_SUB_TOPIC);
    MqttClient.subscribe(MQTT_SUB_TOPIC);
  }
  else {
    Serial.printf("Failed to connect to MQTT Server - %d\n", MqttClient.state());
    delay(1000);
  }
}

void tryConnectWifi(int numOfAttempts = 20) {
  int numberOfAttemptsCompleted = 0;
  WifiIpAddress = "X";

  while (numberOfAttemptsCompleted < numOfAttempts) {
    Serial.print(".");
    if (WiFi.status() == WL_CONNECTED) {
      ConnectedToWifi = true;
      WifiIpAddress = WiFi.localIP().toString();

      Serial.printf("Wifi connected - %s\n", WifiSsid.c_str());
      break;
    }

    numberOfAttemptsCompleted++;
    delay(500);
  }
}

void checkAndConnectToWifi() {
  Serial.println("Checking Wifi Connection");

  switch (WiFi.status()) {
    case WL_CONNECTED:
      if (!ConnectedToWifi) {
        WifiIpAddress = WiFi.localIP().toString();
        ConnectedToWifi = true;
      }
      break;
    case WL_IDLE_STATUS:
    case WL_NO_SSID_AVAIL:
      break;
    case WL_SCAN_COMPLETED:
    case WL_CONNECT_FAILED:
    case WL_CONNECTION_LOST:
    case WL_DISCONNECTED:
      ConnectedToWifi = false;
      WiFi.disconnect();
      initWiFi();
      tryConnectWifi(RECONNECTION_ATTEMPTS);
      break;
    default:
      break;
  }
}

void publishToWeb(Message message) {
  String dataToWeb = Conv.toResponseJson(message);

  switch (message.Content[0]) {
    case 'a':
    case 'h':
    case 'e':
      MqttClient.publish(MQTT_PUB_TOPIC, dataToWeb.c_str());
      break;
    case 'm':
      if (MqttClient.publish(MQTT_PUB_TOPIC, dataToWeb.c_str())) {
        String responseMsg = Conv.getAckMessage(message);
        LoRa_sendMessage(responseMsg);
      } else {
        String responseMsg = Conv.getErrorMessage(message);
        LoRa_sendMessage(responseMsg);
      }
      break;
  }
}

void LoRa_rxMode() {
  IsLoraInReceivingMode = true;

  LoRa.disableInvertIQ();               // normal mode
  LoRa.receive();                       // set receive mode

  Serial.println("################# RX Mode #################");
}

void LoRa_txMode() {
  IsLoraInReceivingMode = false;

  LoRa.idle();                          // set standby mode
  LoRa.enableInvertIQ();                // active invert I and Q signals

  Serial.println("################# TX Mode #################");
}

void LoRa_sendMessage(String message) {
  LoRa_txMode();

  LoRa.beginPacket();                   // start packet
  LoRa.print(message);                  // add payload
  LoRa.endPacket(true);                 // finish packet and send it
}

Message readData() {
  String rxMessage = "";

  while (LoRa.available()) {
    rxMessage += (char)LoRa.read();
  }

  Serial.print("Feedback from Node: ");
  Serial.println(rxMessage);      // NodeTo;NodeFrom;MsgId;Content 0;1;1;a:ACK

  Message rxData = Conv.toObject(rxMessage);

  if (rxData.NodeTo == GATEWAY_ADDR) {
    switch (rxData.Content[0]) {
      case 'a':
        int index;
        index = Mq.isIdPresent(rxData.Id);

        if (index == -1) {
          return {};
        }

        Mq.ackMessage(index);
        Serial.println("Ack Received");
        return rxData;
        break;
      case 'h':
        Serial.println("Node Alive signal Received");
        return rxData;
        break;
      case 'e':
        Serial.println("Err Received");
        return rxData;
        break;
      case 'm':
        Serial.println("Sensor Message Received");
        return rxData;
        break;
      default:
        break;
    }
  }
}

void transmitAllMessages(int totalMsgs) {
  vector<Message> messages = Mq.getMessages();

  for (int i = 0; i < totalMsgs; i++) {
    Serial.print("Message In Queue : ");
    String msgToTransmit = Conv.toStrWithDelim(messages[i]);
    Serial.println(msgToTransmit);

    LoRa_sendMessage(msgToTransmit);
    delay(500);
  }
}

String getHeartBeatMessage() {
  Message msg;
  msg.NodeFrom = 0;
  msg.Id = -200;
  msg.Content = "h:200";

  String heartBeatMsg = Conv.toResponseJson(msg);
  return heartBeatMsg;
}

// #################### CALLBACKS ####################

void subscribeCallback(char* topic, byte* payload, unsigned int length) {
  char rxMastStateData[length + 1];

  Serial.print("Message:");
  for (int i = 0; i < length; i++) {
    rxMastStateData[i] = payload[i];
  }

  // {"network_add":255,"node_add":1,"msg_id":5,"msg_content":"m:ON"}
  StaticJsonDocument<200> rxDataJson;

  Serial.print("Parsing Incoming Message: ");
  Serial.println(String(rxMastStateData));
  DeserializationError error = deserializeJson(rxDataJson, payload);

  if (error) {
    Serial.print("--------------- deserializeJson() failed: ");
    Serial.println(error.c_str());
  }

  Message rxData;
  rxData.Network = rxDataJson["network_add"];

  if (rxData.Network == NETWORK_ID) {
    rxData.NodeFrom = 0;
    rxData.NodeTo = rxDataJson["node_add"];
    rxData.Id = rxDataJson["msg_id"];
    rxData.Content = (const char*)rxDataJson["msg_content"];

    Serial.printf("Parsed String in Callback- %d, %d, %d, %d, %s\n", rxData.Network, rxData.NodeFrom, rxData.NodeTo, rxData.Id, rxData.Content);

    Mq.addMessage(rxData);
  }
}

void onReceive(int packetSize) {
  ReadReceivedData = true;
}

void onTxDone() {
  Serial.println("TxDone");
  LoRa_rxMode();
}

void heartBeatSignal() {
  String heartBeatMsg = getHeartBeatMessage();
  
  MqttClient.publish(MQTT_PUB_TOPIC, heartBeatMsg.c_str());
  Serial.println(heartBeatMsg);
}

void setup() {
  Serial.begin(115200);
  Serial.println(VERSION);

  pinMode(LORA_STATUS_OK, OUTPUT);
  pinMode(LORA_STATUS_FAIL, OUTPUT);
  pinMode(MQTT_STATUS, OUTPUT);

  digitalWrite(LORA_STATUS_OK, LOW);
  digitalWrite(LORA_STATUS_FAIL, LOW);
  digitalWrite(MQTT_STATUS, LOW);

  LoRa.setPins(CS_PIN, RESET_PIN, IRQ_PIN);
  if (!LoRa.begin(FREQUENCY)) {
    Serial.println("LoRa init failed. Check your connections.");
    loRaSetLedStatus(false);
    while (true);
  }

  LoRa.setSyncWord(SYNC_WORD);
  loRaSetLedStatus(true);
  Serial.println("LoRa Gateway Initialized successfully");

  LoRa.onReceive(onReceive);
  LoRa.onTxDone(onTxDone);

  initWiFi();
  tryConnectWifi(RECONNECTION_ATTEMPTS);
  initMqtt();

  HeartBeatSignal.attach(HEART_BEAT_INTERVAL, heartBeatSignal);

  delay(1000);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED)
    checkAndConnectToWifi();

  if (ConnectedToWifi && !MqttClient.connected()) {
    mqttSetLedStatus(false);
    reconnectToMqtt();
  }

  if (!IsLoraInReceivingMode) {
    int totalMsgs = Mq.getCount();

    if (totalMsgs > 0) {
      Serial.printf("Msg Count in Queue: %d\n", totalMsgs);
      transmitAllMessages(totalMsgs);
    }
    else {
      Serial.printf("No msg in Queue: %d\n", totalMsgs);
    }
  }

  if (ReadReceivedData) {
    Message rxData = readData();

    if (rxData.Content.length() > 0) {
      publishToWeb(rxData);
    }

    ReadReceivedData = false;
    IsLoraInReceivingMode = false;
  }

  MqttClient.loop();
  delay(1000);
}
