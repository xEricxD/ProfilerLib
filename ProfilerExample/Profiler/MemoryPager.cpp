#include "MemoryPager.h"

MemoryPager MemoryPager::s_memoryPager;

MemoryPager::MemoryPager() : m_numPages(0)
{
}

MemoryPager::~MemoryPager()
{
  for (auto it = m_freeList.begin(); it != m_freeList.end(); it)
  {
    ReleasePage(*it);
    it = m_freeList.erase(it);
  }

  for (auto it = m_usedList.begin(); it != m_usedList.end(); it)
  {
    ReleasePage(*it);
    it = m_usedList.erase(it);
  }
}

MemoryPager::Page* MemoryPager::GetPage()
{
  Page* p = nullptr;
  if (m_freeList.size() > 0)
  {
    p = m_freeList.back();
    m_freeList.pop_back();
  }
  else
  {
    p = AllocatePage();
  }

  m_usedList.push_back(p);
  return p;
}

void MemoryPager::ReleasePage(Page* page)
{
  for (auto it = m_usedList.begin(); it != m_usedList.end(); it++)
  {
    if (*it == page)
    {
      page->bufferCurrent = page->bufferStart;
      page->bufferReadOffset = page->bufferWriteOffset = 0;

      m_usedList.erase(it);
      m_freeList.push_back(page);
    }
  }
}

MemoryPager::Page* MemoryPager::AllocatePage()
{
  Page* p = new Page();
  p->bufferStart = new int8_t[kPageSize];
  p->bufferCurrent = p->bufferStart;
  p->bufferWriteOffset = p->bufferReadOffset = 0;

  m_numPages++;
  return p;
}

void MemoryPager::FreePage(Page* page)
{
  delete[] page->bufferStart;
  delete page;
  m_numPages--;
}
