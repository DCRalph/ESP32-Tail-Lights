// application.cpp

#include "Application.h"
#include "config.h"      // Contains NUM_LEDS, LEDS_PIN, etc.
#include "IO/Wireless.h" // Your wireless library
#include "FastLED.h"

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
  brake = &input2;
  leftIndicator = &input3;
  rightIndicator = &input4;
  reverse = &input5;
#endif

  // Set default states.
  accOnState = false;
  brakeState = false;
  leftIndicatorState = false;
  rightIndicatorState = false;
  reverseState = false;

  lastAccOn = 0;
}

/*
 * Application destructor. Clean up dynamic allocations.
 * This is not necessary for this project, but is good practice.
 */
Application::~Application()
{
  delete brakeEffect;
  delete leftIndicatorEffect;
  delete rightIndicatorEffect;
  delete reverseLightEffect;
  delete rgbEffect;
  delete nightriderEffect;
  delete startupEffect;

  delete unlockSequence;
  delete lockSequence;
  delete RGBFlickSequence;
  delete nightRiderFlickSequence;

  delete ledManager;
  ledManager = nullptr;
}

/*
 * Begin: Initializes the LED strip, effects, wireless, etc.
 */
void Application::begin()
{
  FastLED.addLeds<WS2812, LEDS_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(255);

  // Flash a test LED to indicate startup.
  leds[0] = CRGB(255, 0, 0);
  FastLED.show();
  delay(500);
  leds[0] = CRGB(0, 0, 0);
  FastLED.show();

  mode = static_cast<ApplicationMode>(preferences.getUInt("mode", 0));

  // Create and assign the custom LED manager.
  ledManager = new LEDManager(NUM_LEDS);
  ledManager->setFPS(100);
  ledManager->setDrawFunction(drawLEDs);

  // Set each effect's LED manager pointer.
  brakeEffect = new BrakeLightEffect(ledManager, 8, false);
  leftIndicatorEffect = new IndicatorEffect(ledManager, IndicatorEffect::LEFT,
                                            10, true);
  rightIndicatorEffect = new IndicatorEffect(ledManager, IndicatorEffect::RIGHT,
                                             10, true);
  reverseLightEffect = new ReverseLightEffect(ledManager, 6, false);
  rgbEffect = new RGBEffect(ledManager, 2, false);
  nightriderEffect = new NightRiderEffect(ledManager, 2, false);
  startupEffect = new StartupEffect(ledManager, 4, false);

  leftIndicatorEffect->setOtherIndicator(rightIndicatorEffect);
  rightIndicatorEffect->setOtherIndicator(leftIndicatorEffect);

  // Add effects to the LED manager.
  ledManager->addEffect(brakeEffect);
  ledManager->addEffect(leftIndicatorEffect);
  ledManager->addEffect(rightIndicatorEffect);
  ledManager->addEffect(reverseLightEffect);
  ledManager->addEffect(rgbEffect);
  ledManager->addEffect(nightriderEffect);
  ledManager->addEffect(startupEffect);

  // Sequences
  unlockSequence = new BothIndicatorsSequence(1);
  lockSequence = new BothIndicatorsSequence(3);
  RGBFlickSequence = new IndicatorFlickSequence(IndicatorSide::LEFT_SIDE);
  nightRiderFlickSequence = new IndicatorFlickSequence(IndicatorSide::RIGHT_SIDE);
  brakeTapSequence3 = new BrakeTapSequence(3);

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

  brakeTapSequence3->setActive(true);
  brakeTapSequence3->setCallback([this]()
                                 {
                                   // diable all special effects
                                   rgbEffect->setActive(false);
                                   nightriderEffect->setActive(false);

                                   enableNormalMode();
                                   //
                                 });

  wireless.addOnReceiveFor(0xe0, [&](fullPacket *fp) // ping cmd
                           {
                             lastRemotePing = millis();

                             data_packet pTX;
                             pTX.type = 0xe0;
                             pTX.len = 8;
                             // sent current mode as uint8_t
                             //  pTX.data[0] = static_cast<uint8_t>(mode);
                             switch (mode)
                             {
                             case ApplicationMode::NORMAL:
                               pTX.data[0] = 0;
                               break;
                             case ApplicationMode::TEST:
                               pTX.data[0] = 1;
                               break;
                             case ApplicationMode::REMOTE:
                               pTX.data[0] = 2;
                               break;
                             case ApplicationMode::OFF:
                               pTX.data[0] = 3;
                               break;
                             default:
                               break;
                             }

                             // sent current effects as uint8_t
                             pTX.data[1] = brakeEffect->isActive();
                             pTX.data[2] = leftIndicatorEffect->isActive();
                             pTX.data[3] = rightIndicatorEffect->isActive();
                             pTX.data[4] = reverseLightEffect->isActive();
                             pTX.data[5] = startupEffect->isActive();
                             pTX.data[6] = rgbEffect->isActive();
                             pTX.data[7] = nightriderEffect->isActive();

                             wireless.send(&pTX, fp->mac);
                             //
                           });

  wireless.addOnReceiveFor(0xe1, [&](fullPacket *fp) // set mode cmd
                           {
                             lastRemotePing = millis();

                             uint8_t *data = fp->p.data;
                             uint8_t rxMode = data[0];

                             switch (rxMode)
                             {
                             case 0:
                               enableNormalMode();
                               break;
                             case 1:
                               enableTestMode();
                               break;
                             case 2:
                               enableRemoteMode();
                               break;
                             case 3:
                               enableOffMode();
                               break;
                             default:
                               break;
                             }

                             data_packet pTX;
                             pTX.type = 0xe1;
                             pTX.len = 1;
                             // sent current mode as uint8_t
                             pTX.data[0] = static_cast<uint8_t>(mode);

                             wireless.send(&pTX, fp->mac);
                             //
                           });

  wireless.addOnReceiveFor(0xe2, [&](fullPacket *fp) // set effects cmd
                           {
                             lastRemotePing = millis();

                             uint8_t *data = fp->p.data;

                             for (int i = 0; i < 7; i++)
                             {
                               bool firstBit = data[i] & 0x01;
                               bool secondBit = data[i] & 0x02;
                               bool thirdBit = data[i] & 0x04;

                               switch (i)
                               {
                               case 0:
                                 brakeEffect->setActive(firstBit);
                                 brakeEffect->setIsReversing(secondBit);
                                 break;
                               case 1:
                                 leftIndicatorEffect->setActive(firstBit);
                                 break;
                               case 2:
                                 rightIndicatorEffect->setActive(firstBit);
                                 break;
                               case 3:
                                 reverseLightEffect->setActive(firstBit);
                                 break;
                               case 4:
                                 startupEffect->setActive(firstBit);
                                 break;
                               case 5:
                                 rgbEffect->setActive(firstBit);
                                 break;
                               case 6:
                                 nightriderEffect->setActive(firstBit);
                                 break;

                               default:
                                 break;
                               }
                             }

                             //
                           });

#ifndef ENABLE_HV_INPUTS
  enableTestMode();
  rgbEffect->setActive(true);
#endif
}

static float accVolt = 0;
static float brakeVolt = 0;
static float leftVolt = 0;
static float rightVolt = 0;
static float reverseVolt = 0;

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
  brakeVolt = (brakeVolt * smoothFactor) +
              (((float)brake->analogRead() / ADC_MAX *
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
  reverseVolt = (reverseVolt * smoothFactor) +
                (((float)reverse->analogRead() / ADC_MAX *
                  ADC_REF_VOLTAGE * DIVIDER_FACTOR) *
                 (1 - smoothFactor));

  // Determine digital state for each input.
  accOnState = accVolt > VOLTAGE_THRESHOLD;
  brakeState = brakeVolt > VOLTAGE_THRESHOLD;
  leftIndicatorState = leftVolt > VOLTAGE_THRESHOLD;
  rightIndicatorState = rightVolt > VOLTAGE_THRESHOLD;
  reverseState = reverseVolt > VOLTAGE_THRESHOLD;

#else

  accOnState = false;
  brakeState = false;
  leftIndicatorState = false;
  rightIndicatorState = false;
  reverseState = false;
#endif
}

/*
 * update():
 * Main loop update.
 */
void Application::update()
{
  // Update input states.
  updateInputs();

  brakeTapSequence3->setInput(brakeState);
  brakeTapSequence3->loop();

  if (lastRemotePing != 0 && millis() - lastRemotePing > 2000)
  {
    if (mode == ApplicationMode::TEST || mode == ApplicationMode::REMOTE)
    {
      lastRemotePing = 0;
      mode = ApplicationMode::NORMAL;
      // disable all effects
      brakeEffect->setActive(false);
      leftIndicatorEffect->setActive(false);
      rightIndicatorEffect->setActive(false);
      reverseLightEffect->setActive(false);
      rgbEffect->setActive(false);
      nightriderEffect->setActive(false);
      startupEffect->setActive(false);
    }
  }

  switch (mode)
  {
  case ApplicationMode::NORMAL:
    handleNormalEffects();
    break;

  case ApplicationMode::TEST:
  {
    // reverseLightEffect->setActive(true);
    // brakeEffect->setIsReversing(true);

    // brakeEffect->setActive(io0.read());
    // leftIndicatorEffect->setActive(io0.read());
    // rightIndicatorEffect->setActive(io0.read());
    // reverseLightEffect->setActive(io0.read());
    // rgbEffect->setActive(io0.read());
    startupEffect->setActive(io0.read());
  }
  break;

  case ApplicationMode::REMOTE:
    handleRemoteEffects();
    break;

  case ApplicationMode::OFF:
  {
    // turn off all effects
    brakeEffect->setActive(false);
    leftIndicatorEffect->setActive(false);
    rightIndicatorEffect->setActive(false);
    reverseLightEffect->setActive(false);
    rgbEffect->setActive(false);
    nightriderEffect->setActive(false);
    startupEffect->setActive(false);
  }
  break;
  }

  // Update and draw LED effects.
  ledManager->updateEffects();
  ledManager->draw();
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
    brakeEffect->setActive(false);
    leftIndicatorEffect->setActive(false);
    rightIndicatorEffect->setActive(false);
    reverseLightEffect->setActive(false);
    rgbEffect->setActive(false);
    nightriderEffect->setActive(false);
    startupEffect->setActive(false);
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
    brakeEffect->setActive(false);
    leftIndicatorEffect->setActive(false);
    rightIndicatorEffect->setActive(false);
    reverseLightEffect->setActive(false);
  }
  else
  {
    lastAccOn = currentTime;

    // When ACC is on, ensure the startup effect is turned off and reset flash tracking.
    startupEffect->setActive(false);

    // And process the other effects normally.
    brakeEffect->setActive(brakeState);
    brakeEffect->setIsReversing(reverseState || reverseLightEffect->isAnimating());

    leftIndicatorEffect->setActive(leftIndicatorState);
    rightIndicatorEffect->setActive(rightIndicatorState);
    reverseLightEffect->setActive(reverseState);
  }
}

void Application::handleRemoteEffects()
{
}

void Application::drawLEDs()
{
  auto buf = Application::getInstance()->ledManager->getBuffer();
  uint16_t numLEDs = Application::getInstance()->ledManager->getNumLEDs();
  for (int i = 0; i < numLEDs; i++)
  {
    Application::getInstance()->leds[i] =
        CRGB(buf[i].r, buf[i].g, buf[i].b);
  }
  FastLED.show();
}
