#ifndef _MEMORY_PAGER_H
#define _MEMORY_PAGER_H

#include <vector>

class MemoryPager
{
public:
  
  static const uint32_t kPageSize = (uint32_t)256e3; 
  struct Page
  {
    int8_t* bufferStart;
    int8_t* bufferCurrent;

    uint32_t bufferWriteOffset; // offset from the start to write to
    uint32_t bufferReadOffset;  // offset from the start to begin reading from
  };

  static MemoryPager* Get() { return &s_memoryPager; }

  Page* GetPage();
  void ReleasePage(Page* page);

  uint32_t GetNumPages() { return m_numPages; }

private:
  MemoryPager();
  ~MemoryPager();

  Page* AllocatePage();
  void FreePage(Page* page);

  static MemoryPager s_memoryPager;

  std::vector<Page*> m_freeList;
  std::vector<Page*> m_usedList;
  uint32_t m_numPages;
};

#endif