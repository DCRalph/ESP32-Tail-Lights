#include "CommitEffect.h"
#include <cmath>
#include <algorithm>

CommitEffect::CommitEffect(uint8_t priority, bool transparent)
    : LEDEffect(priority, transparent),
      active(false),
      commitSpeed(20.0f),             // LEDs per second
      trailLength(15.0f),             // 15 LED trail length as requested
      commitInterval(1.2f),           // New commit every 0.8 seconds
      headR(0), headG(0), headB(255), // Bright green for commits
      timeSinceLastCommit(0.0f),
      lastUpdateTime(0),
      syncEnabled(true)
{
}

void CommitEffect::setActive(bool _active)
{
  if (active == _active)
    return;

  active = _active;

  if (active)
  {
    // Reset state
    commits.clear();
    timeSinceLastCommit = 0.0f;
    lastUpdateTime = SyncManager::syncMillis();
  }
}

bool CommitEffect::isActive() const
{
  return active;
}

void CommitEffect::setSyncData(CommitSyncData syncData)
{
  active = syncData.active;
  commitSpeed = syncData.commitSpeed;
  trailLength = syncData.trailLength;
  commitInterval = syncData.commitInterval;
  headR = syncData.headR;
  headG = syncData.headG;
  headB = syncData.headB;
  timeSinceLastCommit = syncData.timeSinceLastCommit;
}

CommitSyncData CommitEffect::getSyncData()
{
  return CommitSyncData{
      .commitSpeed = commitSpeed,
      .trailLength = trailLength,
      .commitInterval = commitInterval,
      .headR = headR,
      .headG = headG,
      .headB = headB,
      .timeSinceLastCommit = timeSinceLastCommit,
      .active = active,
  };
}

void CommitEffect::update(LEDStrip *strip)
{
  if (!active)
    return;

  unsigned long currentTime = SyncManager::syncMillis();
  if (lastUpdateTime == 0)
  {
    lastUpdateTime = currentTime;
    return;
  }

  // Calculate elapsed time in seconds
  unsigned long dtMillis = currentTime - lastUpdateTime;
  float deltaTime = dtMillis / 1000.0f;
  lastUpdateTime = currentTime;

  uint16_t numLEDs = strip->getNumLEDs();
  if (numLEDs < 3) // Need at least 3 LEDs for center + edges
    return;

  // Update time since last commit
  timeSinceLastCommit += deltaTime;

  // Spawn new commit if interval has passed
  if (timeSinceLastCommit >= commitInterval)
  {
    spawnCommit();
    timeSinceLastCommit = 0.0f;
  }

  // Update existing commits
  updateCommits(deltaTime, numLEDs);
}

void CommitEffect::render(LEDStrip *strip, Color *buffer)
{
  if (!active)
    return;

  uint16_t numLEDs = strip->getNumLEDs();
  if (numLEDs < 3)
    return;

  // Clear the buffer
  for (uint16_t i = 0; i < numLEDs; i++)
  {
    buffer[i] = Color(0, 0, 0);
  }

  // Render all commits
  for (const auto &commit : commits)
  {
    renderCommit(commit, strip, buffer, numLEDs);
  }
}

void CommitEffect::onDisable()
{
  active = false;
}

void CommitEffect::spawnCommit()
{
  Commit newCommit;
  newCommit.positionLeft = 0.0f;  // Start at center
  newCommit.positionRight = 0.0f; // Start at center
  newCommit.age = 0.0f;
  newCommit.activeLeft = true;
  newCommit.activeRight = true;

  commits.push_back(newCommit);
}

void CommitEffect::updateCommits(float deltaTime, uint16_t numLEDs)
{
  float centerPos = (numLEDs - 1) / 2.0f;
  float maxDistance = centerPos; // Maximum distance from center to edge

  // Update each commit
  for (auto &commit : commits)
  {
    commit.age += deltaTime;

    if (commit.activeLeft)
    {
      commit.positionLeft += commitSpeed * deltaTime;
      // Deactivate if it has moved beyond the edge plus trail length
      if (commit.positionLeft > maxDistance + trailLength)
      {
        commit.activeLeft = false;
      }
    }

    if (commit.activeRight)
    {
      commit.positionRight += commitSpeed * deltaTime;
      // Deactivate if it has moved beyond the edge plus trail length
      if (commit.positionRight > maxDistance + trailLength)
      {
        commit.activeRight = false;
      }
    }
  }

  // Remove inactive commits
  commits.erase(
      std::remove_if(commits.begin(), commits.end(),
                     [](const Commit &c)
                     { return !c.activeLeft && !c.activeRight; }),
      commits.end());
}

void CommitEffect::renderCommit(const Commit &commit, LEDStrip *strip, Color *buffer, uint16_t numLEDs)
{
  float centerPos = (numLEDs - 1) / 2.0f;

  // Render left-moving commit
  if (commit.activeLeft)
  {
    float headPos = centerPos - commit.positionLeft;
    int headIndex = static_cast<int>(roundf(headPos));

    // Draw head
    if (headIndex >= 0 && headIndex < static_cast<int>(numLEDs))
    {
      buffer[headIndex] = Color(headR, headG, headB);
    }

    // Draw trail
    for (int i = 0; i < static_cast<int>(numLEDs); i++)
    {
      if (i == headIndex)
        continue; // Skip head

      float distance = headPos - i; // Distance behind the head
      if (distance > 0 && distance <= trailLength)
      {
        // Brightness diminishes linearly with distance
        float brightness = 1.0f - (distance / trailLength);
        uint8_t trailR = static_cast<uint8_t>(headR * brightness);
        uint8_t trailG = static_cast<uint8_t>(headG * brightness);
        uint8_t trailB = static_cast<uint8_t>(headB * brightness);

        // Add to existing color (for blending)
        uint16_t newR = buffer[i].r + trailR;
        uint16_t newG = buffer[i].g + trailG;
        uint16_t newBl = buffer[i].b + trailB;

        buffer[i] = Color(
            std::min(newR, (uint16_t)255),
            std::min(newG, (uint16_t)255),
            std::min(newBl, (uint16_t)255));
      }
    }
  }

  // Render right-moving commit
  if (commit.activeRight)
  {
    float headPos = centerPos + commit.positionRight;
    int headIndex = static_cast<int>(roundf(headPos));

    // Draw head
    if (headIndex >= 0 && headIndex < static_cast<int>(numLEDs))
    {
      // Add to existing color for blending
      uint16_t newR = buffer[headIndex].r + headR;
      uint16_t newG = buffer[headIndex].g + headG;
      uint16_t newBl = buffer[headIndex].b + headB;

      buffer[headIndex] = Color(
          std::min(newR, (uint16_t)255),
          std::min(newG, (uint16_t)255),
          std::min(newBl, (uint16_t)255));
    }

    // Draw trail
    for (int i = 0; i < static_cast<int>(numLEDs); i++)
    {
      if (i == headIndex)
        continue; // Skip head

      float distance = i - headPos; // Distance behind the head
      if (distance > 0 && distance <= trailLength)
      {
        // Brightness diminishes linearly with distance
        float brightness = 1.0f - (distance / trailLength);
        uint8_t trailR = static_cast<uint8_t>(headR * brightness);
        uint8_t trailG = static_cast<uint8_t>(headG * brightness);
        uint8_t trailB = static_cast<uint8_t>(headB * brightness);

        // Add to existing color (for blending)
        uint16_t newR = buffer[i].r + trailR;
        uint16_t newG = buffer[i].g + trailG;
        uint16_t newBl = buffer[i].b + trailB;

        buffer[i] = Color(
            std::min(newR, (uint16_t)255),
            std::min(newG, (uint16_t)255),
            std::min(newBl, (uint16_t)255));
      }
    }
  }
}