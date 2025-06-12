#include "Application.h"

void Application::setupSequences()
{
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
                                taillightEffect->setStartup();
                                headlightEffect->setStartup();
                                unlockSequence->setActive(false);
                                lockSequence->reset();
                                //
                              });

  lockSequence->setActive(true);
  lockSequence->setCallback([this]()
                            {
                              taillightEffect->setOff();
                              headlightEffect->setOff();
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
                                   policeEffect->setActive(false);
                                   pulseWaveEffect->setActive(false);
                                   auroraEffect->setActive(false);

                                   enableNormalMode();
                                   //
                                 });
#endif
}