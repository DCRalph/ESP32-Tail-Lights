#include "ClickButton.h"

ClickButton::ClickButton(uint8_t buttonPin)
{
  _r             = false;
  _pin           = buttonPin;
  _activeHigh    = LOW;
  _btnState      = !_activeHigh;
  _lastState     = _btnState;
  _clickCount    = 0;
  clicks         = 0;
  depressed      = false;
  _lastBounceTime= 0;
  debounceTime   = defaultDebounceTime;
  multiclickTime = defaultMulticlickTime;
  longClickTime  = defaultLongClickTime;
  changed        = false;
  pinMode(_pin, INPUT);
}

ClickButton::ClickButton(uint8_t buttonPin, boolean activeType)
{
  _r             = false;
  _pin           = buttonPin;
  _activeHigh    = activeType;
  _btnState      = !_activeHigh;
  _lastState     = _btnState;
  _clickCount    = 0;
  clicks         = 0;
  depressed      = 0;
  _lastBounceTime= 0;
  debounceTime   = defaultDebounceTime;
  multiclickTime = defaultMulticlickTime;
  longClickTime  = defaultLongClickTime;
  changed        = false;
  pinMode(_pin, INPUT);
}

ClickButton::ClickButton(uint8_t buttonPin, boolean activeType, boolean internalPullup)
{
  _r             = false;
  _pin           = buttonPin;
  _activeHigh    = activeType;
  _btnState      = !_activeHigh;
  _lastState     = _btnState;
  _clickCount    = 0;
  clicks         = 0;
  depressed      = 0;
  _lastBounceTime= 0;
  debounceTime   = defaultDebounceTime;
  multiclickTime = defaultMulticlickTime;
  longClickTime  = defaultLongClickTime;
  changed        = false;
  pinMode(_pin, INPUT);

  if (_activeHigh == LOW && internalPullup == CLICKBTN_PULLUP) digitalWrite(_pin,HIGH);
}

ClickButton::ClickButton(uint8_t buttonPin, int min, int max)
{
  _rValMin       = min;
  _rValMax       = max;

  _r             = true;
  _pin           = buttonPin;
  _activeHigh    = HIGH;
  _btnState      = !_activeHigh;
  _lastState     = _btnState;
  _clickCount    = 0;
  clicks         = 0;
  depressed      = 0;
  _lastBounceTime= 0;
  debounceTime   = defaultDebounceTime;
  multiclickTime = defaultMulticlickTime;
  longClickTime  = defaultLongClickTime;
  changed        = false;
  pinMode(_pin, INPUT);
}

void ClickButton::Update()
{
  long now = (long)millis();
  if (_r)
  {
    x = analogRead(_pin);
    _btnState = (x <= _rValMax && x >= _rValMin);
  }
  else
  {
    _btnState = digitalRead(_pin);
    if (!_activeHigh) _btnState = !_btnState;
  }

  if (_btnState != _lastState) _lastBounceTime = now;

  if (now - _lastBounceTime > debounceTime && _btnState != depressed)
  {
    depressed = _btnState;
    if (depressed) _clickCount++;
  }

  if (_lastState == _btnState) changed = false;
  _lastState = _btnState;

  if (!depressed && (now - _lastBounceTime) > multiclickTime)
  {
    clicks = _clickCount;
    _clickCount = 0;
    if (clicks != 0) changed = true;
  }

  if (depressed && (now - _lastBounceTime > longClickTime))
  {

    clicks = 0 - _clickCount;
    _clickCount = 0;
    if (clicks != 0) changed = true;
  }
}
