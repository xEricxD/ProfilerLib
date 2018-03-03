#include "TimedEvent.h"
#include "Profiler.h"

TimedEvent::TimedEvent(uint32_t color, const char* name)
{
	ProfilerEventManager::ProfilerEvent* ev = Profiler::Get()->BeginEvent(color, name);
	timer.Start(&ev->startTime, &ev->duration);
}

TimedEvent::~TimedEvent()
{
	timer.End();
	Profiler::Get()->EndEvent();
}
