#ifndef _PROFILER_H
#define _PROFILER_H

#include <chrono>
#include "MemoryPager.h"

// Per-thread event manager
class ProfilerEventManager
{
public:
	// Single event data
	struct ProfilerEvent
	{
		unsigned long long startTime;   // 8 -> 8
		unsigned long long duration;    // 8 -> 16
		uint32_t color;									// 4 -> 20
		uint32_t depth;									// 4 -> 24
		char name[64];									// 64 -> 88
	};

  ProfilerEventManager();
  ~ProfilerEventManager() {}

	ProfilerEventManager::ProfilerEvent* PushEvent(uint32_t color, const char* pFormat);
  void PopEvent();

  std::vector<MemoryPager::Page*> &GetPages() { return m_pages; }

  uint32_t GetThreadID() { return m_threadID; }
  const char* GetThreadName() { return m_threadName; }

private:
  MemoryPager::Page* m_currentPage;
  MemoryPager::Page* m_stackPage;
  uint32_t m_eventDepth;
  
  std::vector<MemoryPager::Page*> m_pages;
  std::vector<MemoryPager::Page*> m_stackPages;
  std::vector<ProfilerEvent*> m_eventStack;

  // Thread info
  char m_threadName[64];
  uint32_t m_threadID;
};

// Profiler class
class Profiler
{
public:
  static const unsigned long long kMaxProfileTime = (unsigned long long)(10e9); // 10 second buffer
  struct FrameTime
  {
		unsigned long long startTime;
		unsigned long long duration;
		int32_t color;
  };

  static Profiler* Get() { return &s_profiler; }

  // return the current threads event manager
  ProfilerEventManager* GetEventManager();
  
  ProfilerEventManager::ProfilerEvent* BeginEvent(uint32_t color, const char* aName);
  void EndEvent();

  void BeginFrame();
  void EndFrame();

  void Render();
	void UpdateZoom();

private:
  Profiler();
  ~Profiler();

  void ClearOutdatedEvents();
  void GetCurrentCapture();

  static Profiler s_profiler;

  struct ThreadEventInfo
  {
    char threadName[64];
    uint32_t threadID;
    uint32_t maxDepth; // max event depth for this thread

    std::vector<MemoryPager::Page*> pages;
    std::vector<ProfilerEventManager::ProfilerEvent*> events;
  };

  std::vector<FrameTime> m_frameTimes;
  std::vector<ProfilerEventManager*> m_managers;
  bool m_isOpen;

  // Capture info
  std::vector<ThreadEventInfo> m_captureInfo;
  std::vector<FrameTime> m_captureFrameTimes;
  uint32_t m_numEventsInCapture;
  unsigned long long m_captureTime;
  FrameTime m_longestFrame;

  // Profiler type data
  int m_precedingFrameTime;
  int m_procedingFrameTime;
  int m_lastXAmountOfTime;

	float m_framesPerSecond;
	float m_zoom;

	// Frame timer helpers
	std::chrono::high_resolution_clock::time_point m_frameStart;
};

#endif
