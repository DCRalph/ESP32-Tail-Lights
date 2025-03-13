#include "StartupEffect.h"
#include "../LEDManager.h"
#include <cmath>
#include <Arduino.h> // For millis()

StartupEffect::StartupEffect(LEDManager *ledManager, uint8_t priority, bool transparent)
    : LEDEffect(ledManager, priority, transparent),
      active(false),
      phase(0),
      phase_start(0),
      T0(0.0f),
      T1a(0.6f),
      T1b(0.6f),
      T2(0.6f),
      T2_delay(0.3f),
      T3(0.6f),
      dash_length(15),
      edge_stop(15)
{
    center = ledManager->getNumLEDs() / 2.0f;
    left_dash_pos = center;
    right_dash_pos = center;
    fill_progress = 0.0f;
    split_progress = 0.0f;
}

void StartupEffect::setActive(bool a)
{
    if (active == a)
        return;

    active = a;
    if (active)
    {
        phase = 0;
        phase_start = millis(); // Record the starting time (ms)]

        left_dash_pos = center;
        right_dash_pos = center;
        fill_progress = 0.0f;
        split_progress = 0.0f;
    }
    else
    {
        phase = 0;
        phase_start = 0;
    }
}

bool StartupEffect::isActive()
{
    return active;
}

void StartupEffect::update()
{
    if (!active)
        return;
    unsigned long now = millis();
    if (phase_start == 0)
        phase_start = now;
    // Convert elapsed time from milliseconds to seconds.
    float elapsed = (now - phase_start) / 1000.0f;

    if (phase == 0)
    {
        // Phase 0: Red dot at center.
        if (elapsed >= T0)
        {
            phase = 1;
            phase_start = now;
        }
    }
    else if (phase == 1)
    {
        // Phase 1: Dash outward phase (1a)
        float progress = std::min(elapsed / T1a, 1.0f);
        float ease = 3 * progress * progress - 2 * progress * progress * progress; // ease in-out function
        left_dash_pos = center - (ease * center);
        right_dash_pos = center + (ease * center);
        if (progress >= 1.0f)
        {
            phase = 2;
            phase_start = now;
        }
    }
    else if (phase == 2)
    {
        // Phase 2: Bounce inward phase (dashes return to center).
        float progress = std::min(elapsed / T1b, 1.0f);
        float ease = 3 * progress * progress - 2 * progress * progress * progress;
        left_dash_pos = center - ((1 - ease) * center);
        right_dash_pos = center + ((1 - ease) * center);
        if (progress >= 1.0f || (left_dash_pos >= center - dash_length && right_dash_pos <= center + dash_length))
        {
            phase = 3;
            phase_start = now;
        }
    }
    else if (phase == 3)
    {
        // Phase 3: Fill sweep - from center outward fill red gradually.
        float progress = std::min(elapsed / T2, 1.0f);
        fill_progress = progress;

        if (progress >= 1.0f)
        {
            phase = 4;
            phase_start = now;
        }
    }
    else if (phase == 4)
    {
        // Phase 4: Delay period with full red.
        if (elapsed >= T2_delay)
        {
            phase = 5;
            phase_start = now;
        }
    }
    else if (phase == 5)
    {
        // Phase 5: Split & fade – red retracts from center; final fade state at 100%.
        float progress = std::min(elapsed / T3, 1.0f);
        split_progress = progress;
        if (progress >= 1.0f)
        {
            phase = 6; // Final steady state.
            phase_start = now;
        }
    }
    else if (phase == 6)
    {
        // Final state: no further changes.
    }
}

void StartupEffect::render(std::vector<Color> &buffer)
{
    if (!active)
        return;

    uint16_t num = ledManager->getNumLEDs();
    // Phase 0: Red dot at center.
    if (phase == 0)
    {
        int idx = (int)round(center);
        if (idx >= 0 && idx < num)
            buffer[idx] = Color(255, 0, 0);
    }
    // Phases 1 & 2: Render two dashes (each is a block of dash_length LEDs).
    else if (phase == 1 || phase == 2)
    {
        int left_start = (int)round(left_dash_pos);
        int right_start = (int)round(right_dash_pos);

        int left_size = (int)std::min((float)dash_length, center - left_start);
        int right_size = (int)std::min((float)dash_length, right_start - center);

        // Adjust sizes if near the edges.
        if (left_start < dash_length)
        {
            left_size -= dash_length - left_start;
            if (left_size < 2)
                left_size = 2;
        }
        if (right_start + dash_length > num)
        {
            right_size -= (right_start + dash_length) - num;
            if (right_size < 2)
                right_size = 2;
        }

        for (size_t i = 0; i < buffer.size(); i++)
        {
            if (i >= left_start && i < left_start + left_size)
                buffer[i] = Color(255, 0, 0);
            else if (i >= right_start - right_size && i < right_start)
                buffer[i] = Color(255, 0, 0);
        }
    }
    // Phase 3: Fill sweep from center.
    else if (phase == 3)
    {
        float p = fill_progress;

        // apply ease in out curve to p
        p = 3 * p * p - 2 * p * p * p;

        // map p from [0, 1] to [(dash_length / ledManager->getNumLEDs()), 1]
        p = (1 - ((float)dash_length / ledManager->getNumLEDs())) * p + ((float)dash_length / ledManager->getNumLEDs());

        for (int i = 0; i < num; i++)
        {
            if (i <= center)
            {
                if ((center - i) <= p * center)
                    buffer[i] = Color(255, 0, 0);
            }
            else
            {
                if ((i - center) <= p * (num - 1 - center))
                    buffer[i] = Color(255, 0, 0);
            }
        }
    }
    // Phase 4: Entire strip is full red.
    else if (phase == 4)
    {
        for (int i = 0; i < num; i++)
        {
            buffer[i] = Color(255, 0, 0);
        }
    }
    // Phase 5 and beyond: Split and fade.
    else if (phase >= 5)
    {
        float p = split_progress;
        // Apply an ease-in-out curve for a smoother transition.
        p = 3 * p * p - 2 * p * p * p;
        float left_cutoff = center - p * (center - edge_stop);
        float right_cutoff = center + p * ((num - edge_stop) - center);
        for (int i = 0; i < num; i++)
        {
            if (i < left_cutoff || i >= right_cutoff)
                buffer[i] = Color(255, 0, 0);
            else
                buffer[i] = Color(0, 0, 0);
        }
    }
}
