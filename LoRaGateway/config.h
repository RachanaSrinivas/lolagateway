#ifndef CONFIG_H
#define CONFIG_H

#define VERSION "2.0"

#define MQTT_CLIENT_ID "G13"
#define NETWORK_ID 255
#define SYNC_WORD 0xFF
#define GATEWAY_ADDR 0
#define DELIMITER ";"

#include <WiFi.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <LoRa.h>
#include <ArduinoJson.h>
#include <Ticker.h>

#define MQTT_SERVER "173.248.136.206"
#define MQTT_PORT 1883
#define RECONNECTION_ATTEMPTS 10
#define WIFI_CONNECTION_CHECK_INTERVAL 15
#define HEART_BEAT_INTERVAL 31

#define MQTT_SUB_TOPIC            "TO_EDGE"
#define MQTT_PUB_TOPIC            "TO_SERVER"

#define LORA_STATUS_OK 25       // Blue
#define LORA_STATUS_FAIL 26     // Red
#define MQTT_STATUS 27          // Green

#define CS_PIN 5 //10          // LoRa radio chip select
#define RESET_PIN 14 //9        // LoRa radio reset
#define IRQ_PIN 2          // change for your board; must be a hardware interrupt pin

#define FREQUENCY 868E6

String WifiSsid = "Axis-DIGITAL";
String WifiPassword = "@2Crypt#Test";

const char* MqttServer = "173.248.136.206";

Ticker HeartBeatSignal;

#endif
