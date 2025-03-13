#pragma once
#include <Arduino.h>

// Callback type (can be a function pointer or std::function if available)
typedef std::function<void()> SequenceCallback;

class SequenceBase
{
public:
  SequenceBase(String _name, uint64_t _timeout);
  virtual ~SequenceBase();

  // Call this every loop; returns true when the sequence is complete.
  // When complete, the callback is called (if set) and the sequence resets.
  void loop();

  // Set the callback function to be called when the sequence completes.
  void setCallback(SequenceCallback cb);

  void setTimeout(uint64_t _timeout);

  // Set the sequence active state.
  void setActive(bool _active);
  bool isActive();

  bool isStarted();

  // Reset the sequence state.
  void reset();
  virtual void VReset() = 0;

  // Update the sequence state.
  // Return true if the sequence is complete.
  virtual bool update() = 0;

protected:
  void start();

  SequenceCallback callback;
  String name;
  uint64_t timeout;
  bool active;
  uint64_t startTime;
  bool started;
};
