#include "TimeProfiler.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <set>

// Static task handle
static TaskHandle_t callCounterResetTaskHandle = NULL;

// Static task function for call counter resets
static void callCounterResetTask(void *parameter)
{
  TimeProfiler *profiler = (TimeProfiler *)parameter;

  for (;;)
  {
    vTaskDelay(1000 / portTICK_PERIOD_MS); // Wait 1 second
    profiler->resetCallCounters();
  }
}

TimeProfiler::TimeProfiler()
{
  // Constructor - maps are automatically initialized
  // Initialize mutexes
  timingMutex = xSemaphoreCreateMutex();
  callCountMutex = xSemaphoreCreateMutex();
}

void TimeProfiler::begin()
{
  // Create FreeRTOS task to reset call counters every second
  xTaskCreate(
      callCounterResetTask,       // Task function
      "TimeProfilerCallReset",    // Task name
      2048,                       // Stack size (bytes)
      this,                       // Parameter passed to task
      1,                          // Task priority (low priority)
      &callCounterResetTaskHandle // Task handle
  );
}

void TimeProfiler::start(const String &key, TimeUnit unit)
{
  if (xSemaphoreTake(timingMutex, portMAX_DELAY) == pdTRUE)
  {
    if (unit == TimeUnit::MICROSECONDS)
    {
      startTimes[key] = micros();
    }
    else
    {
      startTimes[key] = millis();
    }
    startUnits[key] = unit;
    xSemaphoreGive(timingMutex);
  }
}

void TimeProfiler::stop(const String &key)
{
  if (xSemaphoreTake(timingMutex, portMAX_DELAY) == pdTRUE)
  {
    if (startTimes.find(key) != startTimes.end() && startUnits.find(key) != startUnits.end())
    {
      TimingData data;
      data.unit = startUnits[key];

      if (data.unit == TimeUnit::MICROSECONDS)
      {
        data.time = micros() - startTimes[key];
      }
      else
      {
        data.time = millis() - startTimes[key];
      }

      elapsedTimes[key] = data;
      startTimes.erase(key); // Remove from start times to free memory
      startUnits.erase(key); // Remove from start units to free memory
    }
    xSemaphoreGive(timingMutex);
  }
}

uint32_t TimeProfiler::getTime(const String &key)
{
  uint32_t result = 0;
  if (xSemaphoreTake(timingMutex, portMAX_DELAY) == pdTRUE)
  {
    auto it = elapsedTimes.find(key);
    if (it != elapsedTimes.end())
    {
      result = it->second.time;
    }
    xSemaphoreGive(timingMutex);
  }
  return result;
}

TimeUnit TimeProfiler::getTimeUnit(const String &key)
{
  TimeUnit result = TimeUnit::MICROSECONDS; // Default fallback
  if (xSemaphoreTake(timingMutex, portMAX_DELAY) == pdTRUE)
  {
    auto it = elapsedTimes.find(key);
    if (it != elapsedTimes.end())
    {
      result = it->second.unit;
    }
    xSemaphoreGive(timingMutex);
  }
  return result;
}

uint32_t TimeProfiler::getTimeUs(const String &key)
{
  uint32_t result = 0;
  if (xSemaphoreTake(timingMutex, portMAX_DELAY) == pdTRUE)
  {
    auto it = elapsedTimes.find(key);
    if (it != elapsedTimes.end())
    {
      if (it->second.unit == TimeUnit::MICROSECONDS)
      {
        result = it->second.time;
      }
      else
      {
        result = it->second.time * 1000; // Convert ms to us
      }
    }
    xSemaphoreGive(timingMutex);
  }
  return result;
}

float TimeProfiler::getTimeMs(const String &key)
{
  float result = 0.0f;
  if (xSemaphoreTake(timingMutex, portMAX_DELAY) == pdTRUE)
  {
    auto it = elapsedTimes.find(key);
    if (it != elapsedTimes.end())
    {
      if (it->second.unit == TimeUnit::MILLISECONDS)
      {
        result = (float)it->second.time;
      }
      else
      {
        result = it->second.time / 1000.0f; // Convert us to ms
      }
    }
    xSemaphoreGive(timingMutex);
  }
  return result;
}

// === CALL COUNTING SYSTEM METHODS ===
void TimeProfiler::increment(const String &key)
{
  if (xSemaphoreTake(callCountMutex, portMAX_DELAY) == pdTRUE)
  {
    callCounts[key]++;
    xSemaphoreGive(callCountMutex);
  }
}

uint32_t TimeProfiler::getCallsPerSecond(const String &key)
{
  uint32_t result = 0;
  if (xSemaphoreTake(callCountMutex, portMAX_DELAY) == pdTRUE)
  {
    auto it = callsPerSecond.find(key);
    if (it != callsPerSecond.end())
    {
      result = it->second;
    }
    xSemaphoreGive(callCountMutex);
  }
  return result;
}

uint32_t TimeProfiler::getCurrentCallCount(const String &key)
{
  uint32_t result = 0;
  if (xSemaphoreTake(callCountMutex, portMAX_DELAY) == pdTRUE)
  {
    auto it = callCounts.find(key);
    if (it != callCounts.end())
    {
      result = it->second;
    }
    xSemaphoreGive(callCountMutex);
  }
  return result;
}

void TimeProfiler::resetCallCounters()
{
  if (xSemaphoreTake(callCountMutex, portMAX_DELAY) == pdTRUE)
  {
    // Copy current call counts to calls per second
    for (const auto &pair : callCounts)
    {
      callsPerSecond[pair.first] = pair.second;
    }

    // Reset call counters for next second
    callCounts.clear();
    xSemaphoreGive(callCountMutex);
  }
}

// === SHARED UTILITIES ===
bool TimeProfiler::hasKey(const String &key)
{
  bool result = false;

  // Check timing data
  if (xSemaphoreTake(timingMutex, portMAX_DELAY) == pdTRUE)
  {
    result = elapsedTimes.find(key) != elapsedTimes.end();
    xSemaphoreGive(timingMutex);
  }

  // If not found in timing data, check call count data
  if (!result && xSemaphoreTake(callCountMutex, portMAX_DELAY) == pdTRUE)
  {
    result = callsPerSecond.find(key) != callsPerSecond.end() ||
             callCounts.find(key) != callCounts.end();
    xSemaphoreGive(callCountMutex);
  }

  return result;
}

void TimeProfiler::clear()
{
  // Clear timing data
  if (xSemaphoreTake(timingMutex, portMAX_DELAY) == pdTRUE)
  {
    startTimes.clear();
    startUnits.clear();
    elapsedTimes.clear();
    xSemaphoreGive(timingMutex);
  }

  // Clear call count data
  if (xSemaphoreTake(callCountMutex, portMAX_DELAY) == pdTRUE)
  {
    callCounts.clear();
    callsPerSecond.clear();
    xSemaphoreGive(callCountMutex);
  }
}

void TimeProfiler::remove(const String &key)
{
  // Remove from timing data
  if (xSemaphoreTake(timingMutex, portMAX_DELAY) == pdTRUE)
  {
    startTimes.erase(key);
    startUnits.erase(key);
    elapsedTimes.erase(key);
    xSemaphoreGive(timingMutex);
  }

  // Remove from call count data
  if (xSemaphoreTake(callCountMutex, portMAX_DELAY) == pdTRUE)
  {
    callCounts.erase(key);
    callsPerSecond.erase(key);
    xSemaphoreGive(callCountMutex);
  }
}

void TimeProfiler::printAll()
{
  Serial.println("=== TimeProfiler Results ===");

  // Collect all unique keys from both systems
  std::set<String> allKeys;

  // Collect keys from timing system
  if (xSemaphoreTake(timingMutex, portMAX_DELAY) == pdTRUE)
  {
    for (const auto &pair : elapsedTimes)
    {
      allKeys.insert(pair.first);
    }
    xSemaphoreGive(timingMutex);
  }

  // Collect keys from call count system
  if (xSemaphoreTake(callCountMutex, portMAX_DELAY) == pdTRUE)
  {
    for (const auto &pair : callsPerSecond)
    {
      allKeys.insert(pair.first);
    }
    for (const auto &pair : callCounts)
    {
      allKeys.insert(pair.first);
    }
    xSemaphoreGive(callCountMutex);
  }

  for (const String &key : allKeys)
  {
    Serial.print(key);
    Serial.print(": ");

    // Print timing data if available
    bool hasTimingData = false;
    if (xSemaphoreTake(timingMutex, portMAX_DELAY) == pdTRUE)
    {
      auto timingIt = elapsedTimes.find(key);
      if (timingIt != elapsedTimes.end())
      {
        hasTimingData = true;
        Serial.print(timingIt->second.time);
        if (timingIt->second.unit == TimeUnit::MICROSECONDS)
        {
          Serial.print(" μs (");
          Serial.print(timingIt->second.time / 1000.0f);
          Serial.print(" ms)");
        }
        else
        {
          Serial.print(" ms (");
          Serial.print(timingIt->second.time * 1000);
          Serial.print(" μs)");
        }
      }
      xSemaphoreGive(timingMutex);
    }

    if (!hasTimingData)
    {
      Serial.print("no timing data");
    }

    // Print call data if available
    if (xSemaphoreTake(callCountMutex, portMAX_DELAY) == pdTRUE)
    {
      auto cpsIt = callsPerSecond.find(key);
      if (cpsIt != callsPerSecond.end())
      {
        Serial.print(" - ");
        Serial.print(cpsIt->second);
        Serial.print(" calls/sec");
      }

      auto currentIt = callCounts.find(key);
      if (currentIt != callCounts.end())
      {
        Serial.print(" (current: ");
        Serial.print(currentIt->second);
        Serial.print(")");
      }
      xSemaphoreGive(callCountMutex);
    }

    Serial.println();
  }
  Serial.println("=============================");
}

// Global instance
TimeProfiler timeProfiler;