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
  virtual void onDisable() override;

  // Activate or disable the effect.
  void setActive(bool active);
  bool isActive() const;

  // Sync functionality
  void setSyncData(CommitSyncData syncData);
  CommitSyncData getSyncData();

  // Customizable parameters:
  float commitSpeed;           // Speed of commits in LED positions per second
  float trailLength;           // Length of the fading trail in LED units (default 15)
  float commitInterval;        // Time between commits in seconds
  uint8_t headR, headG, headB; // Color of the commit head

private:
  bool active;

  struct Commit
  {
    float positionLeft;  // Position of commit moving left from center
    float positionRight; // Position of commit moving right from center
    float age;           // Age of the commit in seconds
    bool activeLeft;     // Whether left commit is still active
    bool activeRight;    // Whether right commit is still active
  };

  std::vector<Commit> commits;
  float timeSinceLastCommit;
  unsigned long lastUpdateTime;

  // Sync support
  bool syncEnabled;

  void spawnCommit();
  void updateCommits(float deltaTime, uint16_t numLEDs);
  void renderCommit(const Commit &commit, LEDSegment *segment, Color *buffer);
};