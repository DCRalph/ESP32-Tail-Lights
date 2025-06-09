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

private:
  bool _enabled;

  GpIO *_gpio;
  bool _state;
  bool _activeState;
  bool _debouncedState;
  bool _lastState;
  bool _lastRawState;
  float _voltage;
  float _threshold;
  uint64_t _lastDebounceTime;

  uint64_t _lastActiveTime;
  float _debounceTime;

  bool _override;
  bool _overrideState;

public:
  HVInput(GpIO *gpio, bool activeState, float threshold = DEFAULT_VOLTAGE_THRESHOLD, float debounceTime = DEFAULT_DEBOUNCE_TIME);
  void enable();
  void disable();
  bool isEnabled();

  void update();
  float getVoltage();
  bool get();
  bool getLast();
  bool isActiveFor(uint64_t duration);
  void setLastActiveTime(uint64_t time);
  uint64_t getLastActiveTime();

  void override(bool state);
  void clearOverride();
};