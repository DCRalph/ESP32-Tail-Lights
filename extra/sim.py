import time, math
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation


# ---------------------
# Basic Color class
# ---------------------
class Color:
    def __init__(self, r=0, g=0, b=0):
        self.r = int(r)
        self.g = int(g)
        self.b = int(b)

    def to_tuple(self):
        # Return normalized tuple for plotting
        return (self.r / 255.0, self.g / 255.0, self.b / 255.0)


# ---------------------
# LED Manager
# ---------------------
class LEDManager:
    def __init__(self, num_leds):
        self.num_leds = num_leds
        self.buffer = [Color(0, 0, 0) for _ in range(num_leds)]
        self.effects = []

    def add_effect(self, effect):
        self.effects.append(effect)
        # Optional: sort by priority (if needed)
        self.effects.sort(key=lambda e: e.priority)

    def update_effects(self):
        for effect in self.effects:
            effect.update()
        # Clear the LED buffer
        self.buffer = [Color(0, 0, 0) for _ in range(self.num_leds)]
        for effect in self.effects:
            effect.render(self.buffer)

    def get_num_leds(self):
        return self.num_leds


# ---------------------
# Base effect class
# ---------------------
class LEDEffect:
    def __init__(self, led_manager, priority=0, transparent=False):
        self.led_manager = led_manager
        self.priority = priority
        self.transparent = transparent

    def update(self):
        raise NotImplementedError

    def render(self, buffer):
        raise NotImplementedError


# ---------------------
# StartupEffect
# ---------------------
class StartupEffect(LEDEffect):
    def __init__(self, led_manager, priority=0, transparent=False):
        super().__init__(led_manager, priority, transparent)
        self.active = False
        self.phase = 0  # 0: red dot; 1: dash out; 2: dash bounce back; 3: fill sweep; 4: full hold; 5: split & fade; 6: final steady state
        self.phase_start = None

        # Duration parameters (seconds)
        self.T0 = 0.5  # red dot duration
        self.T1a = 0.6  # dash outward phase duration
        self.T1b = 0.6  # dash bounce back duration
        self.T2 = 0.6  # fill sweep duration
        self.T2_delay = 0.3  # delay with full red
        self.T3 = 0.6  # split & fade duration

        # Effect parameters
        self.dash_length = 15  # number of LEDs in each dash
        self.edge_stop = 15  # final red region size at each strip end

        self.num_leds = self.led_manager.get_num_leds()
        self.center = (self.num_leds) / 2.0

        # Internal state variables
        self.left_dash_pos = self.center  # starting index (float) for left dash
        self.right_dash_pos = self.center  # starting index for right dash
        self.fill_progress = 0.0  # progress for fill sweep (0 to 1)
        self.split_progress = 0.0  # progress for split & fade (0 to 1)

    def setActive(self, active):
        self.active = active
        if active:
            self.phase = 0
            self.phase_start = time.time()
        else:
            self.phase = 0
            self.phase_start = None

    def update(self):
        if not self.active:
            return
        now = time.time()
        if self.phase_start is None:
            self.phase_start = now
        elapsed = now - self.phase_start

        # Phase 0: red dot at center
        if self.phase == 0:
            if elapsed >= self.T0:
                self.phase = 1
                self.phase_start = now
        # Phase 1: Dash outward phase (1a)
        elif self.phase == 1:
            progress = min(elapsed / self.T1a, 1.0)
            ease = 3 * progress**2 - 2 * progress**3  # ease in-out function
            # Left dash: moves from center to index 0
            self.left_dash_pos = self.center - (ease * self.center)
            self.right_dash_pos = self.center + (ease * self.center)

            if progress >= 1.0:
                self.phase = 2
                self.phase_start = now
        # Phase 2: Bounce inward phase (dashes return to center)
        elif self.phase == 2:
            progress = min(elapsed / self.T1b, 1.0)
            ease = 3 * progress**2 - 2 * progress**3
            self.left_dash_pos = self.center - ((1 - ease) * self.center)
            self.right_dash_pos = self.center + ((1 - ease) * self.center)

            if progress >= 1.0:
                self.phase = 3
                self.phase_start = now
        # Phase 3: Fill sweep - from center outward fill red gradually
        elif self.phase == 3:
            progress = min(elapsed / self.T2, 1.0)
            self.fill_progress = progress
            if progress >= 1.0:
                self.phase = 4
                self.phase_start = now
        # Phase 4: Delay with full red
        elif self.phase == 4:
            if elapsed >= self.T2_delay:
                self.phase = 5
                self.phase_start = now
        # Phase 5: Split and fade - red retracts from center; final state at p=1
        elif self.phase == 5:
            progress = min(elapsed / self.T3, 1.0)
            self.split_progress = progress
            if progress >= 1.0:
                self.phase = 6  # Final steady state
        elif self.phase == 6:
            # Final state: no further changes.
            pass

    def render(self, buffer):
        if not self.active:
            return
        num = self.num_leds

        # Phase 0: red dot at center
        if self.phase == 0:
            idx = int(round(self.center))
            if 0 <= idx < num:
                buffer[idx] = Color(255, 0, 0)
        # Phases 1 & 2: render two dashes (each dash is a block of dash_length LEDs)
        elif self.phase in [1, 2]:
            left_start = int(round(self.left_dash_pos))
            right_start = int(round(self.right_dash_pos))

            left_size = int(min(self.dash_length, self.center - left_start))
            right_size = int(min(self.dash_length, right_start - self.center))

            if left_start < self.dash_length:
                left_size -= self.dash_length - left_start
                left_size = max(2, left_size)
            if right_start + self.dash_length > num:
                right_size -= (right_start + self.dash_length) - num
                right_size = max(2, right_size)

            for i in range(len(buffer)):
                if left_start <= i < left_start + left_size:
                    buffer[i] = Color(255, 0, 0)
                elif right_start - right_size <= i < right_start:
                    buffer[i] = Color(255, 0, 0)

        # Phase 3: Fill sweep from center
        elif self.phase == 3:
            p = self.fill_progress
            for i in range(num):
                # For left half
                if i <= self.center:
                    if (self.center - i) <= p * self.center:
                        buffer[i] = Color(255, 0, 0)
                else:
                    if (i - self.center) <= p * (num - 1 - self.center):
                        buffer[i] = Color(255, 0, 0)
        # Phase 4: Entire strip is full red (delay period)
        elif self.phase == 4:
            for i in range(num):
                buffer[i] = Color(255, 0, 0)
        # Phase 5 and beyond: Split and fade
        elif self.phase >= 5:
            p = self.split_progress
            p = 3 * p**2 - 2 * p**3
            # Left fade: the cutoff moves from center to index edge_stop.
            left_cutoff = self.center - p * (self.center - self.edge_stop)
            # Right fade: cutoff moves from center to (num - edge_stop)
            right_cutoff = self.center + p * ((num - self.edge_stop) - self.center)
            for i in range(num):
                if i < left_cutoff or i >= right_cutoff:
                    # Fully red region.
                    buffer[i] = Color(255, 0, 0)
                else:
                    buffer[i] = Color(0, 0, 0)


# ---------------------
# Main simulation
# ---------------------
def main():
    # Create an LED Manager for, say, 60 LEDs.
    num_leds = 100
    led_manager = LEDManager(num_leds)

    # Create an instance of StartupEffect and add it.
    startup_effect = StartupEffect(led_manager, priority=0)
    startup_effect.setActive(True)
    led_manager.add_effect(startup_effect)

    # Set up matplotlib to visualize the LED strip.
    fig, ax = plt.subplots(figsize=(10, 2))
    led_colors = np.zeros((1, num_leds, 3))
    im = ax.imshow(led_colors, aspect="auto")
    ax.axis("off")

    # Animation update function.
    def update_frame(frame):
        led_manager.update_effects()
        # led_manager.buffer[30] = Color(0, 255, 0)
        for i, col in enumerate(led_manager.buffer):
            led_colors[0, i, :] = np.array(col.to_tuple())
        im.set_data(led_colors)
        return (im,)

    ani = FuncAnimation(fig, update_frame, interval=10, blit=True)
    plt.show()


if __name__ == "__main__":
    main()
