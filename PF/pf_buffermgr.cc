#include <cstdio>
#include <unistd.h>
#include <iostream>
#include "pf_buffermgr.h"

using namespace std;

// The switch PF_STATS indicates that the user wishes to have statistics
// tracked for the PF layer
#ifdef PF_STATS
#include "statistics.h"   // For StatisticsMgr interface

// Global variable for the statistics manager
StatisticsMgr *pStatisticsMgr;
#endif


//
// PF_BufferMgr
//
// Desc: Constructor - called by PF_Manager::PF_Manager
//       The buffer manager manages the page buffer.  When asked for a page,
//       it checks if it is in the buffer.  If so, it pins the page (pages
//       can be pinned multiple times).  If not, it reads it from the file
//       and pins it.  If the buffer is full and a new page needs to be
//       inserted, an unpinned page is replaced according to an LRU
// In:   numPages - the number of pages in the buffer
//
// Note: The constructor will initialize the global pStatisticsMgr.  We
//       make it global so that other components may use it and to allow
//       easy access.
//
// Aut2003
// numPages changed to _numPages for to eliminate CC warnings

PF_BufferMgr::PF_BufferMgr(int _numPages) : hashTable(PF_HASH_TBL_SIZE)
{
  pStatisticsMgr = new StatisticsMgr ();
  bufTable = new PF_BufPageDesc[_numPages]();
  numPages = _numPages;
  pageSize = PF_PAGE_SIZE + 4;
  first = -1;
  last = -1;
  free = -1;
  for(int i = numPages - 1; i >= 0; i--)
  {
    InsertFree(i);
  }
}

//
// ~PF_BufferMgr
//
// Desc: Destructor - called by PF_Manager::~PF_Manager
//
PF_BufferMgr::~PF_BufferMgr()
{
  delete[] bufTable;
}

//
// GetPage
//
// Desc: Get a pointer to a page pinned in the buffer.  If the page is
//       already in the buffer, (re)pin the page and return a pointer
//       to it.  If the page is not in the buffer, read it from the file,
//       pin it, and return a pointer to it.  If the buffer is full,
//       replace an unpinned page.
// In:   fd - OS file descriptor of the file to read
//       pageNum - number of the page to read
//       bMultiplePins - if FALSE, it is an error to ask for a page that is
//                       already pinned in the buffer.
// Out:  ppBuffer - set *ppBuffer to point to the page in the buffer
// Ret:  PF return code
//
RC PF_BufferMgr::GetPage(int fd, PageNum pageNum, char **ppBuffer,
      int bMultiplePins)
{
  // page in buffer
  pStatisticsMgr->Register(PF_GETPAGE, STAT_ADDONE);
  pStatisticsMgr->Register("GetPage", STAT_ADDONE);

  int slot;
  RC rc_ = hashTable.Find(fd, pageNum, slot);
  if(rc_ == OK_RC)
  {
    pStatisticsMgr->Register(PF_PAGEFOUND, STAT_ADDONE);
    pStatisticsMgr->Register("PageFound", STAT_ADDONE);
    if(!bMultiplePins && bufTable[slot].pinCount > 0)
    {
      return PF_PAGEPINNED;
    }
    else
    {
      *ppBuffer = bufTable[slot].pData;
      bufTable[slot].pinCount++;
      Unlink(slot);
      LinkHead(slot);
      return OK_RC;
    }
  }

  else
  {
    pStatisticsMgr->Register(PF_PAGENOTFOUND, STAT_ADDONE);
    pStatisticsMgr->Register("PageNotFound", STAT_ADDONE);
    RC rc;
    if((rc = AllocatePage(fd, pageNum, ppBuffer)))
    {
      return rc;
    }
    if((rc = ReadPage(fd, pageNum, *ppBuffer)))
    {
      return rc;
    }
  // page is full and has not unpinned page
  }
	return OK_RC;
  //return PF_NOBUF;
}

//
// AllocatePage
//
// Desc: Allocate a new page in the buffer and return a pointer to it.
// In:   fd - OS file descriptor of the file associated with the new page
//       pageNum - number of the new page
// Out:  ppBuffer - set *ppBuffer to point to the page in the buffer
// Ret:  PF return code
//
RC PF_BufferMgr::AllocatePage(int fd, PageNum pageNum, char **ppBuffer)
{
  if(free != -1)
    {
      int cur_free = free;
      RC rc;
      if((rc = InitPageDesc(fd, pageNum, free)))
        return rc;
      *ppBuffer = bufTable[free].pData;
      if(bufTable[free].next != -1)
        bufTable[bufTable[free].next].prev = -1;
      free = bufTable[free].next;
      LinkHead(cur_free);
      return OK_RC;
    }
  else
  {
    int slot = last, prev;
    while(slot != INVALID_SLOT)
    {
      prev = bufTable[slot].prev;
      if(bufTable[slot].pinCount == 0)
      {
        if(bufTable[slot].bDirty == 1)
        {
          ForcePages(bufTable[slot].fd, bufTable[slot].pageNum);
        }
        delete[] bufTable[slot].pData;
        Unlink(slot);
        RC rc;
        hashTable.Delete(bufTable[slot].fd, bufTable[slot].pageNum);
        if((rc = InitPageDesc(fd, pageNum, slot)))
          return rc;
        *ppBuffer = bufTable[slot].pData;
        LinkHead(slot);
        return OK_RC;
      }
      slot = prev;
    }
  }
  return PF_NOBUF;
}

//
// MarkDirty
//
// Desc: Mark a page dirty so that when it is discarded from the buffer
//       it will be written back to the file.
// In:   fd - OS file descriptor of the file associated with the page
//       pageNum - number of the page to mark dirty
// Ret:  PF return code
//
RC PF_BufferMgr::MarkDirty(int fd, PageNum pageNum)
{
  int slot, next;
  slot = first;
  while(slot != INVALID_SLOT)
  {
    next = bufTable[slot].next;
    if(bufTable[slot].pageNum == pageNum && bufTable[slot].fd == fd)
    {
      bufTable[slot].bDirty = 1;
      return OK_RC;
    }
    slot = next;
  }

  return PF_PAGENOTINBUF; 
}

//
// UnpinPage
//
// Desc: Unpin a page so that it can be discarded from the buffer.
// In:   fd - OS file descriptor of the file associated with the page
//       pageNum - number of the page to unpin
// Ret:  PF return code
//
RC PF_BufferMgr::UnpinPage(int fd, PageNum pageNum)
{
  int slot, next;
  slot = first;
  while(slot != INVALID_SLOT)
  {
    next = bufTable[slot].next;
    if(bufTable[slot].pageNum == pageNum && bufTable[slot].fd == fd)
    {
      if(bufTable[slot].pinCount > 0)
      {
        bufTable[slot].pinCount--;
        return OK_RC;
      }
      else
      {
        return PF_PAGEUNPINNED; 
      }
    }
    slot = next;
  }

	return PF_PAGENOTINBUF;
}

//
// FlushPages
//
// Desc: Release all pages for this file and put them onto the free list
//       Returns a warning if any of the file's pages are pinned.
//       A linear search of the buffer is performed.
//       A better method is not needed because # of buffers are small.
// In:   fd - file descriptor
// Ret:  PF_PAGEPINNED or other PF return code
//
RC PF_BufferMgr::FlushPages(int fd)
{
  pStatisticsMgr->Register(PF_FLUSHPAGES, STAT_ADDONE);
  pStatisticsMgr->Register("FlushPage", STAT_ADDONE);
  for(int i = 0; i < numPages; i++)
  {
    if(bufTable[i].fd == fd)
    {
      if(bufTable[i].pinCount > 0) 
      {
        return PF_PAGEPINNED;
        
      }
    }
  }
  int slot = first, next;
  while(slot != INVALID_SLOT)
  {
    next = bufTable[slot].next;
    if(bufTable[slot].fd == fd)
    {
      if(bufTable[slot].bDirty == 1)
      {
        ForcePages(bufTable[slot].fd, bufTable[slot].pageNum);
      }
      Unlink(slot);
      InsertFree(slot);
      delete[] bufTable[slot].pData;
      hashTable.Delete(bufTable[slot].fd, bufTable[slot].pageNum);
      bufTable[slot].pData = NULL;
      bufTable[slot].bDirty = 0;
      bufTable[slot].pinCount = 0;
      bufTable[slot].pageNum = -1;
      bufTable[slot].fd = -1;
    }
    slot = next;
  }


	return OK_RC;
}

//
// ForcePages
//
// Desc: If a page is dirty then force the page from the buffer pool
//       onto disk.  The page will not be forced out of the buffer pool.
// In:   The page number, a default value of ALL_PAGES will be used if
//       the client doesn't provide a value.  This will force all pages.
// Ret:  Standard PF errors
//
//
RC PF_BufferMgr::ForcePages(int fd, PageNum pageNum)
{
  if(pageNum == ALL_PAGES)
  {
    int slot = first, next;
    while(slot != INVALID_SLOT)
    {
      next = bufTable[slot].next;
      if(bufTable[slot].bDirty == 1)
      {
        RC rc;
        if((rc = WritePage(fd, pageNum, bufTable[slot].pData)))
        {
          return rc;
        }
        bufTable[slot].bDirty = 0;
      }
      slot = next;
    }
    
    return OK_RC;
  }
  else
  {
    int slot = first, next;
    while(slot != INVALID_SLOT)
    {
      next = bufTable[slot].next;
      if(bufTable[slot].fd == fd && bufTable[slot].pageNum == pageNum)
      {
        if(bufTable[slot].bDirty == 1)
        {
          RC rc;
          if((rc = WritePage(fd, pageNum, bufTable[slot].pData)))
          {
            return rc;
          }
          bufTable[slot].bDirty = 0;
          return OK_RC;
        }
      }
      slot = next;
    }
  }
	return PF_PAGENOTINBUF;
}


//
// PrintBuffer
//
// Desc: Display all of the pages within the buffer.
//       This routine will be called via the system command.
// In:   Nothing
// Out:  Nothing
// Ret:  Always returns 0
//
RC PF_BufferMgr::PrintBuffer()
{
   cout << "Buffer contains " << numPages << " pages of size "
      << pageSize <<".\n";
   cout << "Contents in order from most recently used to "
      << "least recently used.\n";

   int slot, next;
   slot = first;
   while (slot != INVALID_SLOT) {
      next = bufTable[slot].next;
      cout << slot << " :: \n";
      cout << "  fd = " << bufTable[slot].fd << "\n";
      cout << "  pageNum = " << bufTable[slot].pageNum << "\n";
      cout << "  bDirty = " << bufTable[slot].bDirty << "\n";
      cout << "  pinCount = " << bufTable[slot].pinCount << "\n";
      slot = next;
   }

   if (first==INVALID_SLOT)
      cout << "Buffer is empty!\n";
   else
      cout << "All remaining slots are free.\n";

   return 0;
}


//
// ClearBuffer
//
// Desc: Remove all entries from the buffer manager.
//       This routine will be called via the system command and is only
//       really useful if the user wants to run some performance
//       comparison starting with an clean buffer.
// In:   Nothing
// Out:  Nothing
// Ret:  Will return an error if a page is pinned and the Clear routine
//       is called.
RC PF_BufferMgr::ClearBuffer()
{
   for(int i = 0; i < numPages; i++)
   {
      if(bufTable[i].pinCount > 0)
        return PF_PAGEPINNED;
   }
   for(int i = 0; i < numPages; i++)
   {
      delete[] bufTable[i].pData;
      hashTable.Delete(bufTable[i].fd, bufTable[i].pageNum);
      bufTable[i].pData = NULL;
      bufTable[i].next = -1;
      bufTable[i].prev = -1;
      bufTable[i].bDirty = 0;
      bufTable[i].pinCount = 0;
      bufTable[i].pageNum = -1;
      bufTable[i].fd = -1;
   }
   return OK_RC;
}

//
// ResizeBuffer
//
// Desc: Resizes the buffer manager to the size passed in.
//       This routine will be called via the system command.
// In:   The new buffer size
// Out:  Nothing
// Ret:  0 for success or,
//       Some other PF error (probably PF_NOBUF)
//
// Notes: This method attempts to copy all the old pages which I am
// unable to kick out of the old buffer manager into the new buffer
// manager.  This obviously cannot always be successfull!
//
RC PF_BufferMgr::ResizeBuffer(int iNewSize)
{
   if(iNewSize < pageSize)
   {
     return PF_NOBUF;
   }
   numPages = iNewSize;
   PF_BufPageDesc* temp = new PF_BufPageDesc[iNewSize]();
   for(int i = 0; i < iNewSize; i++)
   {
     temp[i].pData = bufTable[i].pData;
     temp[i].next = bufTable[i].next;
     temp[i].prev = bufTable[i].prev;
     temp[i].bDirty = bufTable[i].bDirty;
     temp[i].pinCount = bufTable[i].pinCount;
     temp[i].pageNum = bufTable[i].pageNum;
     temp[i].fd = bufTable[i].fd;
   }
   bufTable = temp;
   return OK_RC;
}


//
// InsertFree
//
// Desc: Internal.  Insert a slot at the head of the free list
// In:   slot - slot number to insert
// Ret:  PF return code
//
RC PF_BufferMgr::InsertFree(int slot)
{
   if(free == -1)
   {
     free = slot;
     bufTable[slot].next = -1;
     bufTable[slot].prev = -1;
   }
   else
   {
     bufTable[free].prev = slot;
     bufTable[slot].next = free;
     bufTable[slot].prev = -1;
     free = slot;
   }
   // Return ok
   return OK_RC;
}

//
// LinkHead
//
// Desc: Internal.  Insert a slot at the head of the used list, making
//       it the most-recently used slot.
// In:   slot - slot number to insert
// Ret:  PF return code
//
RC PF_BufferMgr::LinkHead(int slot)
{
   if(first == -1)
   {
     first = slot;
     last = slot;
     bufTable[slot].prev = -1;
     bufTable[slot].next = -1;
   }
   else
   {
    bufTable[first].prev = slot;
    bufTable[slot].next = first;
    bufTable[slot].prev = -1;
    first = slot;
   }
   // Return ok
   return OK_RC;
}

//
// Unlink
//
// Desc: Internal.  Unlink the slot from the used list.  Assume that
//       slot is valid.  Set prev and next pointers to INVALID_SLOT.
//       The caller is responsible to either place the unlinked page into
//       the free list or the used list.
// In:   slot - slot number to unlink
// Ret:  PF return code
//
RC PF_BufferMgr::Unlink(int slot)
{

  int prev = bufTable[slot].prev;
  int next = bufTable[slot].next;
  if(prev == -1 && next == -1)
  {
    first = -1;
    last = -1;
  }
  else if (prev == -1)
  {
    bufTable[next].prev = -1;
    bufTable[slot].next = -1;
    first = next;
  }
  else if (next == -1)
  {
    bufTable[prev].next = -1;
    bufTable[slot].prev = -1;
    last = prev;
  }
  else
  {
    bufTable[prev].next = next;
    bufTable[next].prev = prev;
    bufTable[slot].prev = -1;
    bufTable[slot].next = -1;
  }
   // Return ok
   return OK_RC;
}

//
// ReadPage
//
// Desc: Read a page from disk
//
// In:   fd - OS file descriptor
//       pageNum - number of page to read
//       dest - pointer to buffer in which to read page
// Out:  dest - buffer contains page contents
// Ret:  PF return code
//
RC PF_BufferMgr::ReadPage(int fd, PageNum pageNum, char *dest)
{
  pStatisticsMgr->Register(PF_READPAGE, STAT_ADDONE);
  pStatisticsMgr->Register("ReadPage", STAT_ADDONE);
  lseek(fd, pageSize * pageNum + sizeof(PF_FileHdr), SEEK_SET);
  if(read(fd, dest, pageSize) < 0){
    cout << "Some Error in Read : pf_buffermgr.cc:565" << endl;
    return PF_INCOMPLETEREAD;
  }
    return OK_RC;
}

//
// WritePage
//
// Desc: Write a page to disk
//
// In:   fd - OS file descriptor
//       pageNum - number of page to write
//       dest - pointer to buffer containing page contents
// Ret:  PF return code
//
RC PF_BufferMgr::WritePage(int fd, PageNum pageNum, char *source)
{
  pStatisticsMgr->Register(PF_WRITEPAGE, STAT_ADDONE);
  pStatisticsMgr->Register("WritePage", STAT_ADDONE);
  lseek(fd, pageSize * pageNum + sizeof(PF_FileHdr), SEEK_SET);
  if(write(fd, source, pageSize) < 0)
  {
    cout << "Some Error in Write : pf_buffermgr.cc:588" << endl;
    return PF_INCOMPLETEWRITE;
  }
  return OK_RC;
}

//
// InitPageDesc
//
// Desc: Internal.  Initialize PF_BufPageDesc to a newly-pinned page
//       for a newly pinned page
// In:   fd - file descriptor
//       pageNum - page number
// Ret:  PF return code
//
RC PF_BufferMgr::InitPageDesc(int fd, PageNum pageNum, int slot)
{
   bufTable[slot].bDirty = 0;
   bufTable[slot].pData = new char[pageSize];
   bufTable[slot].pinCount++;
   bufTable[slot].fd = fd;
   bufTable[slot].pageNum = pageNum;
   hashTable.Insert(fd, pageNum, slot);
   // Return ok
   return OK_RC;
}

//------------------------------------------------------------------------------
// Methods for manipulating raw memory buffers
//------------------------------------------------------------------------------

#define MEMORY_FD -1

//
// GetBlockSize
//
// Return the size of the block that can be allocated.  This is simply
// just the size of the page since a block will take up a page in the
// buffer pool.
//
RC PF_BufferMgr::GetBlockSize(int &length) const
{
   length = numPages;
   return OK_RC;
}


//
// AllocateBlock
//
// Allocates a page in the buffer pool that is not associated with a
// particular file and returns the pointer to the data area back to the
// user.
//
RC PF_BufferMgr::AllocateBlock(char *&buffer)
{
   static int pageNum = 1;
   RC rc;
   if(rc = AllocatePage(MEMORY_FD, pageNum, &buffer))
     return rc;
   pageNum++;
   return OK_RC;
}

//
// DisposeBlock
//
// Free the block of memory from the buffer pool.
//
RC PF_BufferMgr::DisposeBlock(char* buffer)
{
  int slot = first, next;
  while(slot != INVALID_SLOT)
  {
    next = bufTable[slot].next;
    if(bufTable[slot].pData == buffer)
    {
      Unlink(slot);
      InsertFree(slot);
      bufTable[slot].pData = NULL;
      bufTable[slot].bDirty = 0;
      bufTable[slot].pinCount = 0;
      bufTable[slot].pageNum = -1;
      bufTable[slot].fd = -1;
      return OK_RC;
    }
    slot = next;
  }
	return PF_PAGENOTINBUF;
}
