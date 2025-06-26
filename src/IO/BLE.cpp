#include "BLE.h"
#include "../Application.h"
#include "LED/LEDStripManager.h"
#include "TimeProfiler.h"
#include <esp_system.h>
#include "Battery.h"

// Initialize static instance
BLEManager *BLEManager::instance = nullptr;

BLEManager *BLEManager::getInstance()
{
  if (!instance)
  {
    instance = new BLEManager();
  }
  return instance;
}

BLEManager::BLEManager()
    : pServer(nullptr), pService(nullptr), app(nullptr), deviceConnected(false), connectionCount(0), lastPingUpdate(0)
{
  // Initialize characteristic pointers to nullptr
  pPingCharacteristic = nullptr;
  pModeCharacteristic = nullptr;
  pEffectsCharacteristic = nullptr;
}

BLEManager::~BLEManager()
{
  end();
}

void BLEManager::begin()
{
  Serial.println("BLEManager: Initializing BLE...");

  // Initialize BLE Device
  String deviceName = "ESP32-TailLights-" + String((uint32_t)ESP.getEfuseMac(), HEX);
  BLEDevice::init(deviceName.c_str());

  // Create BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new CarThingBLEServerCallbacks(this));

  // Create BLE Service
  pService = pServer->createService(BLE_SERVICE_UUID);

  setupCharacteristics();
  setupCallbacks();

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(BLE_SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06); // Functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  Serial.println("BLEManager: BLE service started and advertising");
}

void BLEManager::setupCharacteristics()
{
  // Ping Characteristic (Read only + Notify)
  pPingCharacteristic = pService->createCharacteristic(
      PING_CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  pPingCharacteristic->addDescriptor(new BLE2902());

  // Mode Characteristic (Read/Write + Notify)
  pModeCharacteristic = pService->createCharacteristic(
      MODE_CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);
  pModeCharacteristic->addDescriptor(new BLE2902());

  // Effects Characteristic (Read/Write + Notify)
  pEffectsCharacteristic = pService->createCharacteristic(
      EFFECTS_CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);
  pEffectsCharacteristic->addDescriptor(new BLE2902());
}

void BLEManager::setupCallbacks()
{
  // Setup callbacks for the 3 characteristics
  pPingCharacteristic->setCallbacks(new CarThingBLECharacteristicCallbacks(this, "Ping"));
  pModeCharacteristic->setCallbacks(new CarThingBLECharacteristicCallbacks(this, "Mode"));
  pEffectsCharacteristic->setCallbacks(new CarThingBLECharacteristicCallbacks(this, "Effects"));
}

void BLEManager::loop()
{
  uint32_t now = millis();

  if (!deviceConnected)
  {
    return;
  }

  // Update ping data every 1000ms
  if (now - lastPingUpdate > 1000)
  {
    updatePingData();
    lastPingUpdate = now;
  }
}

void BLEManager::end()
{
  if (pServer)
  {
    pServer->getAdvertising()->stop();
    BLEDevice::deinit();
    pServer = nullptr;
    pService = nullptr;
  }
  deviceConnected = false;
  connectionCount = 0;
}

bool BLEManager::isConnected()
{
  return deviceConnected;
}

uint16_t BLEManager::getConnectionCount()
{
  return connectionCount;
}

void BLEManager::setApplication(Application *application)
{
  app = application;
}

void BLEManager::updatePingData()
{
  if (!deviceConnected || !pPingCharacteristic)
  {
    return;
  }

  BLEPingData pingData = preparePingData();

  // Check if data has changed
  bool changed = memcmp(&pingData, &lastPingData, sizeof(BLEPingData)) != 0;
  if (changed)
  {
    pPingCharacteristic->setValue((uint8_t *)&pingData, sizeof(pingData));
    pPingCharacteristic->notify();
    lastPingData = pingData;
  }
}

// Data preparation methods
BLEPingData BLEManager::preparePingData()
{
  BLEPingData data = {};

  if (!app)
    return data;

  LEDStripManager *ledManager = LEDStripManager::getInstance();

  data.mode = static_cast<uint8_t>(app->getMode());
  data.headlight = ledManager->isStripEnabled(LEDStripType::HEADLIGHT);
  data.taillight = ledManager->isStripEnabled(LEDStripType::TAILLIGHT);
  data.underglow = ledManager->isStripEnabled(LEDStripType::UNDERGLOW);
  data.interior = ledManager->isStripEnabled(LEDStripType::INTERIOR);
  data.deviceId = (uint32_t)ESP.getEfuseMac();
  data.batteryLevel = batteryGetPercentageSmooth();
  data.uptime = millis() / 1000;

  return data;
}

BLEModeData BLEManager::prepareModeData()
{
  BLEModeData data = {};

  if (app)
  {
    data.mode = static_cast<uint8_t>(app->getMode());
  }

  return data;
}

BLEEffectsData BLEManager::prepareEffectsData()
{
  BLEEffectsData data = {};

  if (!app)
    return data;

  // Get effect states from the application
  data.leftIndicator = app->leftIndicatorEffect ? app->leftIndicatorEffect->isActive() : false;
  data.rightIndicator = app->rightIndicatorEffect ? app->rightIndicatorEffect->isActive() : false;

  if (app->headlightEffect)
  {
    data.headlightMode = static_cast<uint8_t>(app->headlightEffect->getMode());
    data.headlightSplit = app->headlightEffect->getSplit();
    bool r, g, b;
    app->headlightEffect->getColor(r, g, b);
    data.headlightR = r;
    data.headlightG = g;
    data.headlightB = b;
  }

  if (app->taillightEffect)
  {
    data.taillightMode = static_cast<uint8_t>(app->taillightEffect->getMode());
    data.taillightSplit = app->taillightEffect->getSplit();
  }

  data.brake = app->brakeInput.get();
  data.reverse = app->reverseInput.get();
  data.rgb = app->rgbEffect ? app->rgbEffect->isActive() : false;
  data.nightrider = app->nightriderEffect ? app->nightriderEffect->isActive() : false;
  data.police = app->policeEffect ? app->policeEffect->isActive() : false;
  data.policeMode = app->policeEffect ? static_cast<uint8_t>(app->policeEffect->getMode()) : 0;
  data.pulseWave = app->pulseWaveEffect ? app->pulseWaveEffect->isActive() : false;
  data.aurora = app->auroraEffect ? app->auroraEffect->isActive() : false;
  data.solidColor = app->solidColorEffect ? app->solidColorEffect->isActive() : false;
  data.colorFade = app->colorFadeEffect ? app->colorFadeEffect->isActive() : false;

  if (app->solidColorEffect)
  {
    data.solidColorPreset = static_cast<uint8_t>(app->solidColorEffect->getColorPreset());
    uint8_t r, g, b;
    app->solidColorEffect->getCustomColor(r, g, b);
    data.solidColorR = r;
    data.solidColorG = g;
    data.solidColorB = b;
  }

  return data;
}

// Characteristic callback handlers
void BLEManager::handleModeWrite(BLECharacteristic *pCharacteristic)
{
  if (!app)
    return;

  std::string value = pCharacteristic->getValue();
  if (value.length() != sizeof(BLEModeData))
  {
    Serial.println("BLE Mode: Invalid data size");
    return;
  }

  BLEModeData *data = (BLEModeData *)value.data();
  Serial.printf("BLE Mode: Setting mode to %d\n", data->mode);

  // Set the mode in the application

  switch (data->mode)
  {
  case 0:
    app->enableNormalMode();
    break;
  case 1:
    app->enableTestMode();
    break;
  case 2:
    app->enableRemoteMode();
    break;
  case 3:
    app->enableOffMode();
    break;
  default:
    break;
  }
}

void BLEManager::handleEffectsWrite(BLECharacteristic *pCharacteristic)
{
  if (!app)
    return;

  std::string value = pCharacteristic->getValue();
  if (value.length() != sizeof(BLEEffectsData))
  {
    Serial.println("BLE Effects: Invalid data size");
    return;
  }

  BLEEffectsData *data = (BLEEffectsData *)value.data();
  Serial.println("BLE Effects: Updating effects");

  // Update indicators
  if (app->leftIndicatorEffect)
    app->leftIndicatorEffect->setActive(data->leftIndicator);
  if (app->rightIndicatorEffect)
    app->rightIndicatorEffect->setActive(data->rightIndicator);

  // Update headlight
  if (app->headlightEffect)
  {
    app->headlightEffect->setMode(data->headlightMode);
    app->headlightEffect->setSplit(data->headlightSplit);
    app->headlightEffect->setColor(data->headlightR, data->headlightG, data->headlightB);
  }

  // Update taillight
  if (app->taillightEffect)
  {
    app->taillightEffect->setMode(data->taillightMode);
    app->taillightEffect->setSplit(data->taillightSplit);
  }

  // Update other effects
  if (app->rgbEffect)
    app->rgbEffect->setActive(data->rgb);
  if (app->nightriderEffect)
    app->nightriderEffect->setActive(data->nightrider);
  if (app->policeEffect)
  {
    app->policeEffect->setActive(data->police);
    app->policeEffect->setMode(static_cast<PoliceMode>(data->policeMode));
  }
  if (app->pulseWaveEffect)
    app->pulseWaveEffect->setActive(data->pulseWave);
  if (app->auroraEffect)
    app->auroraEffect->setActive(data->aurora);
  if (app->solidColorEffect)
  {
    app->solidColorEffect->setActive(data->solidColor);
    app->solidColorEffect->setColorPreset(static_cast<SolidColorPreset>(data->solidColorPreset));
    app->solidColorEffect->setCustomColor(data->solidColorR, data->solidColorG, data->solidColorB);
  }
  if (app->colorFadeEffect)
    app->colorFadeEffect->setActive(data->colorFade);
}

// BLE Server Callbacks
CarThingBLEServerCallbacks::CarThingBLEServerCallbacks(BLEManager *manager)
    : bleManager(manager)
{
}

void CarThingBLEServerCallbacks::onConnect(BLEServer *pServer)
{
  bleManager->deviceConnected = true;
  bleManager->connectionCount++;
  Serial.printf("BLE Client connected (count: %d)\n", bleManager->connectionCount);
}

void CarThingBLEServerCallbacks::onDisconnect(BLEServer *pServer)
{
  bleManager->deviceConnected = false;
  if (bleManager->connectionCount > 0)
  {
    bleManager->connectionCount--;
  }
  Serial.printf("BLE Client disconnected (count: %d)\n", bleManager->connectionCount);

  // Restart advertising
  pServer->startAdvertising();
}

// BLE Characteristic Callbacks
CarThingBLECharacteristicCallbacks::CarThingBLECharacteristicCallbacks(BLEManager *manager, const String &charName)
    : bleManager(manager), characteristicName(charName)
{
}

void CarThingBLECharacteristicCallbacks::onWrite(BLECharacteristic *pCharacteristic)
{
  Serial.println("BLE Write to " + characteristicName);

  if (characteristicName == "Mode")
  {
    bleManager->handleModeWrite(pCharacteristic);
  }
  else if (characteristicName == "Effects")
  {
    bleManager->handleEffectsWrite(pCharacteristic);
  }
}

void CarThingBLECharacteristicCallbacks::onRead(BLECharacteristic *pCharacteristic)
{
  Serial.println("BLE Read from " + characteristicName);

  // Update characteristic value based on current state
  if (characteristicName == "Ping")
  {
    BLEPingData data = bleManager->preparePingData();
    pCharacteristic->setValue((uint8_t *)&data, sizeof(data));
  }
  else if (characteristicName == "Mode")
  {
    BLEModeData data = bleManager->prepareModeData();
    pCharacteristic->setValue((uint8_t *)&data, sizeof(data));
  }
  else if (characteristicName == "Effects")
  {
    BLEEffectsData data = bleManager->prepareEffectsData();
    pCharacteristic->setValue((uint8_t *)&data, sizeof(data));
  }
}