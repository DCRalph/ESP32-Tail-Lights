# ESP32 Custom LED Tail Lights

This project implements custom LED tail lights for a car using an ESP32 and a WS2812B LED strip. The system integrates with the car's indicator stalk and brake pedal for real-time control over various lighting effects and animations.

## Features

- **Custom Startup Animation**  
  A unique startup effect that automatically turns off to save battery if left on.
- **Brake Lights Animation**  
  Dynamic animations for brake lights when turning off.
- **Reverse Lights Animation**  
  Special animated effects for reverse lights.
- **Left/Right Indicators**  
  Swiping animations for both left and right indicators.
- **Extra Effects**  
  Additional features include RGB cycling and a "Night Rider" effect.
- **In-Car Control**  
  Seamlessly control the lighting states using the car's existing indicator stalk and brake pedal.

## Hardware Requirements

- **ESP32** microcontroller
- **WS2812B** LED strip
- Wires and connectors for integration with the car’s electrical system

## Software Requirements

- [PlatformIO](https://platformio.org/).
- Required libraries such as [FastLED](https://github.com/FastLED/FastLED) for controlling the WS2812B LED strip.

## Setup and Installation

1. **Clone the Repository:**

   ```bash
   git clone https://github.com/DCRalph/ESP32-Tail-Lights.git
   cd ESP32-Tail-Lights
   ```

2. **Configure Your Development Environment:**

   Open the project in the Arduino IDE or your preferred ESP32 development environment. Install the libraries mentioned in the Software Requirements section.

3. **Hardware Connections:**

   Connect the WS2812B LED strip and input signals (indicator stalk and brake pedal) to the corresponding pins on the ESP32. Use the following default pin mapping:

   ```cpp
   #ifdef S2_CAR
   #define LED_PIN 15      // On board LED (optional)
   #define LEDS_PIN 16     // LED strip data pin

   #define INPUT_1_PIN 1
   #define INPUT_2_PIN 2
   #define INPUT_3_PIN 3
   #define INPUT_4_PIN 4
   #define INPUT_5_PIN 5
   #define INPUT_6_PIN 6
   #endif
   ```

   Adjust these definitions based on your wiring and hardware.

4. **Upload the Code:**

   Compile and upload the code to your ESP32. Make sure to select the correct board and port in your development environment.

## Customization

- **Animations:**  
  The animations (startup, brake, reverse, indicators, and extra effects) can be customized by modifying the source code. Look for the functions handling each effect.
- **Battery Saving Feature:**  
  The startup animation includes an auto-off feature to save battery power if the lights are accidentally left on. Adjust timers as needed.
- **Pin Configurations:**  
  If using a different wiring configuration, update the pin definitions in the source code accordingly.

## Project Structure

- **src/**  
  Contains the main source code files implementing animations and hardware control.
- **README.md**  
  This file.
- **LICENSE**  
  The license file that describes the open source terms of this project.

  
## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for more details.
