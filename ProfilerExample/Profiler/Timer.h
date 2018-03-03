#ifndef _TIMER_H
#define _TIMER_H
#include <chrono>

struct Timer
{
	void Start(unsigned long long* aStartTime, unsigned long long* aDuration)
	{
		startTime = aStartTime;
		duration = aDuration;

		start = std::chrono::high_resolution_clock::now();
		*startTime = (start - globalStartTime).count();
	}

	void End()
	{
		std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
		auto elaps = end - start;
		*duration = (elaps).count();
	}

	static void Init() { globalStartTime = std::chrono::high_resolution_clock::now(); }
	static std::chrono::high_resolution_clock::time_point GetGlobalStartTime() { return globalStartTime; }

	// Pointer to start and end times
	unsigned long long* startTime;
	unsigned long long* duration;

	// Clock to track times
	std::chrono::high_resolution_clock::time_point start;

	// TODO - remove this
	static std::chrono::high_resolution_clock::time_point globalStartTime;
};

#endif