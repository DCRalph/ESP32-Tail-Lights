#ifndef TIMEPROFILER_H
#define TIMEPROFILER_H

#include <Arduino.h>
#include <map>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

enum class TimeUnit
{
  MICROSECONDS,
  MILLISECONDS
};

struct TimingData
{
  uint32_t time;
  TimeUnit unit;
};

class TimeProfiler
{
private:
  std::map<String, uint32_t> startTimes;
  std::map<String, TimeUnit> startUnits;
  std::map<String, TimingData> elapsedTimes;

  // Separate call counting system
  std::map<String, uint32_t> callCounts;
  std::map<String, uint32_t> callsPerSecond;

  // Mutexes for thread safety
  SemaphoreHandle_t timingMutex;
  SemaphoreHandle_t callCountMutex;

public:
  TimeProfiler();

  // Initialize the FreeRTOS task for call counter resets
  void begin();

  // === TIMING SYSTEM ===
  // Start timing for a given key (defaults to microseconds)
  void start(const String &key, TimeUnit unit = TimeUnit::MICROSECONDS);

  // Stop timing for a given key and store the elapsed time in the original unit
  void stop(const String &key);

  // Get the elapsed time for a key in its original recorded unit
  uint32_t getTime(const String &key);

  // Get the time unit that was used for recording
  TimeUnit getTimeUnit(const String &key);

  // Get time converted to microseconds (regardless of original unit)
  uint32_t getTimeUs(const String &key);

  // Get time converted to milliseconds (regardless of original unit)
  float getTimeMs(const String &key);

  // === CALL COUNTING SYSTEM (independent of timing) ===
  // Increment the call counter for a given key
  void increment(const String &key);

  // Get the number of calls per second for a key
  uint32_t getCallsPerSecond(const String &key);

  // Get the current call count for a key (since last reset)
  uint32_t getCurrentCallCount(const String &key);

  // === SHARED UTILITIES ===
  // Check if a key exists in timing or call data
  bool hasKey(const String &key);

  // Clear all timing and call data
  void clear();

  // Remove a specific key from both systems
  void remove(const String &key);

  // Print all timing and call statistics
  void printAll();

  // Internal method called by FreeRTOS task to reset call counters
  void resetCallCounters();
};

// Global instance
extern TimeProfiler timeProfiler;

#endif