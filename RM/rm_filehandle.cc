#include "rm.h"
#include "rm_internal.h"
#include <iostream>
using namespace std;

#define HEADER_SIZE (sizeof(RM_PageHdr) + bitmapSize * sizeof(int)) 

#ifdef RM_LOG

//
// WriteLog
//
// This is a self contained unit that will create a new log file and send
// psMessage to the log file.  Notice that I do not close the file fLog at
// any time.  Hopefully if all goes well this will be done when the program
// exits.
//
void WriteLog(const char *psMessage)
{
   static FILE *fLog = NULL;

   // The first time through we have to create a new Log file
   if (fLog == NULL) {
      // This is the first time so I need to create a new log file.
      // The log file will be named "PF_LOG.x" where x is the next
      // available sequential number
      int iLogNum = -1;
      int bFound = FALSE;
      char psFileName[10];

      while (iLogNum < 999 && bFound==FALSE) {
         iLogNum++;
         sprintf (psFileName, "RM_LOG.%d", iLogNum);
         fLog = fopen(psFileName,"r");
         if (fLog==NULL) {
            bFound = TRUE;
            fLog = fopen(psFileName,"w");
         } //else
            // delete fLog;
      }

      if (!bFound) {
         cerr << "Cannot create a new log file!\n";
         exit(1);
      }
   }
   // Now we have the log file open and ready for writing
   fprintf (fLog, psMessage);
   cout << psMessage << endl;
}
#endif


RM_FileHandle::RM_FileHandle() {
  pfh = NULL;
  recordNum = -1;
  bitmapSize = -1;
}

RM_FileHandle::~RM_FileHandle() {
}

RC RM_FileHandle::GetRec(const RID &rid, RM_Record &rec) const {
  if(pfh == NULL)
    return RM_OPENFILE;
  SlotNum sn;
  PageNum pn;
  rid.GetSlotNum(sn);
  rid.GetPageNum(pn);
  PF_PageHandle pf;
#ifdef RM_LOG
   char psMessage[100];
   sprintf (psMessage, "GetRec. (page, slot) : (%d , %d)\n",
         pn, sn);
   WriteLog(psMessage);
#endif
  int rc;
  char* pData;
  if((rc = GetData(pn, pData))) {
    return rc;
  }

  if(isEmpty(GetMap(pData))) {
    return RM_PAGEEMPTY;
  }

  int offset = HEADER_SIZE + hdr.recordSize * sn;
  char* myData = new char[hdr.recordSize];
  memcpy(myData, pData + offset, hdr.recordSize);
  rec.pData = myData;
  rec.rid = new RID(pn, sn);

  pfh->UnpinPage(pn);
  return (0);
}

RC RM_FileHandle::InsertRec(const char *pData, RID &rid) {
  PF_PageHandle pf;
  int rc;
  PageNum pn;
  char* pageData;
  if(hdr.firstFree == RM_PAGE_LIST_END) {
    hdr.numPage++;
    pfh->AllocatePage(pf);
    pf.GetPageNum(pn);
    if((rc = pf.GetData(pageData)))
      return rc;
    hdr.firstFree = pn;
    RM_PageHdr* rph = (RM_PageHdr *) pageData;
    rph->nextFree = RM_PAGE_LIST_END;
  }
  else { 
    pn = hdr.firstFree;
    if((rc = GetData(pn, pageData))) {
      return rc;
    }
  }
  int* map = GetMap(pageData);
  int index = FindFree(map);

  if(index == -1)
    return RM_OFFSET;

  SetBit(map, index);

  int offset = HEADER_SIZE + (index * hdr.recordSize);
  memcpy(pageData + offset, pData, hdr.recordSize);

  if(isFull(map)) {
    RM_PageHdr* rph = (RM_PageHdr *)pageData;
    hdr.firstFree = rph->nextFree;
    rph->nextFree = RM_PAGE_FULL;
  }
/* #ifdef RM_LOG
   sprintf (psMessage, "InsertRec. (pageData + offset, pData, recordSize) : (%x, %x, %d)\n",
         pageData + offset, pData, hdr.recordSize);
   WriteLog(psMessage);
#endif */
  rid.pn = pn;
  rid.sn = index;
#ifdef RM_LOG
   char psMessage[100];
   sprintf (psMessage, "InsertRec. (page, slot, data) : (%d , %d , %s)\n",
         pn, index, pData);
   WriteLog(psMessage);
#endif
  pfh->UnpinPage(pn);
  pfh->MarkDirty(pn);
  return (0);  
}

RC RM_FileHandle::DeleteRec(const RID &rid) {
  SlotNum sn;
  PageNum pn;
  rid.GetSlotNum(sn);
  rid.GetPageNum(pn);
#ifdef RM_LOG
   char psMessage[100];
   sprintf (psMessage, "DeleteRec. (page, slot) : (%d , %d)\n",
         pn, sn);
   WriteLog(psMessage);
#endif
  char *pData;
  int rc;
  if((rc = GetData(pn, pData)))
    return rc;
  int* map = GetMap(pData);
#ifdef RM_LOG
   sprintf (psMessage, "DeleteRec. (map[0], map[1], map[2], map[3]) : (%x, %x, %x, %x)\n",
         map[0], map[1], map[2], map[3]);
   WriteLog(psMessage);
#endif
  if(!GetBit(map, sn)) {
    return RM_RECNOTIN;
  }
  
  int offset = HEADER_SIZE + sn * hdr.recordSize;
  memset(pData + offset, 0, hdr.recordSize);
  UnsetBit(map, sn);
  RM_PageHdr* rph = (RM_PageHdr *)pData;
  if(rph->nextFree == RM_PAGE_FULL) {
    if(hdr.firstFree == pn) {
      rph->nextFree = RM_PAGE_LIST_END;
    }
    else {
      rph->nextFree = hdr.firstFree;
      hdr.firstFree = pn;
    }
    pfh->MarkDirty(pn);
  }
  pfh->UnpinPage(pn);
  return (0);
}

RC RM_FileHandle::UpdateRec(const RM_Record &rec) {
  RID rid;
  char* recData;
  rec.GetData(recData);
  rec.GetRid(rid);
  SlotNum sn;
  PageNum pn;
  rid.GetPageNum(pn);
  rid.GetSlotNum(sn);
  char *pData;
  int rc;
  if((rc = GetData(pn, pData)))
    return rc;
#ifdef RM_LOG
   char psMessage[100];
   sprintf (psMessage, "UpdataRec. (page, slot, recData) : (%d , %d, %s)\n",
         pn, sn, recData);
   WriteLog(psMessage);
#endif
  int* map = GetMap(pData);
#ifdef RM_LOG
   sprintf (psMessage, "UpdateRec. (map[0], map[1], map[2], map[3]) : (%x, %x, %x, %x)\n",
         map[0], map[1], map[2], map[3]);
   WriteLog(psMessage);
#endif
  if(!GetBit(map, sn)) {
    return RM_RECNOTIN;
  }

  int offset = HEADER_SIZE + sn * hdr.recordSize;
  memcpy(pData + offset, recData, hdr.recordSize);
  pfh->MarkDirty(pn);
  pfh->UnpinPage(pn);
  
  return (0);
}

RC RM_FileHandle::ForcePages(PageNum pageNum) {
  
  lseek(fd, sizeof(PF_FileHdr), L_SET);
  int numBytes;
  if((numBytes = write(fd, &hdr, sizeof(RM_FileHdr))) != sizeof(RM_FileHdr)) {
    return PF_UNIX; 
  }

  int rc;
  if((rc = pfh->ForcePages(pageNum)))
    return rc;

  return (0);
}

RC RM_FileHandle::GetData(PageNum pn, char *&buf) const {
  PF_PageHandle pf;
  int rc;
  if((rc = pfh->GetThisPage(pn, pf)))
    return rc;
  char* pData;
  if((rc = pf.GetData(pData)))
    return rc;
  buf = pData;
  return (0);
}

int* RM_FileHandle::GetMap(char* data) const {
  int* map = (int *)(data + sizeof(RM_PageHdr));
  // memcpy(map, data + sizeof(RM_PageHdr), bitmapSize);
  return map;
}

int RM_FileHandle::FindFree(int* map) const {
  for(int i = 0; i < bitmapSize; i++) {
    if(map[i] != 0xffffffff) {
      for(int j = 0; j < 8 * sizeof(int); j++) {
        int index = i * 8 * sizeof(int) + j; 
        if(!GetBit(map, index)) {
          if(index >= recordNum)
            return -1;
          else {
            cout << "index : " << index << endl;
            return index; 
          }
        }
      }
    }
  }
  return -1;
}

int RM_FileHandle::GetBit(int* map, int index) const {
  int i = index / (8 * sizeof(int));
  int j = index % (8 * sizeof(int));
  return (map[i] >> j) & 0x1;
}

void RM_FileHandle::SetBit(int* map, int index) {
  int i = index / (8 * sizeof(int));
  int j = index % (8 * sizeof(int));
  map[i] = map[i] | (0x1 << j);
}

void RM_FileHandle::UnsetBit(int* map, int index) {
  int i = index / (8 * sizeof(int));
  int j = index % (8 * sizeof(int));
  map[i] = map[i] & (0xffffffff ^ (0x1 << j));
}

void RM_FileHandle::XorBit(int* map, int index) {
  int i = index / (8 * sizeof(int));
  int j = index % (8 * sizeof(int));
  map[i] = map[i] ^ (0x1 << j);
}

bool RM_FileHandle::isEmpty(int* map) const {
  for(int i = 0; i < bitmapSize; i++) {
    if(map[i] != 0) {
      return false;
    }
  }
  return true;
}

bool RM_FileHandle::isFull(int* map) const {
  if(FindFree(map) == -1)
    return 1;
  return 0;
}
