#include "Types.h"
#include "Effects/SolidColorEffect.h"

String RGBSyncData::print()
{
  return String("Hue Center: " + String(hueCenter) + ", Hue Edge: " + String(hueEdge) + ", Speed: " + String(speed) + ", Hue Offset: " + String(hueOffset) + ", Active: " + String(active));
}

String NightRiderSyncData::print()
{
  return String("Cycle Time: " + String(cycleTime) + ", Tail Length: " + String(tailLength) + ", Progress: " + String(progress) + ", Forward: " + String(forward) + ", Active: " + String(active));
}

String PoliceSyncData::print()
{
  return String("Mode: " + String((int)mode) + ", Flash Progress: " + String(flashProgress) + ", Cycle Progress: " + String(cycleProgress) + ", Current Flash: " + String(currentFlash) + ", Active: " + String(active));
}

String SolidColorSyncData::print()
{
  return String("Color Preset: " + String((int)colorPreset) + ", RGB: (" + String(customR) + "," + String(customG) + "," + String(customB) + "), Active: " + String(active));
}

String ColorFadeSyncData::print()
{
  return String("Hold Time: " + String(holdTime) + ", Fade Time: " + String(fadeTime) + ", Progress: " + String(progress) + ", Current Color Index: " + String(currentColorIndex) + ", In Fade Phase: " + String(inFadePhase) + ", Active: " + String(active));
}

String CommitSyncData::print()
{
  return String("Commit Speed: " + String(commitSpeed) + ", Trail Length: " + String(trailLength) + ", Commit Interval: " + String(commitInterval) + ", Head Color: (" + String(headR) + "," + String(headG) + "," + String(headB) + "), Time Since Last: " + String(timeSinceLastCommit) + ", Active: " + String(active));
}

void EffectSyncState::print()
{
  Serial.println("RGB: " + rgbSyncData.print());
  Serial.println("Night Rider: " + nightRiderSyncData.print());
  Serial.println("Police: " + policeSyncData.print());
  Serial.println("Solid Color: " + solidColorSyncData.print());
  Serial.println("Color Fade: " + colorFadeSyncData.print());
  Serial.println("Commit: " + commitSyncData.print());
}
