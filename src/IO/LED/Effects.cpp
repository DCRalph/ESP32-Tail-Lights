#include "Effects.h"
#include <Arduino.h> // For millis()

//
// LEDEffect Base Class Implementation
//
LEDEffect::LEDEffect(LEDManager *_ledManager, uint8_t priority, bool transparent)
    : ledManager(_ledManager), priority(priority), transparent(transparent) {}

LEDEffect::~LEDEffect() {}

uint8_t LEDEffect::getPriority() const { return priority; }
bool LEDEffect::isTransparent() const { return transparent; }
void LEDEffect::setPriority(uint8_t prio) { priority = prio; }
void LEDEffect::setTransparent(bool transp) { transparent = transp; }

// //
// // BrakeLightEffect Implementation
// //
// BrakeLightEffect::BrakeLightEffect(uint16_t numLEDs, uint8_t priority,
//                                    bool transparent)
//     : LEDEffect(priority, transparent),
//       numLEDs(numLEDs),
//       brakeActive(false),
//       brightness(1.0f),
//       dimStep(0.01f) {}

// void BrakeLightEffect::setActive(bool active) {
//   if (active) {
//     // When brakes are applied, ensure full red immediately.
//     brakeActive = true;
//     brightness = 1.0f;
//   } else {
//     brakeActive = false;
//     // After brake release, begin the dimming/fade-out.
//   }
// }

// void BrakeLightEffect::update() {
//   if (!brakeActive) {
//     // Gradually dim the overall brightness.
//     if (brightness > 0.0f) {
//       brightness -= dimStep;
//       if (brightness < 0.0f) {
//         brightness = 0.0f;
//       }
//     }
//   } else {
//     // If brakes are applied, maintain full brightness.
//     brightness = 1.0f;
//   }
// }

// void BrakeLightEffect::render(std::vector<Color> &buffer) {
//   uint16_t len = buffer.size();

//   if (brakeActive) {
//     // While active, paint every LED full red.
//     for (uint16_t i = 0; i < len; i++) {
//       buffer[i].r = 255;
//       buffer[i].g = 0;
//       buffer[i].b = 0;
//     }
//   } else {
//     // When brakes are released, use a gradient to fade out the red color.
//     // The center stays red longer while the edges fade quickly.
//     uint16_t mid = len / 2;
//     for (uint16_t i = 0; i < len; i++) {
//       // Calculate the normalized distance from the center.
//       float normDist = (mid > 0) ? (fabs((int)i - (int)mid) / (float)mid)
//                                  : 0.0f;
//       // Local brightness is lower at the edges.
//       // (Note: When brakes are just released, brightness==1.0f so the very edge
//       // becomes off immediately. You can adjust dimStep or add a delay if needed.)
//       float localBrightness = brightness * (1.0f - normDist);
//       uint8_t redVal = (uint8_t)(255 * localBrightness);
//       buffer[i].r = redVal;
//       buffer[i].g = 0;
//       buffer[i].b = 0;
//     }
//   }
// }

// //
// // IndicatorEffect Implementation
// //
// IndicatorEffect::IndicatorEffect(uint16_t numLEDs, Side side,
//                                  uint8_t priority, bool transparent)
//     : LEDEffect(priority, transparent),
//       numLEDs(numLEDs),
//       side(side),
//       indicatorActive(false),
//       lastToggleTime(0),
//       blinkInterval(500), // Blink every 500 ms
//       indicatorOn(false) {}

// void IndicatorEffect::setIndicatorActive(bool active) {
//   indicatorActive = active;
//   if (!active) {
//     indicatorOn = false;
//   }
// }

// void IndicatorEffect::update() {
//   if (indicatorActive) {
//     unsigned long currentTime = millis();
//     if (currentTime - lastToggleTime >= blinkInterval) {
//       indicatorOn = !indicatorOn;
//       lastToggleTime = currentTime;
//     }
//   } else {
//     indicatorOn = false;
//   }
// }

// void IndicatorEffect::render(std::vector<Color> &buffer) {
//   if (!indicatorOn)
//     return; // Do nothing if the indicator is off in this blink phase.

//   // Determine which portion of the LED strip to control.
//   int start = 0, end = numLEDs;
//   if (side == LEFT) {
//     end = numLEDs / 2; // Left half of the strip.
//   }
//   else {
//     start = numLEDs / 2; // Right half.
//   }

//   // Set the indicator color (amber/yellow in this example) to override other effects.
//   for (int i = start; i < end; i++) {
//     buffer[i].r = 255;
//     buffer[i].g = 150;
//     buffer[i].b = 0;
//   }
// }
