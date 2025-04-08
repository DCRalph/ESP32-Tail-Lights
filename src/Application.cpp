// application.cpp

#include "Application.h"
#include "config.h" 
#include "IO/Wireless.h" // Your wireless library
#include "FastLED.h"
#include "IO/LED/LEDStripManager.h"

//----------------------------------------------------------------------------

Application *Application::getInstance()
{
  static Application instance;
  return &instance;
}

//----------------------------------------------------------------------------
/*
 * Application constructor. Initialize pointers and set defaults.
 */
Application::Application()
{
// Initialize input pointers.
#ifdef ENABLE_HV_INPUTS
  accOn = &input1;
  leftIndicator = &input2;
  rightIndicator = &input3;
  externalControl = &input4;

#ifdef ENABLE_HEADLIGHTS
  highBeam = &input5;
  lowBeam = &input6;
#endif

#ifdef ENABLE_TAILLIGHTS
  brake = &input5;
  reverse = &input6;
#endif
#endif

  // Set default states.
  accOnState = false;
  leftIndicatorState = false;
  rightIndicatorState = false;
  externalControlState = false;

#ifdef ENABLE_HEADLIGHTS
  highBeamState = false;
  lowBeamState = false;
#endif

#ifdef ENABLE_TAILLIGHTS
  brakeState = false;
  reverseState = false;
#endif

  lastAccOn = 0;
}

/*
 * Application destructor. Clean up dynamic allocations.
 * This is not necessary for this project, but is good practice.
 */
Application::~Application()
{
  leftIndicatorEffect = nullptr;
  rightIndicatorEffect = nullptr;
  rgbEffect = nullptr;
  nightriderEffect = nullptr;
  startupEffect = nullptr;


  delete brakeEffect;
  delete reverseLightEffect;

  delete headlightEffect;


  delete leftIndicatorEffect;
  delete rightIndicatorEffect;
  delete rgbEffect;
  delete nightriderEffect;
  delete startupEffect;

  delete unlockSequence;
  delete lockSequence;
  delete RGBFlickSequence;
  delete nightRiderFlickSequence;

#ifdef ENABLE_TAILLIGHTS
  delete brakeTapSequence3;
#endif

#ifdef ENABLE_HEADLIGHTS
  // No headlight-specific sequences to delete yet
#endif
}

/*
 * Begin: Initializes the LED strip, effects, wireless, etc.
 */
void Application::begin()
{

  mode = static_cast<ApplicationMode>(preferences.getUInt("mode", 0));

  LEDStripManager *ledManager = LEDStripManager::getInstance();
  ledManager->begin();

#ifdef ENABLE_HEADLIGHTS
  LEDStripConfig headlights(
      LEDStripType::HEADLIGHT,
      new LEDStrip(HEADLIGHT_LED_COUNT),
      "Headlights");
  FastLED.addLeds<WS2812, HEADLIGHT_LED_PIN, GRB>(headlights.strip->getFastLEDBuffer(), HEADLIGHT_LED_COUNT);
  ledManager->addLEDStrip(headlights);

  // Flash a test LED to indicate startup.
  headlights.strip->getFastLEDBuffer()[0] = CRGB(255, 0, 0);
  FastLED.show();
  delay(500);
  headlights.strip->getFastLEDBuffer()[0] = CRGB(0, 0, 0);
  FastLED.show();

#endif

#ifdef ENABLE_TAILLIGHTS
  LEDStripConfig taillights(
      LEDStripType::TAILLIGHT,
      new LEDStrip(TAILLIGHT_LED_COUNT),
      "Taillights");
  FastLED.addLeds<WS2812, TAILLIGHT_LED_PIN, GRB>(taillights.strip->getFastLEDBuffer(), TAILLIGHT_LED_COUNT);
  ledManager->addLEDStrip(taillights);

  taillights.strip->getFastLEDBuffer()[0] = CRGB(255, 0, 0);
  FastLED.show();
  delay(500);
  taillights.strip->getFastLEDBuffer()[0] = CRGB(0, 0, 0);
  FastLED.show();
#endif

#ifdef ENABLE_UNDERGLOW
  LEDStripConfig underglow(
      LEDStripType::UNDERGLOW,
      new LEDStrip(UNDERGLOW_LED_COUNT),
      "Underglow");
  FastLED.addLeds<WS2812, UNDERGLOW_LED_PIN, GRB>(underglow.strip->getFastLEDBuffer(), UNDERGLOW_LED_COUNT);
  ledManager->addLEDStrip(underglow);
#endif

#ifdef ENABLE_INTERIOR
  LEDStripConfig interior(
      LEDStripType::INTERIOR,
      new LEDStrip(INTERIOR_LED_COUNT),
      "Interior");
  FastLED.addLeds<WS2812, INTERIOR_LED_PIN, GRB>(interior.strip->getFastLEDBuffer(), INTERIOR_LED_COUNT);
  ledManager->addLEDStrip(interior);
#endif

  // Set each effect's LED manager pointer.
  leftIndicatorEffect = new IndicatorEffect(IndicatorEffect::LEFT,
                                            10, true);
  rightIndicatorEffect = new IndicatorEffect(IndicatorEffect::RIGHT,
                                             10, true);
  rgbEffect = new RGBEffect(2, false);
  nightriderEffect = new NightRiderEffect(2, false);
  startupEffect = new StartupEffect(4, false);

  headlightEffect = new HeadlightEffect(7, false);

  brakeEffect = new BrakeLightEffect(8, false);
  reverseLightEffect = new ReverseLightEffect(6, false);


  leftIndicatorEffect->setOtherIndicator(rightIndicatorEffect);
  rightIndicatorEffect->setOtherIndicator(leftIndicatorEffect);

  // Add effects to the LED manager.

  auto headlightStrip = ledManager->getStrip(LEDStripType::HEADLIGHT);
  auto taillightStrip = ledManager->getStrip(LEDStripType::TAILLIGHT);

  if (headlightStrip)
  {
    headlightStrip->addEffect(rgbEffect);
    headlightStrip->addEffect(nightriderEffect);
    headlightStrip->addEffect(startupEffect);

    headlightStrip->addEffect(leftIndicatorEffect);
    headlightStrip->addEffect(rightIndicatorEffect);

    headlightStrip->addEffect(headlightEffect);
  }

  if (taillightStrip)
  {
    taillightStrip->addEffect(leftIndicatorEffect);
    taillightStrip->addEffect(rightIndicatorEffect);
    taillightStrip->addEffect(rgbEffect);
    taillightStrip->addEffect(startupEffect);

#ifdef ENABLE_TAILLIGHTS
    taillightStrip->addEffect(brakeEffect);
    taillightStrip->addEffect(reverseLightEffect);
#endif
  }

  // Sequences
  unlockSequence = new BothIndicatorsSequence(1);
  lockSequence = new BothIndicatorsSequence(3);
  RGBFlickSequence = new IndicatorFlickSequence(IndicatorSide::LEFT_SIDE);
  nightRiderFlickSequence = new IndicatorFlickSequence(IndicatorSide::RIGHT_SIDE);

#ifdef ENABLE_TAILLIGHTS
  brakeTapSequence3 = new BrakeTapSequence(3);
#endif

  unlockSequence->setActive(true);

  unlockSequence->setCallback([this]()
                              {
                                startupEffect->setActive(true);
                                unlockSequence->setActive(false);
                                //
                              });

  lockSequence->setActive(true);

  lockSequence->setCallback([this]()
                            {
                              startupEffect->setActive(false);
                              unlockSequence->setActive(true);
                              //
                            });

  RGBFlickSequence->setActive(true);
  RGBFlickSequence->setCallback([this]()
                                {
                                  rgbEffect->setActive(!rgbEffect->isActive());
                                  nightRiderFlickSequence->reset();

                                  nightriderEffect->setActive(false);
                                  //
                                });

  nightRiderFlickSequence->setActive(true);
  nightRiderFlickSequence->setCallback([this]()
                                       {
                                         nightriderEffect->setActive(!nightriderEffect->isActive());
                                         RGBFlickSequence->reset();

                                         rgbEffect->setActive(false);
                                         //
                                       });

#ifdef ENABLE_TAILLIGHTS
  brakeTapSequence3->setActive(true);
  brakeTapSequence3->setCallback([this]()
                                 {
                                   // diable all special effects
                                   rgbEffect->setActive(false);
                                   nightriderEffect->setActive(false);

                                   enableNormalMode();
                                   //
                                 });
#endif

  // Setup wireless communication handlers
  setupWireless();

#ifndef ENABLE_HV_INPUTS
  enableTestMode();
  rgbEffect->setActive(true);
#endif

  // set brightness to 100
  ledManager->setBrightness(100);

  // clear all buffers and fastled.show
  FastLED.clear();
  FastLED.show();
}

static float accVolt = 0;
static float leftVolt = 0;
static float rightVolt = 0;
static float externaCtrllVolt = 0;

#ifdef ENABLE_HEADLIGHTS
static float highBeamVolt = 0;
static float lowBeamVolt = 0;
#endif

#ifdef ENABLE_TAILLIGHTS
static float reverseVolt = 0;
static float brakeVolt = 0;
#endif

void Application::updateInputs()
{
  // ADC conversion constants and threshold for digital "on".
  const float ADC_MAX = 8192;
  const float ADC_REF_VOLTAGE = 3.3;
  const float DIVIDER_FACTOR = 10.0;
  const float VOLTAGE_THRESHOLD = 3;

  float smoothFactor = 0.5f;

// Update filtered voltages using a simple low-pass filter.
#ifdef ENABLE_HV_INPUTS
  accVolt = (accVolt * smoothFactor) +
            (((float)accOn->analogRead() / ADC_MAX *
              ADC_REF_VOLTAGE * DIVIDER_FACTOR) *
             (1 - smoothFactor));
  leftVolt = (leftVolt * smoothFactor) +
             (((float)leftIndicator->analogRead() / ADC_MAX *
               ADC_REF_VOLTAGE * DIVIDER_FACTOR) *
              (1 - smoothFactor));
  rightVolt = (rightVolt * smoothFactor) +
              (((float)rightIndicator->analogRead() / ADC_MAX *
                ADC_REF_VOLTAGE * DIVIDER_FACTOR) *
               (1 - smoothFactor));
  externaCtrllVolt = (externaCtrllVolt * smoothFactor) +
                     (((float)externalControl->analogRead() / ADC_MAX *
                       ADC_REF_VOLTAGE * DIVIDER_FACTOR) *
                      (1 - smoothFactor));

#ifdef ENABLE_HEADLIGHTS
  highBeamVolt = (highBeamVolt * smoothFactor) +
                 (((float)highBeam->analogRead() / ADC_MAX *
                   ADC_REF_VOLTAGE * DIVIDER_FACTOR) *
                  (1 - smoothFactor));
  lowBeamVolt = (lowBeamVolt * smoothFactor) +
                (((float)lowBeam->analogRead() / ADC_MAX *
                  ADC_REF_VOLTAGE * DIVIDER_FACTOR) *
                 (1 - smoothFactor));
#endif

#ifdef ENABLE_TAILLIGHTS
  brakeVolt = (brakeVolt * smoothFactor) +
              (((float)brake->analogRead() / ADC_MAX *
                ADC_REF_VOLTAGE * DIVIDER_FACTOR) *
               (1 - smoothFactor));
  reverseVolt = (reverseVolt * smoothFactor) +
                (((float)reverse->analogRead() / ADC_MAX *
                  ADC_REF_VOLTAGE * DIVIDER_FACTOR) *
                 (1 - smoothFactor));
#endif

  // Determine digital state for each input.
  accOnState = accVolt > VOLTAGE_THRESHOLD;
  leftIndicatorState = leftVolt > VOLTAGE_THRESHOLD;
  rightIndicatorState = rightVolt > VOLTAGE_THRESHOLD;
  externalControlState = externaCtrllVolt > VOLTAGE_THRESHOLD;

#ifdef ENABLE_HEADLIGHTS
  highBeamState = highBeamVolt > VOLTAGE_THRESHOLD;
  lowBeamState = lowBeamVolt > VOLTAGE_THRESHOLD;
#endif

#ifdef ENABLE_TAILLIGHTS
  brakeState = brakeVolt > VOLTAGE_THRESHOLD;
  reverseState = reverseVolt > VOLTAGE_THRESHOLD;
#endif

#else

  accOnState = false;
  leftIndicatorState = false;
  rightIndicatorState = false;
  externalControlState = false;

#ifdef ENABLE_HEADLIGHTS
  highBeamState = false;
  lowBeamState = false;
#endif

#ifdef ENABLE_TAILLIGHTS
  brakeState = false;
  reverseState = false;
#endif
#endif
}

/*
 * update():
 * Main loop update.
 */
void Application::loop()
{
  // Update input states.
  updateInputs();

  if (lastRemotePing != 0 && millis() - lastRemotePing > 2000)
  {
    if (mode == ApplicationMode::TEST || mode == ApplicationMode::REMOTE)
    {
      lastRemotePing = 0;
      mode = ApplicationMode::NORMAL;
      // disable all effects
      leftIndicatorEffect->setActive(false);
      rightIndicatorEffect->setActive(false);
      rgbEffect->setActive(false);
      nightriderEffect->setActive(false);
      startupEffect->setActive(false);

      headlightEffect->setActive(false);
      headlightEffect->setSplit(false);
      headlightEffect->setColor(false, false, false);

      brakeEffect->setActive(false);
      brakeEffect->setIsReversing(false);
      reverseLightEffect->setActive(false);

    }
  }

#ifdef ENABLE_TAILLIGHTS
  brakeTapSequence3->setInput(brakeState);
  brakeTapSequence3->loop();
#endif

  switch (mode)
  {
  case ApplicationMode::NORMAL:
    handleNormalEffects();
    break;

  case ApplicationMode::TEST:
  {
    handleTestEffects();
  }
  break;

  case ApplicationMode::REMOTE:
    handleRemoteEffects();
    break;

  case ApplicationMode::OFF:
  {
    // turn off all effects
    leftIndicatorEffect->setActive(false);
    rightIndicatorEffect->setActive(false);
    rgbEffect->setActive(false);
    nightriderEffect->setActive(false);
    startupEffect->setActive(false);

    headlightEffect->setActive(false);
    headlightEffect->setSplit(false);
    headlightEffect->setColor(false, false, false);

    brakeEffect->setActive(false);
    brakeEffect->setIsReversing(false);
    reverseLightEffect->setActive(false);

  }
  break;
  }

  // Update and draw LED effects.
  LEDStripManager::getInstance()->updateEffects();
  LEDStripManager::getInstance()->draw();
}

/*
 * setTestMode():
 * Allows external code to enable or disable test mode.
 */
void Application::enableNormalMode()
{
  mode = ApplicationMode::NORMAL;
  preferences.putUInt("mode", static_cast<uint8_t>(mode));
}

void Application::enableTestMode()
{
  mode = ApplicationMode::TEST;
  preferences.putUInt("mode", static_cast<uint8_t>(mode));
}

void Application::enableRemoteMode()
{
  mode = ApplicationMode::REMOTE;
  preferences.putUInt("mode", static_cast<uint8_t>(mode));
}

void Application::enableOffMode()
{
  mode = ApplicationMode::OFF;
  preferences.putUInt("mode", static_cast<uint8_t>(mode));
}

void Application::handleNormalEffects()
{
  unsigned long currentTime = millis();

  unlockSequence->setInputs(accOnState, leftIndicatorState, rightIndicatorState);
  lockSequence->setInputs(accOnState, leftIndicatorState, rightIndicatorState);
  RGBFlickSequence->setInputs(accOnState, leftIndicatorState, rightIndicatorState);
  nightRiderFlickSequence->setInputs(accOnState, leftIndicatorState, rightIndicatorState);

  unlockSequence->loop();
  lockSequence->loop();
  RGBFlickSequence->loop();
  nightRiderFlickSequence->loop();

  if (lastAccOn != 0 && currentTime - lastAccOn > 1 * 60 * 1000)
  {
    lastAccOn = 0;
    unlockSequence->setActive(true);

    // turn off all effects
    leftIndicatorEffect->setActive(false);
    rightIndicatorEffect->setActive(false);
    rgbEffect->setActive(false);
    nightriderEffect->setActive(false);
    startupEffect->setActive(false);


    headlightEffect->setActive(false);
    headlightEffect->setSplit(false);
    headlightEffect->setColor(false, false, false);


    brakeEffect->setActive(false);
    brakeEffect->setIsReversing(false);
    reverseLightEffect->setActive(false);
  }

  if (lastAccOnState != accOnState)
  {
    lastAccOnState = accOnState;

    if (accOnState == false)
    {
      startupEffect->setActive(true);
    }
  }

  if (!accOnState)
  {
    // Since ACC is off, disable the other effects.
    leftIndicatorEffect->setActive(false);
    rightIndicatorEffect->setActive(false);


    headlightEffect->setActive(false);
    headlightEffect->setSplit(false);
    headlightEffect->setColor(false, false, false);



    brakeEffect->setActive(false);
    brakeEffect->setIsReversing(false);
    reverseLightEffect->setActive(false);

  }
  else
  {
    lastAccOn = currentTime;

    // When ACC is on, ensure the startup effect is turned off and reset flash tracking.
    startupEffect->setActive(false);

    // And process the other effects normally.
    leftIndicatorEffect->setActive(leftIndicatorState);
    rightIndicatorEffect->setActive(rightIndicatorState);

#ifdef ENABLE_HEADLIGHTS
    headlightEffect->setActive(highBeamState);
#endif

#ifdef ENABLE_TAILLIGHTS
    brakeEffect->setActive(brakeState);
    brakeEffect->setIsReversing(reverseState || reverseLightEffect->isAnimating());
    reverseLightEffect->setActive(reverseState);
#endif
  }
}

void Application::handleTestEffects()
{
  // Test Mode - Add headlight testing

  reverseLightEffect->setActive(true);
  headlightEffect->setActive(true);
  headlightEffect->setSplit(false);


  // brakeEffect->setIsReversing(true);

  // brakeEffect->setActive(io0.read());
  // leftIndicatorEffect->setActive(io0.read());
  // rightIndicatorEffect->setActive(io0.read());
  // reverseLightEffect->setActive(io0.read());
  // rgbEffect->setActive(io0.read());
  // startupEffect->setActive(io0.read());
}

void Application::handleRemoteEffects()
{
}
