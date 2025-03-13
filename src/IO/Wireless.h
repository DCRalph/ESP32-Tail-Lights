#pragma once

#include "config.h"
#include <functional>
#include <map>

struct __attribute__((packed)) data_packet {
  uint8_t type;
  uint8_t len;
  uint8_t data[200];
};

enum class PacketDirection {
  SEND,
  RECV
};

struct fullPacket {
  uint8_t mac[6];
  PacketDirection direction;
  data_packet p;
};

class Wireless {
private:
  bool setupDone = false;

  // Generic callback for packets that don’t have a type-specific callback.
  std::function<void(fullPacket *fp)> onReceiveOtherCb;
  // Map for type-specific callbacks: key = packet type, value = callback function.
  std::map<uint8_t, std::function<void(fullPacket *fp)>> onReceiveForCallbacks;

public:
  esp_now_send_status_t lastStatus = ESP_NOW_SEND_FAIL;

  Wireless();
  void setup();
  void unSetup();
  void loop();

  bool isSetupDone();

  void sendCallback(const uint8_t *mac_addr,
                    esp_now_send_status_t status);
  void recvCallback(const uint8_t *mac_addr, const uint8_t *data, int len);

  // Set the generic "other" callback.
  void setOnReceiveOther(std::function<void(fullPacket *fp)> cb);
  // Register a type-specific callback.
  void addOnReceiveFor(uint8_t type,
                       std::function<void(fullPacket *fp)> cb);
  // Remove a type-specific callback.
  void removeOnReceiveFor(uint8_t type);

  int send(data_packet *p, u8_t *peer_addr);
  int send(u8_t *data, size_t len, u8_t *peer_addr);

  int send(fullPacket *fp);
};

extern Wireless wireless;
