#include "GPIO.h"

GpIO io0(0, InputPullup, LOW);
GpIO led(LED_PIN, Output, HIGH);

#ifdef ENABLE_HV_INPUTS
GpIO input1(INPUT_1_PIN, Input);
GpIO input2(INPUT_2_PIN, Input);
GpIO input3(INPUT_3_PIN, Input);
GpIO input4(INPUT_4_PIN, Input);
GpIO input5(INPUT_5_PIN, Input);
GpIO input6(INPUT_6_PIN, Input);
#endif

String GpIO::PinModeString(PinMode mode)
{
  switch (mode)
  {
  case Input:
    return "Input";
  case Output:
    return "Output";
  case InputPullup:
    return "InputPullup";
  default:
    return "Unknown";
  }
}

GpIO::GpIO()
{
  pin = -1;
  mode = Input;
  activeState = HIGH;
}

GpIO::GpIO(uint8_t _pin, PinMode _mode)
{
  pin = _pin;
  mode = _mode;
  activeState = HIGH;

  debounceEnabled = false;
  debounceTime = 0;
  lastDebounceTime = 0;
  lastStableValue = digitalRead(pin);
  lastReadValue = lastStableValue;
}

GpIO::GpIO(uint8_t _pin, PinMode _mode, bool _activeState)
{
  pin = _pin;
  mode = _mode;
  activeState = _activeState;

  debounceEnabled = false;
  debounceTime = 0;
  lastDebounceTime = 0;
  lastStableValue = digitalRead(pin);
  lastReadValue = lastStableValue;
}

void GpIO::init()
{
  Serial.println("\t[GPIO] " + String(pin) + " as " + PinModeString(mode) + " Initializing...");
  pinMode(pin, mode);

  if (mode == Output)
    digitalWrite(pin, !activeState);

  // Serial.println("\t[GPIO] Initialized");
}

void GpIO::SetMode(PinMode _mode)
{
  mode = _mode;
  pinMode(pin, mode);
}

void GpIO::setActive(bool _activeState)
{
  activeState = _activeState;
}

void GpIO::Write(bool _value)
{
  if (mode == Input)
    return;
  digitalWrite(pin, _value ^ !activeState);
}

void GpIO::PWM(uint8_t _value)
{
  if (mode == Input)
    return;
  analogWrite(pin, _value);
}

bool GpIO::read()
{
  if (debounceEnabled)
  {
    bool currentValue = digitalRead(pin);
    if (currentValue != lastReadValue)
    {
      lastDebounceTime = millis();
      lastReadValue = currentValue;
    }

    if ((millis() - lastDebounceTime) > debounceTime)
    {
      if (lastStableValue != lastReadValue)
      {
        lastStableValue = lastReadValue;
      }
    }

    return lastStableValue == activeState;
  }
  else
  {
    return digitalRead(pin) == activeState;
  }
}

int GpIO::analogRead()
{
  return ::analogRead(pin);
}

void GpIO::Toggle()
{
  if (mode == Input)
    return;
  digitalWrite(pin, !digitalRead(pin));
}

void GpIO::On()
{
  if (mode == Input)
    return;
  digitalWrite(pin, activeState);
}

void GpIO::Off()
{
  if (mode == Input)
    return;
  digitalWrite(pin, !activeState);
}

void GpIO::enableDebounce(unsigned long _debounceTime)
{
  debounceEnabled = true;
  debounceTime = _debounceTime;
  lastDebounceTime = millis();
  lastStableValue = digitalRead(pin);
  lastReadValue = lastStableValue;
}

uint8_t GpIO::getPin()
{
  return pin;
}

PinMode GpIO::getMode()
{
  return mode;
}

void GpIO::initIO()
{
  Serial.println("\t[INFO] [IO] Configuring pins...");

  io0.init();
  io0.enableDebounce(50);
  led.init();

#ifdef ENABLE_HV_INPUTS
  input1.init();
  input2.init();
  input3.init();
  input4.init();
  input5.init();
  input6.init();
#endif

  Serial.println("\t[INFO] [IO] Pins configured.");
  Serial.println();
}
