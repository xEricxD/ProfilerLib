#ifndef _TIMEDEVENT_H
#define _TIMEDEVENT_H
#include "Timer.h"

struct TimedEvent;

#define SCOPED_EVENT(name) TimedEvent name(0, #name)
#define SCOPED_EVENT_COLORED(name, color) TimedEvent name(color, #name)

#define EVENT_START(name) {	\
														TimedEvent name(0, #name)
#define EVENT_END() }

struct TimedEvent
{
	TimedEvent(uint32_t color, const char* name);
	~TimedEvent();

private:
	Timer timer;
};

#endif