#include "Display.h"
#include "Wireless.h"
#include "TimeProfiler.h"

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

  if (screenManager.getCurrentScreen())
    u8g2.drawStr(0, 9, screenManager.getCurrentScreen()->topBarText.c_str());
  else
    u8g2.drawStr(0, 9, "Unknown");

  u8g2.drawLine(0, 10, DISPLAY_WIDTH, 10);

  char buffer[32];

  sprintf(buffer, "%.1fV", batteryGetVoltageSmooth());

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
  timeProfiler.increment("displayFps");
  timeProfiler.start("display", TimeUnit::MICROSECONDS);

  // u8g2.firstPage();
  // do
  // {
  //   screenManager.draw();
  //   if (!_noTopBar)
  //     drawTopBar();
  // } while (u8g2.nextPage());

  timeProfiler.start("clearBuffer", TimeUnit::MICROSECONDS);
  u8g2.clearBuffer(); // Clear the internal buffer
  timeProfiler.stop("clearBuffer");

  timeProfiler.start("screenManagerDraw", TimeUnit::MICROSECONDS);
  screenManager.draw();
  timeProfiler.stop("screenManagerDraw");

  timeProfiler.start("drawTopBar", TimeUnit::MICROSECONDS);
  if (!_noTopBar)
    drawTopBar();
  timeProfiler.stop("drawTopBar");

  // Check and draw notification if active
  if (isNotificationActive())
  {
    timeProfiler.start("drawNotification", TimeUnit::MICROSECONDS);
    drawNotification();
    timeProfiler.stop("drawNotification");
  }

  timeProfiler.start("sendBuffer", TimeUnit::MILLISECONDS);
  u8g2.sendBuffer();
  timeProfiler.stop("sendBuffer");

  timeProfiler.start("screenUpdate", TimeUnit::MICROSECONDS);
  screenManager.update();
  timeProfiler.stop("screenUpdate");

  timeProfiler.stop("display");

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

Display display;