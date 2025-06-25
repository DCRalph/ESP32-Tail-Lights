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

void LEDStripManager::addLEDStrip(const LEDStripConfig &config)
{

  if (strips.find(config.type) != strips.end())
  {
    Serial.println("LEDStripManager::addLEDStrip: LED strip already exists");
    return;
  }

  // Add to the map of strips
  strips[config.type] = config;
  // Serial.println("LEDStripManager::addLEDStrip: Setting FPS to " + String(drawFPS));
  strips[config.type].strip->setFPS(drawFPS);
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
    }
  }
}

void LEDStripManager::draw()
{
  timeProfiler.start("drawEffects", TimeUnit::MICROSECONDS);
  timeProfiler.increment("ledFps");

  // Draw all strips with safety checks
  for (auto &pair : strips)
  {
    if (pair.second.strip && taskRunning) // Check if we should still be running
    {
      try
      {
        pair.second.strip->draw();
        pair.second.strip->show();
      }
      catch (...)
      {
        // Log error but continue with other strips
        Serial.println("LEDStripManager: Error drawing strip type " + String((int)pair.first));
      }
    }
  }

  timeProfiler.stop("drawEffects");
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
      5,              // Priority (higher priority for LED updates)
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
    ESP_LOGE("LEDTask", "Invalid manager pointer");
    vTaskDelete(NULL);
    return;
  }

  const char *TAG = "LEDTask"; // For ESP-IDF logging

  // Validate drawFPS to prevent division by zero
  if (manager->drawFPS <= 0)
  {
    ESP_LOGE(TAG, "Invalid drawFPS value: %d, setting to default 60", manager->drawFPS);
    manager->drawFPS = 60;
  }

  // Calculate the desired period for each frame in milliseconds
  // Ensure to use floating point for division if drawFPS can be fractional
  TickType_t framePeriodTicks = pdMS_TO_TICKS(1000.0f / manager->drawFPS);

  // Prevent extremely short frame periods that could cause system instability
  if (framePeriodTicks < pdMS_TO_TICKS(1))
  {
    framePeriodTicks = pdMS_TO_TICKS(1);
    ESP_LOGW(TAG, "Frame period too short, clamped to 1ms");
  }

  ESP_LOGI(TAG, "LEDStripManager: Task loop started with FPS: %d", manager->drawFPS);

  // Use a local copy of taskRunning status to reduce race conditions
  while (manager->taskRunning)
  {
    TickType_t startTime = xTaskGetTickCount();

    // Add timeout protection for draw operations
    bool drawSuccess = true;
    try
    {
      // Draw all strips with timeout protection
      manager->draw();
    }
    catch (...)
    {
      ESP_LOGE(TAG, "Exception occurred during draw operation");
      drawSuccess = false;
    }

    TickType_t endTime = xTaskGetTickCount();
    TickType_t elapsedTime = endTime - startTime;

    // Check if we're still supposed to be running (check again to handle stop requests)
    if (!manager->taskRunning)
    {
      break;
    }

    if (framePeriodTicks > 0 && drawSuccess)
    {
      TickType_t timeToDelay = framePeriodTicks - elapsedTime;

      // If draw() took longer than the frame period, timeToDelay will be negative.
      // In such cases, we don't delay, or we might log a warning.
      if (timeToDelay < 0)
      {
        // Draw took too long for the desired FPS.
        // Optionally log this:
        ESP_LOGW(TAG, "Draw took %.2fms, longer than frame period (%.2fms). Frame rate may drop.",
                 (float)elapsedTime * (1000.0f / configTICK_RATE_HZ),
                 (float)framePeriodTicks * (1000.0f / configTICK_RATE_HZ));
        timeToDelay = pdMS_TO_TICKS(1); // Minimum delay to yield to other tasks
      }

      // Use vTaskDelay with bounds checking
      if (timeToDelay > 0 && timeToDelay < pdMS_TO_TICKS(1000))
      { // Max 1 second delay
        vTaskDelay(timeToDelay);
      }
      else
      {
        vTaskDelay(pdMS_TO_TICKS(1)); // Safe fallback delay
      }
    }
    else
    {
      vTaskDelay(pdMS_TO_TICKS(10)); // Longer delay if draw failed to prevent tight loop
    }

    // Yield to watchdog and other tasks periodically
    taskYIELD();
  }

  // Clean up when task ends
  ESP_LOGI(TAG, "LED task ending gracefully");
  manager->ledTaskHandle = NULL;
  vTaskDelete(NULL);
}
