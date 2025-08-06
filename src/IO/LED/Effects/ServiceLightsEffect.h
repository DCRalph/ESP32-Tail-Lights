#pragma once

#include "../Effects.h"
#include <stdint.h>

class ServiceLightsEffect : public LEDEffect
{
public:
  // Constructs the police light effect
  ServiceLightsEffect(uint8_t priority = 0, bool transparent = false);

  virtual void update(LEDSegment *segment) override;
  virtual void render(LEDSegment *segment, Color *buffer) override;
  virtual void onDisable() override;

  // Activate or disable the effect
  void setActive(bool active);
  bool isActive() const;

  void setSyncData(ServiceLightsSyncData syncData);
  ServiceLightsSyncData getSyncData();

  // Set the speed mode (SLOW or FAST)
  void setMode(ServiceLightsMode mode);
  ServiceLightsMode getMode() const;

  // Configuration methods
  void setColor(Color c);
  Color getColor() const;
  void setFastSpeed(float speed);
  float getFastSpeed() const;
  void setSlowSpeed(float speed);
  float getSlowSpeed() const;
  void setFastModeFlashesPerCycle(uint16_t flashes);
  uint16_t getFastModeFlashesPerCycle() const;

  void updateSlowMode(LEDSegment *segment, float deltaTime);
  void updateFastMode(LEDSegment *segment, float deltaTime);
  void updateAlternateMode(LEDSegment *segment, float deltaTime);
  void updateStrobeMode(LEDSegment *segment, float deltaTime);
  void updateScrollMode(LEDSegment *segment, float deltaTime);

  void renderSlowMode(LEDSegment *segment, Color *buffer);
  void renderFastMode(LEDSegment *segment, Color *buffer);
  void renderAlternateMode(LEDSegment *segment, Color *buffer);
  void renderStrobeMode(LEDSegment *segment, Color *buffer);
  void renderScrollMode(LEDSegment *segment, Color *buffer);

private:
  bool active;
  ServiceLightsMode mode;

  // Animation parameters
  unsigned long lastUpdateTime;
  float flashProgress;   // Tracks the current position in the flash cycle (0-1)
  float cycleProgress;   // Tracks which color is currently displayed
  uint16_t currentFlash; // Tracks the current flash count in FAST mode

  // Configuration parameters
  float fastSpeed;                  // Flash cycle speed in seconds (fast mode)
  float slowSpeed;                  // Flash cycle speed in seconds (slow mode)
  uint16_t fastModeFlashesPerCycle; // Number of flashes per cycle

  // Colors
  Color color;
};