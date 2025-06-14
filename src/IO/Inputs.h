#pragma once

#include "config.h"

#define HV_HIGH_ACTIVE true
#define HV_LOW_ACTIVE false

class HVInput
{
  static constexpr float DEFAULT_SMOOTH_FACTOR = 0.5f;
  static constexpr float ADC_MAX = 8192;
  static constexpr float ADC_REF_VOLTAGE = 3.3;
  static constexpr float DIVIDER_FACTOR = 10.0;
  static constexpr float DEFAULT_VOLTAGE_THRESHOLD = 3;

  static constexpr float DEFAULT_DEBOUNCE_TIME = 20;

  // --- OPTIMIZATION CONSTANTS ---
  // Integer-based smoothing factor. Using power of 2 for fast bit-shift operations
  // A factor of 8 means we keep 7/8 of the old value and add 1/8 of the new one
  static constexpr int32_t SMOOTHING_FACTOR = 8;
  static constexpr int32_t ADC_MAX_INT = 8192;

private:
  bool _enabled;

  GpIO *_gpio;
  bool _state;
  bool _activeState;
  bool _debouncedState;
  bool _lastState;
  bool _lastRawState;
  float _voltage; // Keep for backward compatibility, but calculate on-demand
  float _threshold;
  uint64_t _lastDebounceTime;

  uint64_t _lastActiveTime;
  float _debounceTime;

  bool _override;
  bool _overrideState;

  // --- OPTIMIZATION MEMBERS ---
  // Raw ADC values for integer-based processing
  int32_t _rawAdcValue;
  int32_t _rawThreshold;

public:
  HVInput();
  HVInput(GpIO *gpio, bool activeState, float threshold = DEFAULT_VOLTAGE_THRESHOLD, float debounceTime = DEFAULT_DEBOUNCE_TIME);
  void enable();
  void disable();
  bool isEnabled();

  void update();
  float getVoltage(); // Now calculates voltage on-demand from raw ADC value
  bool get();
  bool getLast();
  bool isActiveFor(uint64_t duration);
  void setLastActiveTime(uint64_t time);
  uint64_t getLastActiveTime();

  void override(bool state);
  void clearOverride();

private:
  // Helper function to calculate raw threshold from voltage threshold
  void _calculateRawThreshold();
};