#pragma once

#include "../Effects.h"
#include <vector>

class CommitEffect : public LEDEffect
{
public:
  // Constructs the Commit effect.
  // Sends "commits" from the center of the strip to the edges with bright heads and fading trails
  CommitEffect(uint8_t priority = 0, bool transparent = false);

  virtual void update(LEDSegment *segment) override;
  virtual void render(LEDSegment *segment, Color *buffer) override;
  void NewFunction(uint16_t numLEDs, Color *buffer);
  virtual void onDisable() override;

  // Activate or disable the effect.
  void setActive(bool active);
  bool isActive() const;

  // Sync functionality
  void setSyncData(CommitSyncData syncData);
  CommitSyncData getSyncData();

  // Customizable parameters:
  uint32_t commitSpeed;        // Speed of commits in LED positions per second * 1000 (fixed point)
  uint16_t trailLength;        // Length of the fading trail in LED units * 1000 (fixed point, default 15000)
  uint32_t commitInterval;     // Time between commits in milliseconds
  uint8_t headR, headG, headB; // Color of the commit head

private:
  bool active;

  struct Commit
  {
    uint32_t positionLeft;  // Position of commit moving left from center * 1000 (fixed point)
    uint32_t positionRight; // Position of commit moving right from center * 1000 (fixed point)
    uint32_t age;           // Age of the commit in milliseconds
    bool activeLeft;        // Whether left commit is still active
    bool activeRight;       // Whether right commit is still active
  };

  std::vector<Commit> commits;
  uint32_t timeSinceLastCommit;
  unsigned long lastUpdateTime;

  // Sync support
  bool syncEnabled;

  void spawnCommit();
  void updateCommits(uint32_t deltaTimeMillis, uint16_t numLEDs);
  void renderCommit(const Commit &commit, LEDSegment *segment, Color *buffer);
};