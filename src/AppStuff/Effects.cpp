#include "Application.h"
#include "IO/LED/LEDStripManager.h"

void Application::setupEffects()
{
  LEDStripManager *ledManager = LEDStripManager::getInstance();

  // Set each effect's LED manager pointer.
  leftIndicatorEffect = new IndicatorEffect(IndicatorEffect::LEFT,
                                            10, true);
  rightIndicatorEffect = new IndicatorEffect(IndicatorEffect::RIGHT,
                                             10, true);
  leftIndicatorEffect->setOtherIndicator(rightIndicatorEffect);
  rightIndicatorEffect->setOtherIndicator(leftIndicatorEffect);

  rgbEffect = new RGBEffect(2, false);
  policeEffect = new PoliceEffect(2, false);
  nightriderEffect = new NightRiderEffect(2, false);
  taillightStartupEffect = new TaillightStartupEffect(4, false);
  headlightStartupEffect = new HeadlightStartupEffect(4, false);

  headlightEffect = new HeadlightEffect(7, false);

  brakeEffect = new BrakeLightEffect(8, false);
  reverseLightEffect = new ReverseLightEffect(6, false);

  // Initialize our new effects
  pulseWaveEffect = new PulseWaveEffect(2, false);
  auroraEffect = new AuroraEffect(2, false);

  // Add effects to the LED manager.

  auto headlightStrip = ledManager->getStrip(LEDStripType::HEADLIGHT);
  auto taillightStrip = ledManager->getStrip(LEDStripType::TAILLIGHT);

  if (headlightStrip)
  {
    headlightStrip->addEffect(rgbEffect);
    headlightStrip->addEffect(nightriderEffect);
    headlightStrip->addEffect(headlightStartupEffect);

    headlightStrip->addEffect(leftIndicatorEffect);
    headlightStrip->addEffect(rightIndicatorEffect);

    headlightStrip->addEffect(headlightEffect);
    headlightStrip->addEffect(policeEffect);

    // Add our new effects to the headlights
    headlightStrip->addEffect(pulseWaveEffect);
    headlightStrip->addEffect(auroraEffect);
  }

  if (taillightStrip)
  {
    taillightStrip->addEffect(leftIndicatorEffect);
    taillightStrip->addEffect(rightIndicatorEffect);
    taillightStrip->addEffect(rgbEffect);
    taillightStrip->addEffect(taillightStartupEffect);
    taillightStrip->addEffect(policeEffect);

#ifdef ENABLE_TAILLIGHTS
    taillightStrip->addEffect(brakeEffect);
    taillightStrip->addEffect(reverseLightEffect);
#endif

    taillightStrip->addEffect(pulseWaveEffect);
    taillightStrip->addEffect(auroraEffect);
  }
}