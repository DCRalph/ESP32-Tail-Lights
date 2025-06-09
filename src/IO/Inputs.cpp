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
  if (!_enabled)
    return;

  if (!_gpio)
    return;

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

  float adcReading = (float)_gpio->analogRead();
  float newVoltage = (adcReading / ADC_MAX) * ADC_REF_VOLTAGE * DIVIDER_FACTOR;

  _voltage = (_voltage * DEFAULT_SMOOTH_FACTOR) + (newVoltage * (1.0f - DEFAULT_SMOOTH_FACTOR));
  _lastState = _state;
  bool rawState = _voltage > _threshold;

  // If debouncing is disabled (debounce time is 0), use raw state directly
  if (_debounceTime <= 0)
  {
    _state = rawState;
    _debouncedState = rawState;
  }
  else
  {
    // Debouncing logic
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
  return _voltage;
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