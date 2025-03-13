#include "SequenceBase.h"

SequenceBase::SequenceBase(String _name, uint64_t _timeout)
    : name(_name),
      timeout(_timeout),
      active(false),
      started(false),
      startTime(0)
{
}

SequenceBase::~SequenceBase()
{
}

void SequenceBase::loop()
{
  if (!active)
  {
    started = false;
    startTime = 0;
    return;
  }

  if (update())
  {
    Serial.println("Sequence complete: " + name);
    if (callback)
    {
      callback();
    }
    reset();
  }
  else if (started && startTime != 0 && (millis() - startTime) > timeout)
  {
    reset();
    Serial.println("Sequence timeout: " + name);
  }
}

void SequenceBase::setCallback(SequenceCallback cb)
{
  callback = cb;
}

void SequenceBase::setTimeout(uint64_t _timeout)
{
  timeout = _timeout;
}

void SequenceBase::setActive(bool _active)
{
  active = _active;
  reset();

  if (active)
    Serial.println("Sequence activated: " + name);
  else
    Serial.println("Sequence deactivated: " + name);
}

bool SequenceBase::isActive()
{
  return active;
}

bool SequenceBase::isStarted()
{
  return started;
}

void SequenceBase::reset()
{
  started = false;
  startTime = 0;
  VReset();
  Serial.println("Sequence reset: " + name);
}

void SequenceBase::VReset()
{
}

bool SequenceBase::update()
{
  return false;
}

void SequenceBase::start()
{
  if (started)
    return;

  startTime = millis();
  started = true;

  Serial.println("Sequence started: " + name);
}