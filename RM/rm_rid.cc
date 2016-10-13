#include "rm_rid.h"
#include "rm_internal.h"

RID::RID() {
  pn = -1;
  sn = -1;
}

RID::RID(PageNum pageNum, SlotNum slotNum) {
  pn = pageNum;
  sn = slotNum;
}

RID::~RID() {
}

RC RID::GetPageNum(PageNum &pageNum) const {
  if (pn == -1) 
    return RM_RIDPAGEINVALID;
  pageNum = pn; 
  return (0);
}

RC RID::GetSlotNum(SlotNum &slotNum) const {
  if(sn == -1)
    return RM_RIDSLOTINVALID;
  slotNum = sn;
  return (0);
}
