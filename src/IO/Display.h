#pragma once

#include "config.h"
#include <U8g2lib.h>
#include "IO/Battery.h"

#include "IO/ScreenManager.h"

#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64

class Display
{
private:
  bool _noTopBar = false;

  // Variables to track custom icon positions in the top bar
  int lastIconX = 0;
  bool isFirstIcon = true;

  // Notification system
  String notificationMessage = "";
  uint64_t notificationStartTime = 0;
  uint32_t notificationDuration = 0;
  bool notificationActive = false;

public:
  U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2 = U8G2_SH1106_128X64_NONAME_F_HW_I2C(U8G2_R0, U8X8_PIN_NONE);
  // U8G2_SSD1309_128X64_NONAME0_F_HW_I2C u8g2 = U8G2_SSD1309_128X64_NONAME0_F_HW_I2C(U8G2_R0, U8X8_PIN_NONE);

  Display();

  void init(void);

  void drawCenteredText(uint8_t y, String text);

  void drawTopBar(void);
  void noTopBar(void);
  void display(void);

  // Notification system
  void showNotification(String message, uint32_t durationMs = 2000);
  void hideNotification();
  bool isNotificationActive();
  void drawNotification();

  // Get X position for placing custom icons in the top bar
  int getCustomIconX(int width);

  // Reset custom icon positioning (call this before drawing a new frame if you want to reset positions)
  void resetCustomIconPosition();
};

// class Screen
// {
// public:
//   String name;
//   String topBarText;

//   /**
//    * @brief Constructs a new Screen object with the specified name.
//    *
//    * @param _name The name of the screen.
//    */
//   Screen(String _name);
//   Screen(String _name, String _topBarText);

//   void setTopBarText(String _text);

//   /**
//    * @brief Draws the screen.
//    */
//   virtual void draw();

//   /**
//    * @brief Updates the screen.
//    */
//   virtual void update();

//   /**
//    * @brief Called when the screen is entered.
//    */
//   virtual void onEnter();

//   /**
//    * @brief Called when the screen is exited.
//    */
//   virtual void onExit();
// };

extern Display display;