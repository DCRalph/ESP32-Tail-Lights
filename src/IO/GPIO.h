#pragma once
#include "config.h"
#include "ClickButton.h"

class GpIO;

extern GpIO btnBoot;
extern GpIO btnPrev;
extern GpIO btnSel;
extern GpIO btnNext;

extern GpIO voltage;

extern GpIO input1;
extern GpIO input2;
extern GpIO input3;
extern GpIO input4;
extern GpIO input5;
extern GpIO input6;
extern GpIO input7;
extern GpIO input8;

extern ClickButton BtnBoot;
extern ClickButton BtnPrev;
extern ClickButton BtnSel;
extern ClickButton BtnNext;

enum PinMode
{
  Input = INPUT,
  Output = OUTPUT,
  InputPullup = INPUT_PULLUP
};

class GpIO
{
private:
  uint8_t pin;
  PinMode mode;
  bool activeState;

  bool debounceEnabled;
  unsigned long debounceTime;
  unsigned long lastDebounceTime;
  bool lastStableValue;
  bool lastReadValue;

  String PinModeString(PinMode mode);

public:
  GpIO();
  GpIO(uint8_t _pin, PinMode _mode);
  GpIO(uint8_t _pin, PinMode _mode, bool _activeState);

  void init();

  void SetMode(PinMode _mode);
  void setActive(bool _activeState);

  void Write(bool _value);
  void PWM(uint8_t _value);

  bool read();
  int analogRead();

  void Toggle();
  void On();
  void Off();

  void enableDebounce(unsigned long debounceTime);

  uint8_t getPin();
  PinMode getMode();

  static void initIO();
};