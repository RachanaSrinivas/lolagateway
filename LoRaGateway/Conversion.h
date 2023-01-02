#include "MessageQueue.h"
#include "config.h"

class Conversion {
  public:
    String toStrWithDelim(Message message) {
      char msgString[25];

      sprintf(msgString, "%d;%d;%d;%s", message.NodeFrom, message.NodeTo, message.Id,  message.Content);
      return String(msgString);
    }

    Message toObject(String msg) {
      String msgs[4];
      int index = 0, prevIdx = 0;

      for (int i = 0; i < msg.length() + 1; i++) {
        if (msg[i] == ';' || msg[i] == '\0') {
          msgs[index] = msg.substring(prevIdx, i);
          index++;
          prevIdx = i + 1;
        }
        if (index == 4) break;
      }

      Message message;
      message.NodeFrom = atoi(String(msgs[0]).c_str());
      message.NodeTo = atoi(String(msgs[1]).c_str());
      message.Id = atoi(String(msgs[2]).c_str());
      message.Content = msgs[3];

      return message;
    }

    String toResponseJson(Message message) {
      StaticJsonDocument<150> msgJson;

      msgJson["network_add"] = NETWORK_ID;
      msgJson["node_add"] = message.NodeFrom;
      msgJson["msg_id"] = message.Id;
      msgJson["msg_content"] = String(message.Content);

      char parsedMessage[150];
      serializeJson(msgJson, parsedMessage);

      return String(parsedMessage);
    }

    String getAckMessage(Message rxData) {
      return String(rxData.NodeTo) + ";" + String(rxData.NodeFrom) + ";" + String(rxData.Id) + ";a:ACK";
    }

    String getErrorMessage(Message rxData) {
      return String(rxData.NodeTo) + ";" + String(rxData.NodeFrom) + ";" + String(rxData.Id) + ";e:INVALID COMMAND";
    }
};
