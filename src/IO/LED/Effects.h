#pragma once

#include "LEDStrip.h"
#include <stdint.h>
#include "Sync/SyncManager.h"

struct Color;
class LEDStrip;

// Base class for LED effects.
class LEDEffect
{
public:
  // Each effect has a priority and may be "transparent" so that lower layers show
  LEDEffect(uint8_t priority = 0, bool transparent = false);
  virtual ~LEDEffect();

  // Called each update to change animation state.
  virtual void update(LEDStrip *strip) = 0;

  // Called to render the effect into the provided LED buffer.
  virtual void render(LEDStrip *strip, Color *buffer) = 0;

  virtual void onDisable() = 0;

  uint8_t getPriority() const;
  bool isTransparent() const;
  void setPriority(uint8_t priority);
  void setTransparent(bool transparent);

  static std::vector<LEDEffect *> getEffects();
  static void disableAllEffects();

protected:
  uint8_t priority;
  bool transparent;

private:
  static std::vector<LEDEffect *> effects;
};