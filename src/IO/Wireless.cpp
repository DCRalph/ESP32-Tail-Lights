#include "Wireless.h"
#include "esp_log.h"
#include "esp_now.h"
#include "WiFi.h"

static const char *TAG = "Wireless";

Wireless::Wireless() {
  // Constructor (if any further initialization is needed, add it here)
}

void Wireless::setup() {
#ifndef ESPNOW_NO_DISABLE_WIFI
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
#endif

#ifndef ESPNOW_NO_DISABLE_WIFI
  // Set WiFi channel using promiscuous mode (temporarily enabled)
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(ESP_NOW_CHANNEL, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);
#endif

  if (esp_now_init() != ESP_OK) {
    ESP_LOGI(TAG, "Error initializing ESP-NOW");
    return;
  }

  // Note: Using the global 'wireless' instance for callbacks.
  esp_now_register_send_cb([](const uint8_t* mac_addr,
                               esp_now_send_status_t status) {
    wireless.sendCallback(mac_addr, status);
  });

  esp_now_register_recv_cb([](const uint8_t* mac_addr,
                               const uint8_t* data,
                               int len) {
    wireless.recvCallback(mac_addr, data, len);
  });

  setupDone = true;
}

void Wireless::unSetup() {
  setupDone = false;
  esp_now_deinit();
  esp_now_unregister_recv_cb();
  esp_now_unregister_send_cb();
}

void Wireless::loop() {
  if (!setupDone)
    return;
}

bool Wireless::isSetupDone() {
  return setupDone;
}

void Wireless::sendCallback(const uint8_t* mac_addr,
                            esp_now_send_status_t status) {
  char macStr[18];
  snprintf(macStr, sizeof(macStr),
           "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2],
           mac_addr[3], mac_addr[4], mac_addr[5]);

#ifdef DEBUG_ESP_NOW
  ESP_LOGI(TAG, "########### Sent Packet ###########");
  ESP_LOGI(TAG, "Sent to: %s", macStr);
  ESP_LOGI(TAG, "Send Status: %s",
           status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
  ESP_LOGI(TAG, "###################################");
#endif

  lastStatus = status;
}

void Wireless::recvCallback(const uint8_t* mac_addr, const uint8_t* data,
                            int len) {
  char macStr[18];
  snprintf(macStr, sizeof(macStr),
           "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2],
           mac_addr[3], mac_addr[4], mac_addr[5]);

  data_packet* p = (data_packet*)data;

#ifdef DEBUG_ESP_NOW
  ESP_LOGI(TAG, "########### Received Packet ###########");
  ESP_LOGI(TAG, "Recv from: %s", macStr);
  ESP_LOGI(TAG, "Type: %d", p->type);
  ESP_LOGI(TAG, "Len: %d", p->len);

  // Build the data string using pointer arithmetic for speed.
  char dataStr[256];
  int pos = 0;
  for (int i = 0; i < p->len && pos < (int)sizeof(dataStr); i++) {
    pos += snprintf(dataStr + pos, sizeof(dataStr) - pos, "%02X ", p->data[i]);
  }
  ESP_LOGI(TAG, "Data: %s", dataStr);
  ESP_LOGI(TAG, "#######################################");
#endif

  fullPacket fp;
  memcpy(fp.mac, mac_addr, ESP_NOW_ETH_ALEN);
  fp.direction = PacketDirection::RECV;
  memcpy(&fp.p, p, sizeof(data_packet));

  // Call a type-specific callback if available, otherwise the generic one
  auto it = onReceiveForCallbacks.find(fp.p.type);
  if (it != onReceiveForCallbacks.end()) {
    it->second(&fp);
  } else if (onReceiveOtherCb) {
    onReceiveOtherCb(&fp);
  }
}

void Wireless::setOnReceiveOther(std::function<void(fullPacket* fp)> cb) {
  onReceiveOtherCb = cb;
}

void Wireless::addOnReceiveFor(uint8_t type,
                               std::function<void(fullPacket* fp)> cb) {
  onReceiveForCallbacks[type] = cb;
}

void Wireless::removeOnReceiveFor(uint8_t type) {
  onReceiveForCallbacks.erase(type);
}

int Wireless::send(data_packet* p, uint8_t* peer_addr) {
  return send((uint8_t*)p, sizeof(data_packet), peer_addr);
}

int Wireless::send(uint8_t* data, size_t len, uint8_t* peer_addr) {
  if (!setupDone) {
    ESP_LOGI(TAG, "ESP-NOW not initialized");
    return -1;
  }

  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, peer_addr, ESP_NOW_ETH_ALEN);
  peerInfo.channel = ESP_NOW_CHANNEL;
  peerInfo.encrypt = false;
  // Explicitly select the WiFi interface (typically WIFI_IF_STA)
  peerInfo.ifidx = WIFI_IF_STA;

#ifdef DEBUG_ESP_NOW
  ESP_LOGI(TAG, "######################");
  char peerStr[32];
  int pos = 0;
  for (int i = 0; i < ESP_NOW_ETH_ALEN; i++) {
    pos += snprintf(peerStr + pos, sizeof(peerStr) - pos, "%02X ", 
                    peerInfo.peer_addr[i]);
  }
  ESP_LOGI(TAG, "Peer info: %s", peerStr);
  ESP_LOGI(TAG, "Channel: %d", peerInfo.channel);
  ESP_LOGI(TAG, "Encrypt: %d", peerInfo.encrypt);
  ESP_LOGI(TAG, "######################");
#endif

  esp_err_t err = esp_now_add_peer(&peerInfo);
  if (err != ESP_OK) {
    ESP_LOGI(TAG, "Failed to add peer, error: %d", err);
    return -1;
  }
#ifdef DEBUG_ESP_NOW
  ESP_LOGI(TAG, "Peer added");
#endif

  err = esp_now_send(peerInfo.peer_addr, data, len);
  if (err != ESP_OK) {
    ESP_LOGI(TAG, "Failed to send data, error: %d", err);
    // Attempt to clean up even if sending fails.
    esp_now_del_peer(peer_addr);
    return -1;
  }
#ifdef DEBUG_ESP_NOW
  ESP_LOGI(TAG, "Data sent");
#endif

  err = esp_now_del_peer(peer_addr);
  if (err != ESP_OK) {
    ESP_LOGI(TAG, "Failed to delete peer, error: %d", err);
    return -1;
  }
#ifdef DEBUG_ESP_NOW
  ESP_LOGI(TAG, "Peer deleted");
  ESP_LOGI(TAG, "######################");
#endif

  return 0;
}

int Wireless::send(fullPacket* fp) {
  if (fp->direction == PacketDirection::SEND) {
    return send(&fp->p, fp->mac);
  } else {
    ESP_LOGI(TAG, "Cannot send a receive packet");
    return -1;
  }
}

Wireless wireless;