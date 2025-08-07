#include "CommitEffect.h"
#include <algorithm>

CommitEffect::CommitEffect(uint8_t priority, bool transparent)
    : LEDEffect(priority, transparent),
      active(false),
      commitSpeed(20000),             // LEDs per second * 1000 (20.0 * 1000)
      trailLength(15000),             // 15 LED trail length * 1000 (15.0 * 1000)
      commitInterval(1200),           // New commit every 1200 milliseconds (1.2 seconds)
      headR(0), headG(0), headB(255), // Bright green for commits
      timeSinceLastCommit(0),
      lastUpdateTime(0),
      syncEnabled(true)
{
  name = "Commit";
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
    timeSinceLastCommit = 0;
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

void CommitEffect::update(LEDSegment *segment)
{
  if (!active)
    return;

  unsigned long currentTime = SyncManager::syncMillis();
  if (lastUpdateTime == 0)
  {
    lastUpdateTime = currentTime;
    return;
  }

  // Calculate elapsed time in milliseconds
  uint32_t deltaTimeMillis = currentTime - lastUpdateTime;
  lastUpdateTime = currentTime;

  uint16_t numLEDs = segment->getNumLEDs();

  // Update time since last commit
  timeSinceLastCommit += deltaTimeMillis;

  // Spawn new commit if interval has passed
  if (timeSinceLastCommit >= commitInterval)
  {
    spawnCommit();
    timeSinceLastCommit = 0;
  }

  // Update existing commits
  updateCommits(deltaTimeMillis, numLEDs);
}

void CommitEffect::render(LEDSegment *segment, Color *buffer)
{
  if (!active)
    return;

  uint16_t numLEDs = segment->getNumLEDs();

  // Clear the buffer
  // for (uint16_t i = 0; i < numLEDs; i++)
  // {
  //   buffer[i] = Color(0, 0, 0);
  // }
  memset(buffer, 0, numLEDs * sizeof(Color));

  // Render all commits
  for (const auto &commit : commits)
  {
    renderCommit(commit, segment, buffer);
  }
}

void CommitEffect::onDisable()
{
  active = false;
}

void CommitEffect::spawnCommit()
{
  Commit newCommit;
  newCommit.positionLeft = 0;  // Start at center (fixed point 0)
  newCommit.positionRight = 0; // Start at center (fixed point 0)
  newCommit.age = 0;
  newCommit.activeLeft = true;
  newCommit.activeRight = true;

  commits.push_back(newCommit);
}

void CommitEffect::updateCommits(uint32_t deltaTimeMillis, uint16_t numLEDs)
{
  uint32_t centerPos = ((numLEDs - 1) * 1000) / 2; // Fixed point center position
  uint32_t maxDistance = centerPos;                // Maximum distance from center to edge

  // Update each commit
  for (auto &commit : commits)
  {
    commit.age += deltaTimeMillis;

    if (commit.activeLeft)
    {
      // commitSpeed is in LED positions per second * 1000
      // deltaTimeMillis is in milliseconds
      // Position increment = (commitSpeed * deltaTimeMillis) / 1000
      commit.positionLeft += (commitSpeed * deltaTimeMillis) / 1000;

      // Deactivate if it has moved beyond the edge plus trail length
      if (commit.positionLeft > maxDistance + trailLength)
      {
        commit.activeLeft = false;
      }
    }

    if (commit.activeRight)
    {
      // Same calculation for right side
      commit.positionRight += (commitSpeed * deltaTimeMillis) / 1000;

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

void CommitEffect::renderCommit(const Commit &commit, LEDSegment *segment, Color *buffer)
{
  uint16_t numLEDs = segment->getNumLEDs();
  uint32_t centerPos = ((numLEDs - 1) * 1000) / 2; // Fixed point center position

  // Render left-moving commit
  if (commit.activeLeft)
  {
    // headPos in fixed point (multiply by 1000)
    int32_t headPos = (int32_t)centerPos - (int32_t)commit.positionLeft;
    int headIndex = headPos / 1000; // Convert back to LED index

    // Draw head
    if (headIndex >= 0 && headIndex < static_cast<int>(numLEDs))
      buffer[headIndex] = Color(headR, headG, headB);

    // Draw trail
    for (int i = 0; i < static_cast<int>(numLEDs / 2); i++)
    {
      if (i == headIndex)
        continue; // Skip head

      // Distance in fixed point (multiply LED index by 1000 to match headPos scale)
      int32_t distance = (i * 1000) - headPos; // Distance behind the head (trail to the right of head)
      if (distance > 0 && distance <= (int32_t)trailLength)
      {
        // Brightness diminishes linearly with distance
        // brightness = 1000 - (distance * 1000 / trailLength) (in fixed point with 1000 scale)
        uint32_t brightness = 1000 - ((uint32_t)distance * 1000 / trailLength);

        buffer[i] = Color(headR, headG, headB) * (brightness / 1000.0f);
      }
    }
  }

  // Render right-moving commit
  if (commit.activeRight)
  {
    // headPos in fixed point
    int32_t headPos = (int32_t)centerPos + (int32_t)commit.positionRight;
    int headIndex = headPos / 1000; // Convert back to LED index

    // Draw head
    if (headIndex >= 0 && headIndex < static_cast<int>(numLEDs))
      buffer[headIndex] = Color(headR, headG, headB);

    // Draw trail
    for (int i = numLEDs / 2; i < static_cast<int>(numLEDs); i++)
    {
      if (i == headIndex)
        continue; // Skip head

      // Distance in fixed point (multiply LED index by 1000 to match headPos scale)
      int32_t distance = headPos - (i * 1000); // Distance behind the head (trail to the left of head)
      if (distance > 0 && distance <= (int32_t)trailLength)
      {
        // Brightness diminishes linearly with distance
        uint32_t brightness = 1000 - ((uint32_t)distance * 1000 / trailLength);

        buffer[i] = Color(headR, headG, headB) * (brightness / 1000.0f);
      }
    }
  }
}