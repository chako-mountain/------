
#include "esp_now.h"
#include "WiFi.h"
#include "esp_mac.h" // For the MAC2STR and MACSTR macros
#include "vector"
#include "SD.h" // SDカードライブラリのインクルード
#include "xtensa/xtensa_api.h"

/* Definitions */
#define ESPNOW_WIFI_CHANNEL 6

/* Classes */
class ESP_NOW_Slave
{
public:
  esp_now_peer_info_t peerInfo;

  ESP_NOW_Slave(const uint8_t *mac_addr, uint8_t channel, wifi_interface_t iface, const uint8_t *lmk)
  {
    memcpy(peerInfo.peer_addr, mac_addr, 6);
    peerInfo.channel = channel;
    peerInfo.ifidx = iface;
    peerInfo.encrypt = false;
    if (lmk != nullptr)
    {
      memcpy(peerInfo.lmk, lmk, 16);
      peerInfo.encrypt = true;
    }
  }
  ~ESP_NOW_Slave() {}

  static void onReceive(const esp_now_recv_info_t *info, const uint8_t *data, int len)
  {
    Serial.printf("Received message from " MACSTR ", data length : %u\n", MAC2STR(info->src_addr), len);
    Serial.printf("Message: %s\n", (char *)data);
    save_to_sd((char *)data);
  }

  static void save_to_sd(const char *data)
  {
    if (!SD.begin())
    {
      Serial.println("SD card initialization failed!");
      return;
    }
    Serial.println("SD card initialized.");

    File file = SD.open("/received_data.txt", FILE_APPEND);
    if (file)
    {
      file.println(data);
      file.close();
      Serial.println("Data saved to SD card");
    }
    else
    {
      Serial.println("Failed to open file for writing");
    }
  }
};

/* Global Variables */
std::vector<ESP_NOW_Slave> master_peers;

/* Function to add a new master peer */
void addMasterPeer(const uint8_t *mac_addr)
{
  ESP_NOW_Slave new_peer(mac_addr, ESPNOW_WIFI_CHANNEL, WIFI_IF_STA, NULL);
  master_peers.push_back(new_peer);
  if (esp_now_add_peer(&new_peer.peerInfo) != ESP_OK)
  {
    Serial.println("Failed to register the master peer");
  }
}

/* Main */
void setup()
{
  Serial.begin(115200);
  while (!Serial)
    delay(10);

  WiFi.mode(WIFI_STA);
  WiFi.setChannel(ESPNOW_WIFI_CHANNEL);
  while (!WiFi.STA.started())
    delay(100);

  Serial.println("ESP-NOW Example - Unicast Slave");
  Serial.println("Wi-Fi parameters:");
  Serial.println("  Mode: STA");
  Serial.println("  MAC Address: " + WiFi.macAddress());
  Serial.printf("  Channel: %d\n", ESPNOW_WIFI_CHANNEL);

  if (esp_now_init() != ESP_OK)
  {
    Serial.println("Failed to initialize ESP-NOW");
    Serial.println("Rebooting in 5 seconds...");
    delay(5000);
    ESP.restart();
  }

  // Add master peers
  const uint8_t master1_mac[] = {0x1C, 0x69, 0x20, 0x89, 0xD9, 0x08}; // Replace with your master's MAC address
  const uint8_t master2_mac[] = {0x1C, 0x69, 0x20, 0x89, 0xE7, 0x28}; // Replace with another master's MAC address
  const uint8_t master3_mac[] = {0x08, 0x3A, 0xF2, 0x73, 0x08, 0x94};
  addMasterPeer(master1_mac);
  addMasterPeer(master2_mac);
  addMasterPeer(master3_mac);

  esp_now_register_recv_cb(ESP_NOW_Slave::onReceive);

  Serial.println("Setup complete. Waiting for messages...");
}

void loop()
{
  delay(100);
  // ただいま
}
