#include "GPIO.h"

#ifdef S3_V1
GpIO btnBoot(INPUT_BTN_BOOT_PIN, InputPullup, LOW);
GpIO btnPrev(INPUT_BTN_PREV_PIN, InputPullup, LOW);
GpIO btnSel(INPUT_BTN_SEL_PIN, InputPullup, LOW);
GpIO btnNext(INPUT_BTN_NEXT_PIN, InputPullup, LOW);

GpIO voltage(INPUT_VOLTAGE_PIN, Input);
#endif

#ifdef ENABLE_HV_INPUTS
GpIO input1(INPUT_1_PIN, Input);
GpIO input2(INPUT_2_PIN, Input);
GpIO input3(INPUT_3_PIN, Input);
GpIO input4(INPUT_4_PIN, Input);
GpIO input5(INPUT_5_PIN, Input);
GpIO input6(INPUT_6_PIN, Input);
GpIO input7(INPUT_7_PIN, Input);
GpIO input8(INPUT_8_PIN, Input);
#endif

ClickButton BtnBoot(INPUT_BTN_BOOT_PIN, LOW);
ClickButton BtnPrev(INPUT_BTN_PREV_PIN, LOW);
ClickButton BtnSel(INPUT_BTN_SEL_PIN, LOW);
ClickButton BtnNext(INPUT_BTN_NEXT_PIN, LOW);

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

#ifdef S3_V1
  btnBoot.init();
  btnBoot.enableDebounce(50);
  btnPrev.init();
  btnPrev.enableDebounce(50);
  btnSel.init();
  btnSel.enableDebounce(50);
  btnNext.init();
  btnNext.enableDebounce(50);

  BtnBoot.debounceTime = 20;
  BtnBoot.multiclickTime = 100;
  BtnBoot.longClickTime = 300;
  BtnPrev.debounceTime = 20;
  BtnPrev.multiclickTime = 100;
  BtnPrev.longClickTime = 300;
  BtnSel.debounceTime = 20;
  BtnSel.multiclickTime = 100;
  BtnSel.longClickTime = 300;
  BtnNext.debounceTime = 20;
  BtnNext.multiclickTime = 100;
  BtnNext.longClickTime = 300;

  voltage.init();
#endif

#ifdef ENABLE_HV_INPUTS
  input1.init();
  input2.init();
  input3.init();
  input4.init();
  input5.init();
  input6.init();
  input7.init();
  input8.init();
#endif

  Serial.println("\t[INFO] [IO] Pins configured.");
  Serial.println();
}
