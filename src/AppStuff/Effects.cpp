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

  brakeEffect = new BrakeLightEffect(8, false);
  reverseLightEffect = new ReverseLightEffect(6, false);
  headlightEffect = new HeadlightEffect(4, false);

  taillightEffect = new TaillightEffect(4, false);

  rgbEffect = new RGBEffect(5, false);
  nightriderEffect = new NightRiderEffect(5, false);

  policeEffect = new PoliceEffect(2, false);
  pulseWaveEffect = new PulseWaveEffect(2, false);
  auroraEffect = new AuroraEffect(2, false);

  // Add effects to the LED manager.

  auto headlightStrip = ledManager->getStrip(LEDStripType::HEADLIGHT);
  auto taillightStrip = ledManager->getStrip(LEDStripType::TAILLIGHT);

  if (headlightStrip)
  {
    headlightStrip->addEffect(leftIndicatorEffect);
    headlightStrip->addEffect(rightIndicatorEffect);

    headlightStrip->addEffect(headlightEffect);

    headlightStrip->addEffect(rgbEffect);
    headlightStrip->addEffect(nightriderEffect);
    headlightStrip->addEffect(policeEffect);
    headlightStrip->addEffect(pulseWaveEffect);
    headlightStrip->addEffect(auroraEffect);
  }

  if (taillightStrip)
  {
    taillightStrip->addEffect(leftIndicatorEffect);
    taillightStrip->addEffect(rightIndicatorEffect);

    taillightStrip->addEffect(taillightEffect);

    taillightStrip->addEffect(brakeEffect);
    taillightStrip->addEffect(reverseLightEffect);

    taillightStrip->addEffect(rgbEffect);
    taillightStrip->addEffect(policeEffect);
    taillightStrip->addEffect(pulseWaveEffect);
    taillightStrip->addEffect(auroraEffect);
  }
}