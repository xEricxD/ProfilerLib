#ifndef _PROFILER_H
#define _PROFILER_H

#include "MemoryPager.h"

// Single event data
struct ProfilerEvent
{
  int32_t startTime;   // 4 -> 4
	int32_t duration;    // 4 -> 8
	int32_t color;       // 4 -> 12
	int32_t depth;       // 4 -> 16
  char name[64];        // 64 -> 80
};

// Per-thread event manager
class ProfilerEventManager
{
public:
  ProfilerEventManager();
  ~ProfilerEventManager() {}

  void PushEvent(uint32_t color, const char* pFormat);
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
  static const uint32_t kMaxProfileTime = (uint32_t)10e3; // 10 second buffer
  struct FrameTime
  {
		int32_t startTime;
		int32_t duration;
  };

  static Profiler* Get() { return &s_profiler; }

  // return the current threads event manager
  ProfilerEventManager* GetEventManager();
  
  void BeginEvent(uint32_t color, const char* pFormat, ...);
  void EndEvent();

  void BeginFrame();
  void EndFrame();

  void Render();

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
    std::vector<ProfilerEvent*> events;
  };

  std::vector<FrameTime> m_frameTimes;
  std::vector<ProfilerEventManager*> m_managers;
  bool m_isOpen;

  // Capture info
  std::vector<ThreadEventInfo> m_captureInfo;
  std::vector<FrameTime> m_captureFrameTimes;
  uint32_t m_numEventsInCapture;
  uint32_t m_captureTime;
  FrameTime m_longestFrame;

  // Profiler type data
  int m_precedingFrameTime;
  int m_procedingFrameTime;
  int m_lastXAmountOfTime;
};

#endif
