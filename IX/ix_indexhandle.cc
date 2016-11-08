#include "ix_internal.h"
#include "ix_bptree.h"

IX_IndexHandle::IX_IndexHandle() {
  bHdrChanged = FALSE;
  memset(&fileHdr, 0, sizeof(fileHdr));
  fileHdr.firstFree = IX_PAGE_LIST_END;
}

IX_IndexHandle::~IX_IndexHandle() {

}

RC IX_IndexHandle::InsertEntry(void *pData, const RID &rid) {
  PF_PageHandle pageHandle;
  IX_BpTreeEntry<int> e1;
  IX_BpTreeEntry<float> e2;
  IX_BpTreeEntry<char> e3;
  IX_BpTree<int> *a;
  IX_BpTree<float> *b;
  IX_BpTree<char> *c;

  if(pData == NULL)
    return IX_NULLPOINTER;


  switch(fileHdr.attrType) {
    case INT:
      a = new IX_BpTree<int>(this->pfFileHandle, this->fileHdr.attrType, this->fileHdr.attrLength);
      e1.rid = rid;
      e1.key = (int*) pData;
      a->Insert(NULL, &e1, NULL); 
      break;
    case FLOAT:
      b = new IX_BpTree<float>(this->pfFileHandle, this->fileHdr.attrType, this->fileHdr.attrLength);
      e2.rid = rid;
      e2.key = (float*) pData;
      b->Insert(NULL, &e2, NULL);
      break;
    case STRING:
      c = new IX_BpTree<char>(this->pfFileHandle, this->fileHdr.attrType, this->fileHdr.attrLength);
      e3.rid = rid;
      e3.key = (char*) pData;
      c->Insert(NULL, &e3, NULL);
      break;
  }
  return (0);
}

RC IX_IndexHandle::DeleteEntry(void *pData, const RID &rid) {
  return (0);
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
