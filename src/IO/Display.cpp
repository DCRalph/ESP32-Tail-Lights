#include "Display.h"
#include "Wireless.h"

Display::Display()
{
}

void Display::init(void)
{
  Serial.println("\t[INFO] [Display] Initializing...");
  u8g2.begin();
  u8g2.setBusClock(1600000);

  Serial.println("\t[INFO] [Display] Initialized");
}

void Display::drawCenteredText(uint8_t y, String text)
{
  u8g2.drawStr((DISPLAY_WIDTH - u8g2.getStrWidth(text.c_str())) / 2, y, text.c_str());
}

int Display::getCustomIconX(int width)
{
  int iconX;

  if (isFirstIcon)
  {
    // First icon position is next to WiFi icon
    u8g2.setFont(u8g2_font_koleeko_tf);
    char buffer[32];
    sprintf(buffer, "%d%%%", batteryGetPercentageSmooth());
    int battW = u8g2.getStrWidth(buffer);

    // Position is DISPLAY_WIDTH - battW - 2 - 8 - 2 - width
    // (screen width - battery text width - space - wifi icon width - space - icon width)
    iconX = DISPLAY_WIDTH - battW - 2 - 8 - 2 - width;

    isFirstIcon = false;
  }
  else
  {
    // Subsequent icons are positioned to the left of the previous icon
    iconX = lastIconX - 2 - width; // 2 pixels spacing between icons
  }

  // Update last icon position for next call
  lastIconX = iconX;

  return iconX;
}

void Display::resetCustomIconPosition()
{
  isFirstIcon = true;
  lastIconX = 0;
}

void Display::drawTopBar(void)
{
  u8g2.setFont(u8g2_font_koleeko_tf);
  u8g2.setDrawColor(1);
  u8g2.drawStr(0, 9, screenManager.getCurrentScreen()->topBarText.c_str());
  u8g2.drawLine(0, 10, DISPLAY_WIDTH, 10);

  char buffer[32];

  sprintf(buffer, "%d%%%", batteryGetPercentageSmooth());

  u8g2.setFont(u8g2_font_koleeko_tf);
  int battW = u8g2.getStrWidth(buffer);
  u8g2.drawStr(DISPLAY_WIDTH - battW, 9, buffer);

  u8g2.setFont(u8g2_font_open_iconic_all_1x_t);
  if (WiFi.status() == WL_CONNECTED)
    u8g2.drawGlyph(DISPLAY_WIDTH - 8 - battW - 2, 9, 0x00f8); // wifi icon
  else if (wireless.isSetupDone())
    u8g2.drawGlyph(DISPLAY_WIDTH - 8 - battW - 2, 9, 0x00c6); // link icon
  else
    u8g2.drawGlyph(DISPLAY_WIDTH - 8 - battW - 2, 9, 0x0079); // x icon

  // Reset custom icon positions for the next frame
  resetCustomIconPosition();
}

void Display::noTopBar()
{
  _noTopBar = true;
}

void Display::display(void)
{

  // u8g2.firstPage();
  // do
  // {
  //   screenManager.draw();
  //   if (!_noTopBar)
  //     drawTopBar();
  // } while (u8g2.nextPage());

  // startTime = micros();
  u8g2.clearBuffer(); // Clear the internal buffer
  // elapsedTime = micros() - startTime;
  // clearBufferTime = elapsedTime;

  // startTime = micros();
  screenManager.draw();
  // elapsedTime = micros() - startTime;
  // screenManagerDrawTime = elapsedTime;

  // startTime = micros();
  if (!_noTopBar)
    drawTopBar();
  // elapsedTime = micros() - startTime;
  // drawTopBarTime = elapsedTime;

  // Check and draw notification if active
  if (isNotificationActive())
  {
    drawNotification();
  }

  // startTime = micros();
  u8g2.sendBuffer();
  // elapsedTime = micros() - startTime;
  // sendBufferTime = elapsedTime;

  // startTime = micros();
  screenManager.update();
  // elapsedTime = micros() - startTime;
  // screenUpdateDrawTime = elapsedTime;

  _noTopBar = false;
}

// Notification system implementation
void Display::showNotification(String message, uint32_t durationMs)
{
  notificationMessage = message;
  notificationStartTime = millis();
  notificationDuration = durationMs;
  notificationActive = true;
}

void Display::hideNotification()
{
  notificationActive = false;
  notificationMessage = "";
}

bool Display::isNotificationActive()
{
  if (!notificationActive)
    return false;

  // Check if notification has expired
  if (millis() - notificationStartTime >= notificationDuration)
  {
    hideNotification();
    return false;
  }

  return true;
}

void Display::drawNotification()
{
  if (!isNotificationActive() || notificationMessage.length() == 0)
    return;

  u8g2.setFont(u8g2_font_6x10_tf);

  // Calculate text dimensions
  int textWidth = u8g2.getStrWidth(notificationMessage.c_str());
  int textHeight = 8; // Font height for u8g2_font_6x10_tf

  // Calculate notification box dimensions and position
  int boxWidth = textWidth + 8;   // 4px padding on each side
  int boxHeight = textHeight + 6; // 3px padding top and bottom
  int boxX = (DISPLAY_WIDTH - boxWidth) / 2;
  int boxY = (DISPLAY_HEIGHT - boxHeight) / 2;

  // Draw notification background (inverted box)
  u8g2.setDrawColor(1);
  u8g2.drawBox(boxX, boxY, boxWidth, boxHeight);

  // Draw notification border
  u8g2.drawFrame(boxX - 1, boxY - 1, boxWidth + 2, boxHeight + 2);

  // Draw notification text (inverted color)
  u8g2.setDrawColor(0);
  u8g2.drawStr(boxX + 4, boxY + textHeight + 1, notificationMessage.c_str());

  // Reset draw color
  u8g2.setDrawColor(1);
}

// ######################
// Screen
// ######################

Screen::Screen(String _name)
{
  name = _name;
  topBarText = _name;
}

Screen::Screen(String _name, String _topBarText)
{
  name = _name;
  topBarText = _topBarText;
}

void Screen::setTopBarText(String _text)
{
  topBarText = _text;
}

void Screen::draw()
{
}

void Screen::update()
{
}

void Screen::onEnter()
{
}

void Screen::onExit()
{
}

Display display;