#include "Inputs.h"
#include <Arduino.h>

HVInput::HVInput()
{
  _gpio = nullptr;
  _activeState = HV_HIGH_ACTIVE;
  _threshold = 0.0f;
  _debounceTime = 0.0f;

  _state = !_activeState;
  _lastState = _state;
  _debouncedState = _state;
  _lastRawState = _state;
  _voltage = 0.0f;
  _lastActiveTime = 0;
  _lastDebounceTime = 0;

  _enabled = false;
  _override = false;
  _overrideState = false;

  // Initialize optimization members
  _rawAdcValue = 0;
  _rawThreshold = 0;
}

HVInput::HVInput(GpIO *gpio, bool activeState, float threshold, float debounceTime)
    : _gpio(gpio), _activeState(activeState), _threshold(threshold), _debounceTime(debounceTime)
{
  _state = !_activeState;
  _lastState = _state;
  _debouncedState = _state;
  _lastRawState = _state;
  _voltage = 0.0f;
  _lastActiveTime = 0;
  _lastDebounceTime = 0;

  _enabled = true;
  _override = false;
  _overrideState = false;

  // Initialize optimization members
  _rawAdcValue = 0;
  _rawThreshold = 0;
  _calculateRawThreshold();

  // Initialize the raw ADC value if GPIO is available
  if (_gpio)
  {
    _rawAdcValue = _gpio->analogRead();
  }
}

void HVInput::_calculateRawThreshold()
{
  // Pre-calculate the threshold as a raw ADC value to avoid float calculations in update()
  // Formula: threshold_adc = (V_thresh / (V_ref * Divider)) * ADC_MAX
  _rawThreshold = (int32_t)((_threshold / (ADC_REF_VOLTAGE * DIVIDER_FACTOR)) * ADC_MAX_INT);
}

void HVInput::enable()
{
  _enabled = true;
}

void HVInput::disable()
{
  _enabled = false;
}

bool HVInput::isEnabled()
{
  return _enabled;
}

void HVInput::update()
{
  if (_override)
  {
    _lastState = _state;
    _state = _overrideState;

    if (_state == _activeState)
    {
      _lastActiveTime = millis();
    }
    return;
  }

  if (!_enabled)
    return;

  if (!_gpio)
    return;

  // 1. Read the raw ADC value (this is still the main time cost)
  int32_t newRawAdc = _gpio->analogRead();

  // 2. Apply smoothing using fast integer math
  // This is an integer-based Exponential Moving Average (EMA)
  // new_value = old_value - (old_value / N) + (new_sample / N)
  _rawAdcValue = _rawAdcValue - (_rawAdcValue / SMOOTHING_FACTOR) + (newRawAdc / SMOOTHING_FACTOR);

  // 3. Compare raw integer values - this is extremely fast
  _lastState = _state;
  bool rawState = _rawAdcValue > _rawThreshold;

  // If debouncing is disabled (debounce time is 0), use raw state directly
  if (_debounceTime <= 0)
  {
    _state = rawState;
    _debouncedState = rawState;
  }
  else
  {
    // Debouncing logic - cache millis() call for efficiency
    uint64_t currentTime = millis();

    // If the raw state has changed from the last raw state, reset the debounce timer
    if (rawState != _lastRawState)
    {
      _lastDebounceTime = currentTime;
      _lastRawState = rawState;
    }

    // If enough time has passed, update the debounced state
    if ((currentTime - _lastDebounceTime) >= _debounceTime)
    {
      _debouncedState = rawState;
    }

    _state = _debouncedState;
  }

  // Update last active time if we're in active state
  if (_state == _activeState)
  {
    _lastActiveTime = millis();
  }
}

float HVInput::getVoltage()
{
  // Calculate voltage on-demand from the smoothed raw ADC value
  // This avoids floating-point math in the critical update() path
  return ((float)_rawAdcValue / (float)ADC_MAX_INT) * ADC_REF_VOLTAGE * DIVIDER_FACTOR;
}

bool HVInput::get()
{
  return _state == _activeState;
}

bool HVInput::getLast()
{
  return _lastState == _activeState;
}

bool HVInput::isActiveFor(uint64_t duration)
{
  if (!get())
    return false;

  uint64_t currentTime = millis();
  return (currentTime - _lastActiveTime) >= duration;
}

void HVInput::setLastActiveTime(uint64_t time)
{
  _lastActiveTime = time;
}

uint64_t HVInput::getLastActiveTime()
{
  return _lastActiveTime;
}

void HVInput::override(bool state)
{
  _override = true;
  _overrideState = state;
}

void HVInput::clearOverride()
{
  _override = false;
}

bool HVInput::isOverride()
{
  return _override;
}