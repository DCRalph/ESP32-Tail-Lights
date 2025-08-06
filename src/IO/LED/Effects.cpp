#include "Effects.h"
#include <Arduino.h> // For millis()

//
// LEDEffect Base Class Implementation
//
LEDEffect::LEDEffect(uint8_t priority, bool transparent)
    : priority(priority), transparent(transparent)
{
    effects.push_back(this);
}

LEDEffect::~LEDEffect() {}

uint8_t LEDEffect::getPriority() const { return priority; }
bool LEDEffect::isTransparent() const { return transparent; }
void LEDEffect::setPriority(uint8_t prio) { priority = prio; }
void LEDEffect::setTransparent(bool transp) { transparent = transp; }

std::vector<LEDEffect *> LEDEffect::effects = {};

std::vector<LEDEffect *> LEDEffect::getEffects() { return effects; }
void LEDEffect::disableAllEffects()
{
    for (auto effect : effects)
    {
        effect->onDisable();
    }
}
