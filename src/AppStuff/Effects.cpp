#include "Application.h"
#include "IO/LED/LEDStripManager.h"
#include "IO/LED/Effects/CommitEffect.h"

void Application::setupEffects()
{
  LEDStripManager *ledManager = LEDStripManager::getInstance();

  // Core effects
  leftIndicatorEffect = new IndicatorEffect(IndicatorEffect::LEFT,
                                            10, true);
  rightIndicatorEffect = new IndicatorEffect(IndicatorEffect::RIGHT,
                                             10, true);
  leftIndicatorEffect->setOtherIndicator(rightIndicatorEffect);
  rightIndicatorEffect->setOtherIndicator(leftIndicatorEffect);

  brakeEffect = new BrakeLightEffect(9, true);
  reverseLightEffect = new ReverseLightEffect(8, true);

  headlightEffect = new HeadlightEffect(4, false);
  taillightEffect = new TaillightEffect(4, false);

  // Other effects
  rgbEffect = new RGBEffect(5, false);
  nightriderEffect = new NightRiderEffect(5, false);
  pulseWaveEffect = new PulseWaveEffect(5, false);
  auroraEffect = new AuroraEffect(5, false);
  solidColorEffect = new SolidColorEffect(5, false);
  colorFadeEffect = new ColorFadeEffect(5, false);
  policeEffect = new PoliceEffect(4, false);
  commitEffect = new CommitEffect(5, false);
  serviceLightsEffect = new ServiceLightsEffect(5, false);

  // Add effects to the LED manager.
  auto headlightStrip = ledManager->getStrip(LEDStripType::HEADLIGHT);
  auto taillightStrip = ledManager->getStrip(LEDStripType::TAILLIGHT);
  auto underglowStrip = ledManager->getStrip(LEDStripType::UNDERGLOW);
  auto interiorStrip = ledManager->getStrip(LEDStripType::INTERIOR);

  if (headlightStrip)
  {
    headlightStrip->addEffect(leftIndicatorEffect);
    headlightStrip->addEffect(rightIndicatorEffect);

    headlightStrip->addEffect(headlightEffect);

    headlightStrip->addEffect(rgbEffect);
    headlightStrip->addEffect(nightriderEffect);
    headlightStrip->addEffect(policeEffect);
    // headlightStrip->addEffect(pulseWaveEffect);
    // headlightStrip->addEffect(auroraEffect);
    headlightStrip->addEffect(solidColorEffect);
    headlightStrip->addEffect(colorFadeEffect);
    headlightStrip->addEffect(commitEffect);
    headlightStrip->addEffect(serviceLightsEffect);
  }

  if (taillightStrip)
  {
    taillightStrip->addEffect(leftIndicatorEffect);
    taillightStrip->addEffect(rightIndicatorEffect);

    taillightStrip->addEffect(taillightEffect);

    taillightStrip->addEffect(brakeEffect);
    taillightStrip->addEffect(reverseLightEffect);

    taillightStrip->addEffect(rgbEffect);
    taillightStrip->addEffect(nightriderEffect);
    taillightStrip->addEffect(policeEffect);
    // taillightStrip->addEffect(pulseWaveEffect);
    // taillightStrip->addEffect(auroraEffect);
    taillightStrip->addEffect(solidColorEffect);
    taillightStrip->addEffect(colorFadeEffect);
    taillightStrip->addEffect(commitEffect);
    taillightStrip->addEffect(serviceLightsEffect);
  }

  if (underglowStrip)
  {
    LEDSegment *segmentL = underglowStrip->getSegment(1);
    LEDSegment *segmentF = underglowStrip->getSegment(2);
    LEDSegment *segmentR = underglowStrip->getSegment(3);

    underglowStrip->addEffect(rgbEffect);
    underglowStrip->addEffect(nightriderEffect);
    underglowStrip->addEffect(policeEffect);
    underglowStrip->addEffect(pulseWaveEffect);
    underglowStrip->addEffect(auroraEffect);
    underglowStrip->addEffect(solidColorEffect);
    underglowStrip->addEffect(colorFadeEffect);
    underglowStrip->addEffect(commitEffect);
    underglowStrip->addEffect(serviceLightsEffect);
  }

  LEDEffect::disableAllEffects();
}