#include "ix_internal.h"

IX_IndexHandle::IX_IndexHandle() {
  bHdrChanged = FALSE;
  memset(&fileHdr, 0, sizeof(fileHdr));
  fileHdr.firstFree = IX_PAGE_LIST_END;
}

IX_IndexHandle::~IX_IndexHandle() {

}

RC IX_IndexHandle::InsertEntry(void *pData, const RID &rid) {
  RC rc;
  PageNum pageNum;
  SlotNum slotNum;
  PF_PageHandle pageHandle;
  char *tmpData;
  RID *pRid;

  if(pData == NULL)
    return IX_NULLPOINTER;


}

RC IX_IndexHandle::DeleteEntry(void *pData, const RID &rid) {
}

RC IX_IndexHandle::ForcePages() {
  RC rc;

  if(bHdrChanged) {
    PF_PageHandle pageHandle;
    char* pData;

    if((rc = pfFileHandle.GetFirstPage(pageHandle)))
      goto err_return;

    if((rc = pageHandle.GetData(pData)))
      goto err_unpin;

    memcpy(pData, &fileHdr, sizeof(fileHdr));

    if((rc = pfFileHandle.MarkDirty(IX_HEADER_PAGE_NUM)))
      goto err_unpin;
    
    if((rc = pfFileHandle.UnpinPage(IX_HEADER_PAGE_NUM)))
      goto err_return;
    
    bHdrChanged = FALSE;
  }

  if((rc = pfFileHandle.ForcePages()))
    goto err_return;

  return (0);

err_unpin:
  pfFileHandle.UnpinPage(IX_HEADER_PAGE_NUM);
err_return:
  return (rc);
}
