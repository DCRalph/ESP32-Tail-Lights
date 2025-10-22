#include "LEDStripManager.h"
#include "LEDStrip.h" // Include this for full definitions
#include "../../config.h"
#include <Arduino.h>
#include <set> // Add include for std::set
#include <map>
#include "IO/TimeProfiler.h"

// Initialize static instance pointer
LEDStripManager *LEDStripManager::instance = nullptr;

LEDStripManager *LEDStripManager::getInstance()
{
  if (!instance)
  {
    instance = new LEDStripManager();
  }
  return instance;
}

LEDStripManager::LEDStripManager()
{
  // Store this instance in the static pointer for callbacks to use
  instance = this;
  drawFPS = 200;
  ledTaskHandle = NULL;
  taskRunning = false;
}

LEDStripManager::~LEDStripManager()
{
  // Clean up all LEDManager instances
  for (auto &pair : strips)
  {
    if (pair.second.strip)
    {
      delete pair.second.strip;
      pair.second.strip = nullptr;
    }
  }

  strips.clear();

  // Clear the static instance
  if (instance == this)
  {
    instance = nullptr;
  }
}

void LEDStripManager::begin()
{
  setBrightness(255);
}

LEDStrip *LEDStripManager::getStrip(LEDStripType type)
{
  if (strips.find(type) != strips.end())
  {
    return strips[type].strip;
  }
  return nullptr;
}

std::map<LEDStripType, LEDStripConfig> LEDStripManager::getStrips()
{
  return strips;
}

Color *LEDStripManager::getStripBuffer(LEDStripType type)
{
  if (strips.find(type) != strips.end() && strips[type].strip)
  {
    return strips[type].strip->getBuffer();
  }
  return nullptr;
}

uint16_t LEDStripManager::getStripLEDCount(LEDStripType type)
{
  if (strips.find(type) != strips.end())
  {
    return strips[type].strip ? strips[type].strip->getNumLEDs() : 0;
  }
  return 0;
}

bool LEDStripManager::isStripEnabled(LEDStripType type)
{
  return strips.count(type) > 0;
}

bool LEDStripManager::isStripActive(LEDStripType type)
{
  if (strips.find(type) != strips.end())
  {
    return strips[type].strip ? strips[type].strip->getActive() : false;
  }
  return false;
}

void LEDStripManager::setStripActive(LEDStripType type, bool active)
{
  if (strips.find(type) != strips.end())
  {
    strips[type].strip->setActive(active);
  }
}

void LEDStripManager::addLEDStrip(const LEDStripConfig &config)
{

  if (strips.find(config.type) != strips.end())
  {
    Serial.println("LEDStripManager::addLEDStrip: LED strip already exists");
    return;
  }

  // Add to the map of strips
  strips[config.type] = config;
  // strips[config.type].strip->clearBuffer();
  // Serial.println("LEDStripManager::addLEDStrip: Setting FPS to " + String(drawFPS));
  // strips[config.type].strip->setFPS(drawFPS);
  strips[config.type].strip->setBrightness(255);
  // strips[config.type].strip->start();
}

void LEDStripManager::setBrightness(uint8_t brightness)
{
  for (auto &pair : strips)
  {
    if (pair.second.strip)
      pair.second.strip->setBrightness(brightness);
  }
}

void LEDStripManager::updateEffects()
{
  for (auto &pair : strips)
  {
    if (pair.second.strip)
    {
      pair.second.strip->updateEffects();
      // Serial.println("Updated effects for strip " + pair.second.name);
      // Color::print(pair.second.strip->getBuffer(), pair.second.strip->getNumLEDs());
    }
  }
}

void LEDStripManager::draw()
{
  timeProfiler.start("ledFps", TimeUnit::MICROSECONDS);
  timeProfiler.increment("ledFps");

  // Draw all strips with safety checks
  for (auto &pair : strips)
  {
    if (pair.second.strip) // Check if we should still be running
    {
      timeProfiler.start("draw-" + pair.second.name, TimeUnit::MICROSECONDS);
      timeProfiler.increment("draw-" + pair.second.name);
      pair.second.strip->draw();
      timeProfiler.stop("draw-" + pair.second.name);

      timeProfiler.start("show-" + pair.second.name, TimeUnit::MICROSECONDS);
      timeProfiler.increment("show-" + pair.second.name);
      pair.second.strip->show();
      timeProfiler.stop("show-" + pair.second.name);
    }
    // else
    // {
    //   Serial.println("LEDStripManager: Strip not running");
    // }
  }

  // timeProfiler.start("show", TimeUnit::MICROSECONDS);
  // timeProfiler.increment("show");
  // FastLED.show();
  // timeProfiler.stop("show");

  timeProfiler.stop("ledFps");
}

// Task management functions
void LEDStripManager::startTask()
{
  if (taskRunning)
  {
    Serial.println("LEDStripManager: Task already running");
    return;
  }

  taskRunning = true;

  // Create the LED task with high priority for smooth LED updates
  xTaskCreatePinnedToCore(
      ledTask,        // Task function
      "LEDStripTask", // Task name
      4096,           // Stack size
      this,           // Parameter passed to task
      10,             // Priority (higher priority for LED updates)
      &ledTaskHandle, // Task handle
      0               // Core to run on
  );

  Serial.println("LEDStripManager: Task started");
}

void LEDStripManager::stopTask()
{
  if (!taskRunning)
  {
    Serial.println("LEDStripManager: Task not running");
    return;
  }

  Serial.println("LEDStripManager: Stopping task...");

  // Signal the task to stop
  taskRunning = false;

  // Wait for the task to actually finish (with timeout)
  if (ledTaskHandle != NULL)
  {
    // Give the task time to finish gracefully
    TickType_t timeout = pdMS_TO_TICKS(1000); // 1 second timeout
    TickType_t startTime = xTaskGetTickCount();

    while (ledTaskHandle != NULL && (xTaskGetTickCount() - startTime) < timeout)
    {
      vTaskDelay(pdMS_TO_TICKS(10)); // Wait 10ms
    }

    // Force delete if task didn't finish gracefully
    if (ledTaskHandle != NULL)
    {
      Serial.println("LEDStripManager: Force deleting task");
      vTaskDelete(ledTaskHandle);
      ledTaskHandle = NULL;
    }
  }

  Serial.println("LEDStripManager: Task stopped");
}

bool LEDStripManager::isTaskRunning()
{
  return taskRunning;
}

// Static task function for FreeRTOS
void LEDStripManager::ledTask(void *parameter)
{
  LEDStripManager *manager = static_cast<LEDStripManager *>(parameter);

  // Validate manager pointer
  if (!manager)
  {
    Serial.println("[LEDTask] ERROR: Invalid manager pointer");
    vTaskDelete(NULL);
    return;
  }

  const char *TAG = "LEDTask";

  // Validate drawFPS to prevent division by zero
  if (manager->drawFPS <= 0)
  {
    Serial.print("[LEDTask] ERROR: Invalid drawFPS value: ");
    Serial.print(manager->drawFPS);
    Serial.println(", setting to default 60");
    manager->drawFPS = 60;
  }

  TickType_t framePeriodTicks = pdMS_TO_TICKS(1000.0f / manager->drawFPS);

  if (framePeriodTicks < pdMS_TO_TICKS(1))
  {
    framePeriodTicks = pdMS_TO_TICKS(1);
    Serial.println("[LEDTask] WARNING: Frame period too short, clamped to 1ms");
  }

  Serial.print("[LEDTask] LEDStripManager: Task loop started with FPS: ");
  Serial.println(manager->drawFPS);

  while (manager->taskRunning)
  {
    TickType_t startTime = xTaskGetTickCount();

    manager->draw(); // draws buffers and shows them

    TickType_t endTime = xTaskGetTickCount();
    TickType_t elapsedTime = endTime - startTime;

    if (!manager->taskRunning)
    {
      break;
    }

    if (framePeriodTicks > 0)
    {
      TickType_t timeToDelay = framePeriodTicks - elapsedTime;

      if (timeToDelay < 0)
      {
        Serial.print("[LEDTask] WARNING: Draw took ");
        Serial.print((float)elapsedTime * (1000.0f / configTICK_RATE_HZ));
        Serial.print("ms, longer than frame period (");
        Serial.print((float)framePeriodTicks * (1000.0f / configTICK_RATE_HZ));
        Serial.println("ms). Frame rate may drop.");
        timeToDelay = pdMS_TO_TICKS(1);
      }

      if (timeToDelay > 0 && timeToDelay < pdMS_TO_TICKS(1000))
        vTaskDelay(timeToDelay);
      else
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    else
    {
      vTaskDelay(pdMS_TO_TICKS(10)); // Longer delay if draw failed to prevent tight loop
    }

    // Yield to watchdog and other tasks periodically
    taskYIELD();
  }

  // Clean up when task ends
  Serial.println("[LEDTask] LED task ending gracefully");
  manager->ledTaskHandle = NULL;
  vTaskDelete(NULL);
}
