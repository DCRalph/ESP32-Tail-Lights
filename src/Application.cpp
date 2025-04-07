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
  leftIndicator = &input2;
  rightIndicator = &input3;
  externalControl = &input4;

#ifdef HEAD_LIGHTS
  highBeam = &input5;
  lowBeam = &input6;
#endif

#ifdef TAIL_LIGHTS
  brake = &input5;
  reverse = &input6;
#endif
#endif

  // Set default states.
  accOnState = false;
  leftIndicatorState = false;
  rightIndicatorState = false;
  externalControlState = false;

#ifdef HEAD_LIGHTS
  highBeamState = false;
  lowBeamState = false;
#endif

#ifdef TAIL_LIGHTS
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

#ifdef TAIL_LIGHTS
  delete brakeEffect;
  delete reverseLightEffect;
#endif

#ifdef HEAD_LIGHTS
  delete headlightEffect;
#endif

  delete leftIndicatorEffect;
  delete rightIndicatorEffect;
  delete rgbEffect;
  delete nightriderEffect;
  delete startupEffect;

  delete unlockSequence;
  delete lockSequence;
  delete RGBFlickSequence;
  delete nightRiderFlickSequence;

#ifdef TAIL_LIGHTS
  delete brakeTapSequence3;
#endif

#ifdef HEAD_LIGHTS
  // No headlight-specific sequences to delete yet
#endif

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
  ledManager = new LEDStrip(NUM_LEDS);
  ledManager->setFPS(100);
  ledManager->setDrawFunction(drawLEDs);

  // Set each effect's LED manager pointer.
  leftIndicatorEffect = new IndicatorEffect(IndicatorEffect::LEFT,
                                            10, true);
  rightIndicatorEffect = new IndicatorEffect(IndicatorEffect::RIGHT,
                                             10, true);
  rgbEffect = new RGBEffect(2, false);
  nightriderEffect = new NightRiderEffect(2, false);
  startupEffect = new StartupEffect(4, false);

#ifdef HEAD_LIGHTS
  headlightEffect = new HeadlightEffect(7, false);
#endif

#ifdef TAIL_LIGHTS
  brakeEffect = new BrakeLightEffect(8, false);
  reverseLightEffect = new ReverseLightEffect(6, false);
#endif

  leftIndicatorEffect->setOtherIndicator(rightIndicatorEffect);
  rightIndicatorEffect->setOtherIndicator(leftIndicatorEffect);

  // Add effects to the LED manager.
  ledManager->addEffect(leftIndicatorEffect);
  ledManager->addEffect(rightIndicatorEffect);
  ledManager->addEffect(rgbEffect);
  ledManager->addEffect(nightriderEffect);
  ledManager->addEffect(startupEffect);

#ifdef HEAD_LIGHTS
  ledManager->addEffect(headlightEffect);
#endif

#ifdef TAIL_LIGHTS
  ledManager->addEffect(brakeEffect);
  ledManager->addEffect(reverseLightEffect);
#endif

  // Sequences
  unlockSequence = new BothIndicatorsSequence(1);
  lockSequence = new BothIndicatorsSequence(3);
  RGBFlickSequence = new IndicatorFlickSequence(IndicatorSide::LEFT_SIDE);
  nightRiderFlickSequence = new IndicatorFlickSequence(IndicatorSide::RIGHT_SIDE);

#ifdef HEAD_LIGHTS
  // No headlight-specific sequences yet
#endif

#ifdef TAIL_LIGHTS
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

#ifdef HEAD_LIGHTS
  // No headlight-specific sequences yet
#endif

#ifdef TAIL_LIGHTS
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

  wireless.addOnReceiveFor(0xe0, [&](fullPacket *fp) // ping cmd
                           {
                             lastRemotePing = millis();

                             data_packet pTX;
                             memset(&pTX, 0, sizeof(pTX));
                             pTX.type = 0xe0;
                             pTX.len = 9;

                             /*
                              * 0: mode
                              * 1: headlight/taillight mode
                              * 2.0: left indicator
                              * 3.0: right indicator
                              * 4.0: startup
                              * 5.0: rgb
                              * 6.0: nightrider
                              * 7.0: headlight / brake
                              * 7.1: split
                              * 7.2: red
                              * 7.3: green
                              * 7.4: blue
                              * 8.0: brake
                              * 8.1: reverse
                              */

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

#if defined(HEAD_LIGHTS)
                             pTX.data[1] = 1;
#elif defined(TAIL_LIGHTS)
                             pTX.data[1] = 2;
#else
                             pTX.data[1] = 0;
#endif

                             pTX.data[2] = leftIndicatorEffect->isActive();
                             pTX.data[3] = rightIndicatorEffect->isActive();
                             pTX.data[4] = startupEffect->isActive();
                             pTX.data[5] = rgbEffect->isActive();
                             pTX.data[6] = nightriderEffect->isActive();
#ifdef HEAD_LIGHTS
                             bool r, g, b;
                             headlightEffect->getColor(r, g, b);
                             pTX.data[7] = headlightEffect->isActive();
                             pTX.data[7] |= headlightEffect->getSplit() << 1;
                             pTX.data[7] |= r << 2;
                             pTX.data[7] |= g << 3;
                             pTX.data[7] |= b << 4;
#endif

#ifdef TAIL_LIGHTS
                             pTX.data[7] = brakeEffect->isActive();
                             pTX.data[8] = reverseLightEffect->isActive();
                             pTX.data[8] |= brakeEffect->getIsReversing() << 1;
#endif

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
                             memset(&pTX, 0, sizeof(pTX));
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

                             for (int i = 0; i < 9; i++)
                             {
                               bool firstBit = checkBit(data[i], 0);   // active state
                               bool secondBit = checkBit(data[i], 1);  // optional state
                               bool thirdBit = checkBit(data[i], 2);   // optional state
                               bool fourthBit = checkBit(data[i], 3);  // optional state
                               bool fifthBit = checkBit(data[i], 4);   // optional state
                               bool sixthBit = checkBit(data[i], 5);   // optional state
                               bool seventhBit = checkBit(data[i], 6); // optional state
                               bool eighthBit = checkBit(data[i], 7);  // optional state

                               /*
                                * 0.0: left indicator
                                * 1.0: right indicator
                                * 2.0: startup
                                * 3.0: rgb
                                * 4.0: nightrider
                                * 5.0: headlight / brake
                                * 5.1: split
                                * 5.2: red
                                * 5.3: green
                                * 5.4: blue
                                * 6.0: brake
                                * 6.1: reverse
                                */

                               switch (i)
                               {
                               case 0:
                                 leftIndicatorEffect->setActive(firstBit);
                                 break;
                               case 1:
                                 rightIndicatorEffect->setActive(firstBit);
                                 break;
                               case 2:
                                 startupEffect->setActive(firstBit);
                                 break;
                               case 3:
                                 rgbEffect->setActive(firstBit);
                                 break;
                               case 4:
                                 nightriderEffect->setActive(firstBit);
                                 break;

#ifdef HEAD_LIGHTS
                               case 5:
                                 headlightEffect->setActive(firstBit);
                                 headlightEffect->setSplit(secondBit);
                                 headlightEffect->setColor(thirdBit, fourthBit, fifthBit);
                                 break;

#endif

#ifdef TAIL_LIGHTS
                               case 5:
                                 brakeEffect->setActive(firstBit);
                                 brakeEffect->setIsReversing(secondBit);
                                 break;
                               case 6:
                                 reverseLightEffect->setActive(firstBit);
                                 break;
#endif

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
static float leftVolt = 0;
static float rightVolt = 0;
static float externaCtrllVolt = 0;

#ifdef HEAD_LIGHTS
static float highBeamVolt = 0;
static float lowBeamVolt = 0;
#endif

#ifdef TAIL_LIGHTS
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

#ifdef HEAD_LIGHTS
  highBeamVolt = (highBeamVolt * smoothFactor) +
                 (((float)highBeam->analogRead() / ADC_MAX *
                   ADC_REF_VOLTAGE * DIVIDER_FACTOR) *
                  (1 - smoothFactor));
  lowBeamVolt = (lowBeamVolt * smoothFactor) +
                (((float)lowBeam->analogRead() / ADC_MAX *
                  ADC_REF_VOLTAGE * DIVIDER_FACTOR) *
                 (1 - smoothFactor));
#endif

#ifdef TAIL_LIGHTS
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

#ifdef HEAD_LIGHTS
  highBeamState = highBeamVolt > VOLTAGE_THRESHOLD;
  lowBeamState = lowBeamVolt > VOLTAGE_THRESHOLD;
#endif

#ifdef TAIL_LIGHTS
  brakeState = brakeVolt > VOLTAGE_THRESHOLD;
  reverseState = reverseVolt > VOLTAGE_THRESHOLD;
#endif

#else

  accOnState = false;
  leftIndicatorState = false;
  rightIndicatorState = false;
  externalControlState = false;

#ifdef HEAD_LIGHTS
  highBeamState = false;
  lowBeamState = false;
#endif

#ifdef TAIL_LIGHTS
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

#ifdef HEAD_LIGHTS
      headlightEffect->setActive(false);
      headlightEffect->setSplit(false);
      headlightEffect->setColor(false, false, false);
#endif

#ifdef TAIL_LIGHTS
      brakeEffect->setActive(false);
      brakeEffect->setIsReversing(false);
      reverseLightEffect->setActive(false);
#endif
    }
  }

#ifdef TAIL_LIGHTS
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

#ifdef HEAD_LIGHTS
    headlightEffect->setActive(false);
    headlightEffect->setSplit(false);
    headlightEffect->setColor(false, false, false);
#endif

#ifdef TAIL_LIGHTS
    brakeEffect->setActive(false);
    brakeEffect->setIsReversing(false);
    reverseLightEffect->setActive(false);
#endif
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
    leftIndicatorEffect->setActive(false);
    rightIndicatorEffect->setActive(false);
    rgbEffect->setActive(false);
    nightriderEffect->setActive(false);
    startupEffect->setActive(false);

#ifdef HEAD_LIGHTS
    headlightEffect->setActive(false);
    headlightEffect->setSplit(false);
    headlightEffect->setColor(false, false, false);
#endif

#ifdef TAIL_LIGHTS
    brakeEffect->setActive(false);
    brakeEffect->setIsReversing(false);
    reverseLightEffect->setActive(false);
#endif
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

#ifdef HEAD_LIGHTS
    headlightEffect->setActive(false);
    headlightEffect->setSplit(false);
    headlightEffect->setColor(false, false, false);
#endif

#ifdef TAIL_LIGHTS
    brakeEffect->setActive(false);
    brakeEffect->setIsReversing(false);
    reverseLightEffect->setActive(false);
#endif
  }
  else
  {
    lastAccOn = currentTime;

    // When ACC is on, ensure the startup effect is turned off and reset flash tracking.
    startupEffect->setActive(false);

    // And process the other effects normally.
    leftIndicatorEffect->setActive(leftIndicatorState);
    rightIndicatorEffect->setActive(rightIndicatorState);

#ifdef HEAD_LIGHTS
    headlightEffect->setActive(highBeamState);
#endif

#ifdef TAIL_LIGHTS
    brakeEffect->setActive(brakeState);
    brakeEffect->setIsReversing(reverseState || reverseLightEffect->isAnimating());
    reverseLightEffect->setActive(reverseState);
#endif
  }
}

void Application::handleTestEffects()
{
  // Test Mode - Add headlight testing

  // reverseLightEffect->setActive(true);
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
