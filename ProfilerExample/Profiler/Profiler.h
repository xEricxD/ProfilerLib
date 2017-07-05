#ifndef _PROFILER_H
#define _PROFILER_H

#include "MemoryPager.h"

// Single event data
struct ProfilerEvent
{
  uint32_t startTime;   // 4 -> 4
  uint32_t duration;    // 4 -> 8
  uint32_t color;       // 4 -> 12
  uint32_t depth;       // 4 -> 16
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

private:
  MemoryPager::Page* m_currentPage;
  MemoryPager::Page* m_stackPage;
  uint32_t m_eventDepth;


  std::vector<MemoryPager::Page*> m_pages;
  std::vector<MemoryPager::Page*> m_stackPages;
  std::vector<ProfilerEvent*> m_eventStack;
};

// Profiler class
class Profiler
{
public:
  static const uint32_t kMaxProfileTime = (uint32_t)10e3; // 10 second buffer
  struct FrameTime
  {
    uint32_t startTime;
    uint32_t duration;
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

  static Profiler s_profiler;

  std::vector<FrameTime> m_frameTimes;
  std::vector<ProfilerEventManager*> m_managers;
  bool m_isOpen;
};

#endif
