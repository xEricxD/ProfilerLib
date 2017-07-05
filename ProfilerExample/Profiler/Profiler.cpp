#include <stdarg.h>
#include <time.h>
#include <string>
#include "Profiler.h"
#include "imgui/imgui.h"

uint32_t StringToColor(const char* str)
{
  int a = 54059;
  int b = 76963;

  int hash = 31;
  size_t strl = strlen(str);

  for(size_t i = 0; i < strl; i++)
  {
    hash = (hash * a) ^ (str[i] * b);
    a = a * b;
  }

  uint8_t red = (uint8_t)((hash & 0xFF000000) >> 24);
  uint8_t green = (uint8_t)((hash & 0x00FF0000) >> 16);
  uint8_t blue = (uint8_t)((hash & 0x0000FF00) >> 8);

  return IM_COL32(red, green, blue, 255);
}

std::string BytesToSize(float bytes) 
{
  static const float tb = 1099511627776;
  static const float gb = 1073741824;
  static const float mb = 1048576;
  static const float kb = 1024;
  
  char temp[1024] = {};

  if (bytes >= tb)
    sprintf_s(temp,"%.2f TB", (float)bytes * (1.0f / tb));
  else if (bytes >= gb)
    sprintf_s(temp, "%.2f GB", (float)bytes * (1.0f / gb));
  else if (bytes >= mb)
    sprintf_s(temp, "%.2f MB", (float)bytes * (1.0f / mb));
  else if (bytes >= kb)
    sprintf_s(temp, "%.2f KB", (float)bytes * (1.0f / kb));
  else
    sprintf_s(temp, "%.2f B", bytes);

  std::string returnSize(temp);
  return returnSize;
}

//******************************************************
//                Profiler Event Manager
//******************************************************
ProfilerEventManager::ProfilerEventManager()
  : m_currentPage(nullptr), m_stackPage(nullptr)
  , m_eventDepth(0)
{}

void ProfilerEventManager::PushEvent(uint32_t color, const char* pFormat)
{
  ProfilerEvent ev;

  ev.startTime = clock();
  ev.depth = m_eventDepth++;
  ev.color = color;

  // Copy name
  size_t len = strlen(pFormat);
  memcpy(ev.name, pFormat, len > 64 ? 64 : len);
  
  // Check if event will fit in current page
  if (m_stackPage == nullptr)
  {
    m_stackPage = MemoryPager::Get()->GetPage();
    m_stackPages.push_back(m_stackPage);
  }
  else if (m_stackPage->bufferWriteOffset + sizeof(ProfilerEvent) > MemoryPager::kPageSize)
  {
    m_stackPage = MemoryPager::Get()->GetPage();
    m_stackPages.push_back(m_stackPage);
  }

  // Copy the event into the memory page and add to event stack
  m_stackPage->bufferCurrent = m_stackPage->bufferStart + m_stackPage->bufferWriteOffset;
  memcpy(m_stackPage->bufferCurrent, &ev, sizeof(ProfilerEvent));
  m_eventStack.push_back((ProfilerEvent*)m_stackPage->bufferCurrent);

  // Update page variables
  m_stackPage->bufferWriteOffset += sizeof(ProfilerEvent);
}

void ProfilerEventManager::PopEvent()
{
  ProfilerEvent* ev = m_eventStack.back();
  m_eventStack.pop_back();

  ev->duration = clock() - ev->startTime;

  if (ev->duration >= 1)
  {
    // get color based on name if no color is set
    if (ev->color == 0)
      ev->color = StringToColor(ev->name);

    // Check if event will fit in current page
    if (m_currentPage == nullptr)
    {
      m_currentPage = MemoryPager::Get()->GetPage();
      m_pages.push_back(m_currentPage);
    }
    else if (m_currentPage->bufferWriteOffset + sizeof(ProfilerEvent) > MemoryPager::kPageSize)
    {
      m_currentPage = MemoryPager::Get()->GetPage();
      m_pages.push_back(m_currentPage);
    }

    // Copy the event into the current page
    m_currentPage->bufferCurrent = m_currentPage->bufferStart + m_currentPage->bufferWriteOffset;
    memcpy(m_currentPage->bufferCurrent, ev, sizeof(ProfilerEvent));
    m_currentPage->bufferWriteOffset += sizeof(ProfilerEvent);
  }

  m_eventDepth--;
}

//******************************************************
//                Profiler
//******************************************************
Profiler Profiler::s_profiler;

Profiler::Profiler() 
  : m_isOpen(true)
{}

Profiler::~Profiler()
{}

// Create a per-thread manager
thread_local ProfilerEventManager* g_manager = nullptr;
ProfilerEventManager* Profiler::GetEventManager()
{
  if (g_manager == nullptr)
  {
    g_manager = new ProfilerEventManager();
    m_managers.push_back(g_manager);
  }

  return g_manager;
}

void Profiler::BeginEvent(uint32_t color, const char* pFormat, ...)
{
  char temp[1024];
  // Copy name
  va_list argptr;
  va_start(argptr, pFormat);
  vsprintf_s(temp, 1024, pFormat, argptr);
  GetEventManager()->PushEvent(color, temp);
  va_end(argptr);
}

void Profiler::EndEvent()
{
  GetEventManager()->PopEvent();
}

void Profiler::BeginFrame()
{
  // Remove outdated frame times
  uint32_t currTime = clock();
  for (auto it = m_frameTimes.begin(); it != m_frameTimes.end();)
  {
    if (it->startTime + it->duration < currTime - kMaxProfileTime)
      it = m_frameTimes.erase(it);
    else
      it++;
  }

  // start new frame
  FrameTime ft;
  ft.startTime = clock();
  m_frameTimes.push_back(ft);

  // Remove outdated events
  ClearOutdatedEvents();
}

void Profiler::EndFrame()
{
  m_frameTimes.back().duration = clock() - m_frameTimes.back().startTime;
}

void Profiler::ClearOutdatedEvents()
{
  uint32_t currTime = clock();

  for (auto it = m_managers.begin(); it != m_managers.end(); it++)
  {
    std::vector<MemoryPager::Page*> &pages = (*it)->GetPages();
    
    // Check each page for outdated events
    for (auto p = pages.begin(); p != pages.end();)
    {
      MemoryPager::Page* page = *p;
      page->bufferCurrent = page->bufferStart + page->bufferReadOffset;
      while (page->bufferReadOffset < page->bufferWriteOffset)
      {
        ProfilerEvent* ev = (ProfilerEvent*)page->bufferCurrent;
        if (ev->startTime + ev->duration >= currTime - kMaxProfileTime)
          break;

        page->bufferReadOffset += sizeof(ProfilerEvent);
      }

      // Check if page is fully outdated, and release if it is
      if (page->bufferReadOffset >= page->bufferWriteOffset)
      {
        p = pages.erase(p);
        MemoryPager::Get()->ReleasePage(page);
      }
      else
        p++;

    }
  }

}

void Profiler::Render()
{
  ImGuiIO io = ImGui::GetIO();

  ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.1f, io.DisplaySize.y * 0.1f), ImGuiSetCond_Once);
  ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x * 0.8f, io.DisplaySize.y * 0.8f), ImGuiSetCond_Once);

  ImGui::Begin("Profiler", &m_isOpen);
  ImGui::Text("%u Pages created for %u threads, total mem: %s", MemoryPager::Get()->GetNumPages(), m_managers.size(), BytesToSize((float)MemoryPager::Get()->GetNumPages() * MemoryPager::kPageSize).c_str());
  ImGui::End();
}