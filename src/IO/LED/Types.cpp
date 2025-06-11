#include "Types.h"

String RGBSyncData::print()
{
  return String("Hue Center: " + String(hueCenter) + ", Hue Edge: " + String(hueEdge) + ", Speed: " + String(speed) + ", Hue Offset: " + String(hueOffset) + ", Active: " + String(active));
}

String NightRiderSyncData::print()
{
  return String("Cycle Time: " + String(cycleTime) + ", Tail Length: " + String(tailLength) + ", Current Pos: " + String(currentPos) + ", Forward: " + String(forward) + ", Active: " + String(active));
}

String PoliceSyncData::print()
{
  return String("Mode: " + String((int)mode) + ", Flash Progress: " + String(flashProgress) + ", Cycle Progress: " + String(cycleProgress) + ", Current Flash: " + String(currentFlash) + ", Active: " + String(active));
}

void EffectSyncState::print()
{
  Serial.println("RGB: " + rgbSyncData.print());
  Serial.println("Night Rider: " + nightRiderSyncData.print());
  Serial.println("Police: " + policeSyncData.print());
}
