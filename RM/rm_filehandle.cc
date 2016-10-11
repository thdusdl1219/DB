#include "rm.h"
#include "rm_internal.h"
#include <iostream>
using namespace std;

#define HEADER_SIZE (sizeof(RM_PageHdr) + bitmapSize) 

RM_FileHandle::RM_FileHandle() {
  pfh = NULL;

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
  rec.rid = rid;

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
    cout << "pn : " << pn << endl;
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
  rid.pn = pn;
  rid.sn = index;
  pfh->UnpinPage(pn);
  return (0);  
}

RC RM_FileHandle::DeleteRec(const RID &rid) {
  SlotNum sn;
  PageNum pn;
  rid.GetSlotNum(sn);
  rid.GetPageNum(pn);
  char *pData;
  int rc;
  if((rc = GetData(pn, pData)))
    return rc;
  int* map = GetMap(pData);
  if(!GetBit(map, sn)) {
    return RM_RECNOTIN;
  }
  
  int offset = HEADER_SIZE + sn * hdr.recordSize;
  memset(pData + offset, 0, hdr.recordSize);
  UnsetBit(map, sn);
  if(isEmpty(map)) {
    RM_PageHdr* rph = (RM_PageHdr *)pData;
    rph->nextFree = hdr.firstFree;
    hdr.firstFree = pn;
  }
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
  int* map = GetMap(pData);
  if(!GetBit(map, sn)) {
    return RM_RECNOTIN;
  }

  int offset = HEADER_SIZE + sn * hdr.recordSize;
  memcpy(pData + offset, recData, hdr.recordSize);
  
  return (0);
}

RC RM_FileHandle::ForcePages(PageNum pageNum) {
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
  int* map = new int[bitmapSize];
  memcpy(map, data + sizeof(RM_PageHdr), bitmapSize);
  return map;
}

int RM_FileHandle::FindFree(int* map) const {
  for(int i = 0; i < bitmapSize; i++) {
    if(map[i] != 0xffffffff) {
      for(int j = 0; j < 8 * sizeof(int); j++) {
        if(!GetBit(map, i * 8 * sizeof(int) + j)) {
          int index = i * 8 * sizeof(int) + j; 
          if(index > recordNum)
            return -1;
          else
            return index; 
        }
      }
    }
  }
  return -1;
}

int RM_FileHandle::GetBit(int* map, int index) const {
  int i = index / (8 * sizeof(int));
  int j = index % (8 * sizeof(int));
  return map[i] & (0x1 << j);
}

void RM_FileHandle::SetBit(int* map, int index) {
  int i = index / (8 * sizeof(int));
  int j = index % (8 * sizeof(int));
  map[i] = map[i] | (0x1 << j);
}

void RM_FileHandle::UnsetBit(int* map, int index) {
  int i = index / (8 * sizeof(int));
  int j = index % (8 * sizeof(int));
  map[i] = map[i] & (0x1 << j);
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
