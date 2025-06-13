#ifndef ClickButton_H
#define ClickButton_H

#if (ARDUINO <  100)
#include <WProgram.h>
#else
#include <Arduino.h>
#endif

#define CLICKBTN_PULLUP HIGH

#define defaultDebounceTime 20
#define defaultMulticlickTime 250
#define defaultLongClickTime 1000

class ClickButton
{
  public:
    ClickButton(uint8_t buttonPin);
    ClickButton(uint8_t buttonPin, boolean active);
    ClickButton(uint8_t buttonPin, boolean active, boolean internalPullup);
    ClickButton(uint8_t buttonPin, int min, int max);

    void Update();

    int clicks;
    boolean depressed;
    long debounceTime;
    long multiclickTime;
    long longClickTime;
    boolean changed;

  private:
    boolean _r;
    int _rValMin;
    int _rValMax;
    int x;

    uint8_t _pin;
    boolean _activeHigh;      // Active-low = 0, active-high = 1
    boolean _btnState;
    boolean _lastState;
    int _clickCount;
    long _lastBounceTime;
};

#endif
