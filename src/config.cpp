#include "config.h"

Preferences preferences;

void restart()
{
  Serial.println("[INFO] [CONFIG] Restarting...");
  ESP.restart();
}

String formatBytes(size_t bytes, bool _short)
{
  if (bytes < 1024)
  {
    if (_short)
    {
      // Since bytes is an integer, leave it as is.
      return String(bytes);
    }
    else
    {
      return String(bytes) + " B";
    }
  }
  else if (bytes < (1024UL * 1024UL))
  {
    // Force floating point math by casting bytes to double.
    double kb = ((double)bytes) / 1024.0;
    if (_short)
    {
      return String(kb, 1) + "K";
    }
    else
    {
      return String(kb, 2) + " KB";
    }
  }
  else
  {
    double mb = ((double)bytes) / (1024.0 * 1024.0);
    if (_short)
    {
      return String(mb, 1) + "M";
    }
    else
    {
      return String(mb, 2) + " MB";
    }
  }
}