#include <stdarg.h>
#include <time.h>
#include <string>
#include <thread>
#include "Profiler.h"
#include "imgui/imgui.h"
#include "ImGuiExtended.h"


uint32_t StringToColor(const char* str)
{
  int a = 54059;
  int b = 76963;

  int hash = 31;
  size_t strl = strlen(str);

  for (size_t i = 0; i < strl; i++)
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
    sprintf_s(temp, "%.2f TB", (float)bytes * (1.0f / tb));
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
{
  strcpy_s(m_threadName, "test name");
  m_threadID = (uint32_t)__threadid();
}

void ProfilerEventManager::PushEvent(uint32_t color, const char* pFormat)
{
  ProfilerEvent ev;

  ev.startTime = clock();
  ev.depth = m_eventDepth++;
  ev.color = color;

  // Copy name
  size_t len = strlen(pFormat);
	memset(ev.name, 0, 64);
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
  : m_isOpen(true), m_numEventsInCapture(0)
  , m_precedingFrameTime(10), m_procedingFrameTime(10), m_lastXAmountOfTime((int)(100))
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
  int currTime = clock();
  for (auto it = m_frameTimes.begin(); it != m_frameTimes.end();)
  {
    if (currTime > kMaxProfileTime && it->startTime + it->duration < (int)(currTime - kMaxProfileTime))
      it = m_frameTimes.erase(it);
    else
      it++;
  }

  // start new frame
  FrameTime ft;
  ft.startTime = clock();
  ft.duration = 0;
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
        ProfilerEvent* ev = reinterpret_cast<ProfilerEvent*>(page->bufferCurrent);
        if (ev->startTime + ev->duration >= currTime - (kMaxProfileTime * 1e3))
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

void Profiler::GetCurrentCapture()
{
  // Clear old capture data
  m_numEventsInCapture = 0;
  for (auto it = m_captureInfo.begin(); it != m_captureInfo.end(); it++)
  {
    for (auto p = it->pages.begin(); p != it->pages.end(); p++)
      MemoryPager::Get()->ReleasePage(*p);
  }
  m_captureInfo.clear();

  m_captureTime = clock();

  // Get current data
  for (auto pem = m_managers.begin(); pem != m_managers.end(); pem++)
  {
    ProfilerEventManager* mngr = *pem;
    std::vector<MemoryPager::Page*> &pages = mngr->GetPages();

    m_captureInfo.push_back(ThreadEventInfo());
    ThreadEventInfo &info = m_captureInfo.back();
    strcpy_s(info.threadName, mngr->GetThreadName());
    info.threadID = mngr->GetThreadID();
    info.maxDepth = 0;

    // Copy all pages
    for (auto p = pages.begin(); p != pages.end(); p++)
    {
      MemoryPager::Page* page = *p;
      MemoryPager::Page* newPage = MemoryPager::Get()->GetPage();

      // Copy page data into new page
      memcpy(newPage->bufferStart, page->bufferStart, page->bufferWriteOffset);
      newPage->bufferReadOffset = page->bufferReadOffset;
      newPage->bufferWriteOffset = page->bufferWriteOffset;
      info.pages.push_back(newPage);

      // Extract events from new page
      uint32_t currRead = newPage->bufferReadOffset;
      newPage->bufferCurrent = newPage->bufferStart + currRead;
      while (currRead < newPage->bufferWriteOffset)
      {
        ProfilerEvent* ev = reinterpret_cast<ProfilerEvent*>(newPage->bufferCurrent);
        info.events.push_back(ev);
        currRead += sizeof(ProfilerEvent);
        newPage->bufferCurrent += sizeof(ProfilerEvent);

        if (ev->depth > info.maxDepth)
          info.maxDepth = ev->depth;
      }
    }

    m_numEventsInCapture += (uint32_t)info.events.size();
  }

  // get longest frame time
  m_captureFrameTimes = m_frameTimes;
  m_longestFrame.duration = 0;
  for (auto it = m_captureFrameTimes.begin(); it != m_captureFrameTimes.end(); it++)
  {
    if (it->duration > m_longestFrame.duration)
      m_longestFrame = *it;
  }
}

void Profiler::Render()
{
  ImGuiIO io = ImGui::GetIO();

  ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.1f, io.DisplaySize.y * 0.1f), ImGuiSetCond_Once);
  ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x * 0.8f, io.DisplaySize.y * 0.8f), ImGuiSetCond_Once);

  ImGui::Begin("Profiler", &m_isOpen);

  // Capture button
  ImGui::PushItemWidth(ImGui::GetWindowSize().x * 0.1f);
  if (ImGui::Button("Show Capture"))
  {
    GetCurrentCapture();
  }
  ImGui::PopItemWidth();

  ImGui::SameLine();

  ImGui::PushItemWidth(ImGui::GetWindowSize().x * 0.25f);
  enum ProfilerType : int { kShowLongestFrame = 0, kLongestFrameWithMargin, kLastXMilliseconds };
  const char* comboItems[] = { "Show Longest Frame", "Show Longest Frame with Margin", "Show Last X Amount of Time" };
  static int type = kLastXMilliseconds;
  ImGui::Combo("Profile Mode", &type, comboItems, 3);
  ImGui::PopItemWidth();

  // Render profiler type data
  if (type == kLongestFrameWithMargin)
  {
    ImGui::PushItemWidth(ImGui::GetWindowSize().x * 0.1f);
    ImGui::SameLine();
    ImGui::InputInt("Preceding frame time", &m_precedingFrameTime);
    ImGui::SameLine();
    ImGui::InputInt("Proceding frame time", &m_procedingFrameTime);
    ImGui::PopItemWidth();
  }
  else if (type == kLastXMilliseconds)
  {
    ImGui::PushItemWidth(ImGui::GetWindowSize().x * 0.1f);
    ImGui::SameLine();
    ImGui::InputInt("Time in MS", &m_lastXAmountOfTime);
    ImGui::PopItemWidth();
  }

  ImGui::Text("Showing %u events for %u threads, %u Pages created, total mem: %s", m_numEventsInCapture, m_managers.size(), MemoryPager::Get()->GetNumPages(), BytesToSize((float)MemoryPager::Get()->GetNumPages() * MemoryPager::kPageSize).c_str());

  static float zoomLevel = 100.0f;
  ImGui::SliderFloat("Zoom (temp)", &zoomLevel, 0, 1000);

  // Setup some information we need to help display
  int startTime = 0;
	int displayTime = 0; // total time we will display for this frame
  switch (type)
  {
  case kShowLongestFrame:
    startTime = m_longestFrame.startTime;
    displayTime = m_longestFrame.duration;
    break;
  case kLongestFrameWithMargin:
    startTime = m_longestFrame.startTime - m_precedingFrameTime;
    displayTime = m_precedingFrameTime + m_longestFrame.duration + m_procedingFrameTime;
    break;
  case kLastXMilliseconds:
    startTime = m_captureTime - m_lastXAmountOfTime;
    displayTime = m_lastXAmountOfTime;
    break;
  }
	  
  // Create profiler layout, will be filled with data later
  ImGui::BeginChild("ThreadData", ImVec2(ImGui::GetWindowSize().x * 0.15f, 0), false, ImGuiWindowFlags_NoScrollbar);
  ImGui::EndChild();

  ImGui::SameLine();

  ImGui::BeginChild("EventData", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

  // Calculate clip rect
  ImVec2 clipRectPos(ImGui::GetWindowPos());
  ImVec2 clipRectSize(ImGui::GetWindowSize());
  ImVec2 clipRectEnd(clipRectPos.x + clipRectSize.x, clipRectPos.y + clipRectSize.y);

  // Start by drawing the timeline
  // Store current cursor pos
  ImVec2 cursorScreenPosStart = ImGui::GetCursorScreenPos();

  // Calculate how big each MS will be displayed
  float step = 20 + (zoomLevel * 0.2f);
  ImGui::SetCursorScreenPos(ImVec2(cursorScreenPosStart.x, ImGui::GetWindowPos().y));
	
	// Set the event window size
	ImVec2 curPosStart = ImGui::GetCursorScreenPos();
	ImGui::SetCursorScreenPos(ImVec2(curPosStart.x + (step * displayTime), ImGui::GetWindowPos().y));
	ImGui::SetCursorScreenPos(ImVec2(curPosStart.x, ImGui::GetWindowPos().y));
	
	// calculate visibility
	int displayTimeStart = (int)(ImGui::GetScrollX() / step); // round down
	int displayTimeEnd = std::fmin((int)(displayTimeStart + ImGui::GetWindowSize().x / step) + 2, displayTime); // round up;
	int displayTimeVisible = displayTimeEnd - displayTimeStart;

	// Set cursorpos to first visible ms
	ImGui::SetCursorScreenPos(ImVec2(curPosStart.x + (step * displayTimeStart), ImGui::GetWindowPos().y));
  for (uint32_t i = displayTimeStart; i < displayTimeEnd; i++)
  {
    ImVec2 curPos = ImGui::GetCursorScreenPos();
    ImGui::Text("%i", i);
    if ((i % 2) == 0)
      ImGui::GetWindowDrawList()->AddRectFilled(curPos, ImVec2(curPos.x + step, curPos.y + ImGui::GetWindowSize().y), IM_COL32(50, 50, 50, 200));
	
    ImGui::SetCursorScreenPos(ImVec2(curPos.x + step, ImGui::GetWindowPos().y));
  }
  cursorScreenPosStart.y += ImGui::GetTextLineHeight();
  ImVec2 cursorScreenPosEnd = ImVec2(cursorScreenPosStart.x + (step * displayTime), cursorScreenPosStart.y);

  ImGui::SetCursorScreenPos(cursorScreenPosStart);

  ImGui::EndChild();
  
  // Draw frame times
  ImGui::BeginChild("EventData", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

	// Calculate default item height we'll be using
	float itemHeight = (ImGui::GetWindowFontSize() + ImGui::GetStyle().FramePadding.y * 2) * 0.35f;
	float lineheight = itemHeight * 1.2f;

  for (auto it = m_captureFrameTimes.begin(); it != m_captureFrameTimes.end(); it++)
  {
    if (it->startTime + it->duration < startTime + displayTimeStart || it->startTime > startTime + displayTimeStart + displayTimeVisible)
      continue;

    // Frame is atleast partially inside the time we're displaying, so render it
    // Calculate start pos
    float totalProfileLength = (float)(cursorScreenPosEnd.x - cursorScreenPosStart.x);
    float startP = (float)(it->startTime - startTime) / displayTime;
    ImVec2 framePos((startP * totalProfileLength) + cursorScreenPosStart.x, cursorScreenPosStart.y);
    // Calculate size
    ImVec2 frameSize(((float)it->duration / displayTime) * totalProfileLength, itemHeight);
    ImVec2 frameEnd(framePos.x + frameSize.x, framePos.y + frameSize.y);

    if (ImGui_ClipRect(framePos, frameEnd, clipRectPos, clipRectEnd))
    {
      ImGui::GetWindowDrawList()->AddRectFilled(framePos, frameEnd, IM_COL32(std::fmin(it->duration * 5, 255), 200, 200, 255)); // TODO - randomize frame color
      if (ImGui_IsItemHovered(framePos, frameEnd))
      {
        ImGui::BeginTooltip();
        ImGui::Text("Frame time: %ums", it->duration);
        ImGui::EndTooltip();
      }
    }
  }
	ImGui::EndChild();

	ImGui::BeginChild("EventData", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
	// Update cursor pos and item size
	itemHeight = (ImGui::GetWindowFontSize() + ImGui::GetStyle().FramePadding.y * 2) * 0.6f;
	lineheight = itemHeight * 1.2f;
	cursorScreenPosStart.y += lineheight;
	ImGui::SetCursorScreenPos(cursorScreenPosStart);
	ImGui::EndChild();

	// Draw events for each thread
	for (auto it = m_captureInfo.begin(); it != m_captureInfo.end(); it++)
	{
		ThreadEventInfo &info = *it;

		// TODO - render thread info for left panel (name, num events, etc)
		ImGui::BeginChild("ThreadData", ImVec2(ImGui::GetWindowSize().x * 0.15f, 0), false, ImGuiWindowFlags_NoScrollbar);
		ImGui::EndChild();

		// Render event data
		ImGui::BeginChild("EventData", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
		// TODO - clip event range (binary search)
		for (auto evIt = info.events.begin(); evIt != info.events.end(); evIt++)
		{
			ProfilerEvent* ev = *evIt;

			// Clip if event is out of visible range
			if (ev->startTime + ev->duration < startTime + displayTimeStart || ev->startTime > startTime + displayTimeStart + displayTimeVisible)
				continue;

			// Calculate start pos
			float totalProfileLength = (float)(cursorScreenPosEnd.x - cursorScreenPosStart.x);
			float startP = (float)(ev->startTime - startTime) / displayTime;
			ImVec2 framePos((startP * totalProfileLength) + cursorScreenPosStart.x, cursorScreenPosStart.y);
			// Calculate size
			ImVec2 frameSize(((float)ev->duration / displayTime) * totalProfileLength, itemHeight);
			ImVec2 frameEnd(framePos.x + frameSize.x, framePos.y + frameSize.y);

			if (ImGui_ClipRect(framePos, frameEnd, clipRectPos, clipRectEnd))
			{
				ImGui::GetWindowDrawList()->AddRectFilled(framePos, frameEnd, ev->color);
				if (ImGui_IsItemHovered(framePos, frameEnd))
				{
					ImGui::BeginTooltip();
					ImGui::Text("%s (%ums)", ev->name, ev->duration);
					ImGui::EndTooltip();
				}
			}
		}
		ImGui::EndChild();
	}


  ImGui::End(); // end profiler window
}