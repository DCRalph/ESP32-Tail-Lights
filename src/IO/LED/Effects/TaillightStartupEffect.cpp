// #include "TaillightStartupEffect.h"
// #include <cmath>
// #include <Arduino.h> // For millis()

// TaillightStartupEffect::TaillightStartupEffect(uint8_t priority, bool transparent)
//     : LEDEffect(priority, transparent),
//       active(false),
//       phase(0),
//       phase_start(0),
//       T0(0.0f),
//       T1a(0.6f),
//       T1b(0.6f),
//       T2(0.6f),
//       T2_delay(0.3f),
//       T3(0.6f),
//       dash_length(15),
//       edge_stop(15),
//       outward_progress(0.0f),
//       inward_progress(0.0f),
//       fill_progress(0.0f),
//       split_progress(0.0f)
// {
// }

// void TaillightStartupEffect::setActive(bool a)
// {
//   if (active == a)
//     return;

//   active = a;
//   if (active)
//   {
//     phase = 0;
//     phase_start = millis(); // Record the starting time (ms)

//     outward_progress = 0.0f;
//     inward_progress = 0.0f;
//     fill_progress = 0.0f;
//     split_progress = 0.0f;
//   }
//   else
//   {
//     phase = 0;
//     phase_start = 0;
//   }
// }

// bool TaillightStartupEffect::isActive()
// {
//   return active;
// }

// void TaillightStartupEffect::update(LEDStrip *strip)
// {
//   if (!active)
//     return;
//   unsigned long now = millis();
//   if (phase_start == 0)
//     phase_start = now;
//   // Convert elapsed time from milliseconds to seconds.
//   float elapsed = (now - phase_start) / 1000.0f;

//   if (phase == 0)
//   {
//     // Phase 0: Red dot at center.
//     if (elapsed >= T0)
//     {
//       phase = 1;
//       phase_start = now;
//     }
//   }
//   else if (phase == 1)
//   {
//     // Phase 1: Dash outward phase (1a)
//     outward_progress = std::min(elapsed / T1a, 1.0f);
//     // Apply ease in-out function
//     outward_progress = 3 * outward_progress * outward_progress - 2 * outward_progress * outward_progress * outward_progress;

//     if (outward_progress >= 1.0f)
//     {
//       phase = 2;
//       phase_start = now;
//     }
//   }
//   else if (phase == 2)
//   {
//     // Phase 2: Bounce inward phase (dashes return to center).
//     inward_progress = std::min(elapsed / T1b, 1.0f);
//     // Apply ease in-out function
//     inward_progress = 3 * inward_progress * inward_progress - 2 * inward_progress * inward_progress * inward_progress;

//     if (inward_progress >= 1.0f)
//     {
//       phase = 3;
//       phase_start = now;
//     }
//   }
//   else if (phase == 3)
//   {
//     // Phase 3: Fill sweep - from center outward fill red gradually.
//     fill_progress = std::min(elapsed / T2, 1.0f);

//     if (fill_progress >= 1.0f)
//     {
//       phase = 4;
//       phase_start = now;
//     }
//   }
//   else if (phase == 4)
//   {
//     // Phase 4: Delay period with full red.
//     if (elapsed >= T2_delay)
//     {
//       phase = 5;
//       phase_start = now;
//     }
//   }
//   else if (phase == 5)
//   {
//     // Phase 5: Split & fade – red retracts from center; final fade state at 100%.
//     split_progress = std::min(elapsed / T3, 1.0f);
//     if (split_progress >= 1.0f)
//     {
//       phase = 6; // Final steady state.
//       phase_start = now;
//     }
//   }
//   else if (phase == 6)
//   {
//     // Final state: no further changes.
//   }
// }

// void TaillightStartupEffect::render(LEDStrip *strip, Color *buffer)
// {
//   if (!active)
//     return;

//   Color color = Color();
//   if (strip->getType() == LEDStripType::TAILLIGHT)
//     color = Color(255, 0, 0);
//   else
//     color = Color(255, 255, 255);

//   uint16_t numLEDs = strip->getNumLEDs();
//   float center = numLEDs / 2.0f;

//   // Phase 0: Red dot at center.
//   if (phase == 0)
//   {
//     int idx = (int)round(center);
//     if (idx >= 0 && idx < numLEDs)
//       buffer[idx] = color;
//   }
//   // Phases 1 & 2: Render two dashes (each is a block of dash_length LEDs).
//   else if (phase == 1 || phase == 2)
//   {
//     float progress = (phase == 1) ? outward_progress : (1.0f - inward_progress);

//     float left_dash_pos = center - (progress * center);
//     float right_dash_pos = center + (progress * center);

//     int left_start = (int)round(left_dash_pos);
//     int right_start = (int)round(right_dash_pos);

//     int left_size = (int)std::min((float)dash_length, center - left_start);
//     int right_size = (int)std::min((float)dash_length, right_start - center);

//     // Adjust sizes if near the edges.
//     if (left_start < dash_length)
//     {
//       left_size -= dash_length - left_start;
//       if (left_size < 2)
//         left_size = 2;
//     }
//     if (right_start + dash_length > numLEDs)
//     {
//       right_size -= (right_start + dash_length) - numLEDs;
//       if (right_size < 2)
//         right_size = 2;
//     }

//     for (uint16_t i = 0; i < numLEDs; i++)
//     {
//       if (i >= left_start && i < left_start + left_size)
//         buffer[i] = color;
//       else if (i >= right_start - right_size && i < right_start)
//         buffer[i] = color;
//     }
//   }
//   // Phase 3: Fill sweep from center.
//   else if (phase == 3)
//   {
//     float p = fill_progress;

//     // apply ease in out curve to p
//     p = 3 * p * p - 2 * p * p * p;

//     // new_dash_length is the length of the dash in the fill sweep.
//     // it starts at 0 and grows to dash_length as p goes from 0 to 0.2
//     // then it stays at dash_length as p goes from 0.2 to 0.8
//     float new_dash_length = (p <= 0.2f) ? (p * 5.0f * dash_length) : dash_length;

//     // map p from [0, 1] to [(dash_length / numLEDs), 1]
//     p = (1 - ((float)new_dash_length / numLEDs)) * p + ((float)new_dash_length / numLEDs);

//     for (int i = 0; i < numLEDs; i++)
//     {
//       if (i <= center)
//       {
//         if ((center - i) <= p * center)
//           buffer[i] = color;
//       }
//       else
//       {
//         if ((i - center) <= p * (numLEDs - 1 - center))
//           buffer[i] = color;
//       }
//     }
//   }
//   // Phase 4: Entire strip is full red.
//   else if (phase == 4)
//   {
//     for (int i = 0; i < numLEDs; i++)
//     {
//       buffer[i] = color;
//     }
//   }
//   // Phase 5 and beyond: Split and fade.
//   else if (phase >= 5)
//   {
//     float p = split_progress;
//     // Apply an ease-in-out curve for a smoother transition.
//     p = 3 * p * p - 2 * p * p * p;
//     float left_cutoff = center - p * (center - edge_stop);
//     float right_cutoff = center + p * ((numLEDs - edge_stop) - center);
//     for (int i = 0; i < numLEDs; i++)
//     {
//       if (i < left_cutoff || i >= right_cutoff)
//         buffer[i] = color;
//       else
//         buffer[i] = Color(0, 0, 0);
//     }
//   }
// }