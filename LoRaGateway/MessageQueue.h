#ifndef MESSAGEQUEUE_H
#define MESSAGEQUEUE_H

#include <Arduino.h>
#include <bits/stdc++.h>
#include <vector>

#define SIZE 20

using namespace std;

struct Message {
  int Network;      // GSM = 0x00, LoRa = 0x01 to 0xFF
  int NodeFrom;
  int NodeTo;       // Node Id shd be same at both ends. 0 -> broadcast
  int Id;           // Message Id
  String Content;   // COntains the state
};

struct MessageQueue {
  vector<Message> Queue;
  vector<int> ProcessedMsgs;
  
  int ProcessedCount = 0;
  int Count = 0;
  
  void addMessage(Message message) {
    Queue.push_back(message);
    Count = Queue.size();
  }

  void ackMessage(int index=0) {
    if (Count == 0)
      return;

    Queue.erase(Queue.begin() + index);
    Count = Queue.size();
  }

  vector<Message> getMessages() {
    if (Count == 0)
      return {};
    
    return Queue;
  }

  int getCount() {
    return Count;
  }

  int isIdPresent(int messageId) {
    for (int index = 0; index < Count; index++) {
      if(messageId == Queue[index].Id) {
        return index;
      }
    }
    return -1;
  }

  void addProcessedMsgId(int msgId) {
    ProcessedMsgs.push_back(msgId);
    ProcessedCount = ProcessedMsgs.size();
  }

  vector<int> getProcessedIds() {
    if (Count == 0)
      return {};
    
    return ProcessedMsgs;
  }

  void clearProcessedMsgs() {
    ProcessedMsgs.clear();
  }

  int getProcessedCount() {
    return ProcessedCount;
  }
};

#endif
