#include "ix_bptree.h"

IX_IndexHandle::IX_IndexHandle() {
  bHdrChanged = FALSE;
  memset(&fileHdr, 0, sizeof(fileHdr));
  fileHdr.firstFree = IX_PAGE_LIST_END;
  intt = NULL;
  floatt = NULL;
  chart = NULL;
}

IX_IndexHandle::~IX_IndexHandle() {

}

RC IX_IndexHandle::InsertEntry(void *pData, const RID &rid) {
  RC rc;
  PF_PageHandle pageHandle;
  IX_BpTreeEntry<int> e1;
  IX_BpTreeEntry<float> e2;
  IX_BpTreeEntry<char> e3;
  IX_BpTreeEntry<int> *c1 = NULL;
  IX_BpTreeEntry<float> *c2 = NULL;
  IX_BpTreeEntry<char> *c3 = NULL;

  if(pData == NULL)
    return IX_NULLPOINTER;


  char* newData = new char[fileHdr.attrLength];
  memcpy(newData, pData, fileHdr.attrLength);


  switch(fileHdr.attrType) {
    case INT:
      if(intt == NULL)
        intt = new IX_BpTree<int>(&this->pfFileHandle, this->fileHdr.attrType, this->fileHdr.attrLength);
      e1.rid = rid;
      e1.key = (int*) newData;
      if((rc = intt->Insert(NULL, &e1, &c1)))
        return rc;
      break;
    case FLOAT:
      if(floatt == NULL)
        floatt = new IX_BpTree<float>(&this->pfFileHandle, this->fileHdr.attrType, this->fileHdr.attrLength);
      e2.rid = rid;
      e2.key = (float*) newData;
      if((rc = floatt->Insert(NULL, &e2, &c2)))
        return rc;
      break;
    case STRING:
      if(chart == NULL)
        chart = new IX_BpTree<char>(&this->pfFileHandle, this->fileHdr.attrType, this->fileHdr.attrLength);
      e3.rid = rid;
      e3.key = (char*) newData;
      if((rc = chart->Insert(NULL, &e3, &c3)))
        return rc;
      break;
  }
  return (0);
}

RC IX_IndexHandle::DeleteEntry(void *pData, const RID &rid) {
  RC rc;
  PF_PageHandle pageHandle;
  IX_BpTreeEntry<int> e1;
  IX_BpTreeEntry<float> e2;
  IX_BpTreeEntry<char> e3;
  IX_BpTreeEntry<int> *c1 = NULL;
  IX_BpTreeEntry<float> *c2 = NULL;
  IX_BpTreeEntry<char> *c3 = NULL;

  if(pData == NULL)
    return IX_NULLPOINTER;


  char* newData = new char[fileHdr.attrLength];
  memcpy(newData, pData, fileHdr.attrLength);


  switch(fileHdr.attrType) {
    case INT:
      if(intt == NULL)
        intt = new IX_BpTree<int>(&this->pfFileHandle, this->fileHdr.attrType, this->fileHdr.attrLength);
      e1.rid = rid;
      e1.key = (int*) newData;
      if((rc = intt->Delete(NULL, &e1, &c1)))
        return rc;
      break;
    case FLOAT:
      if(floatt == NULL)
        floatt = new IX_BpTree<float>(&this->pfFileHandle, this->fileHdr.attrType, this->fileHdr.attrLength);
      e2.rid = rid;
      e2.key = (float*) newData;
      if((rc = floatt->Delete(NULL, &e2, &c2)))
        return rc;
      break;
    case STRING:
      if(chart == NULL)
        chart = new IX_BpTree<char>(&this->pfFileHandle, this->fileHdr.attrType, this->fileHdr.attrLength);
      e3.rid = rid;
      e3.key = (char*) newData;
      if((rc = chart->Delete(NULL, &e3, &c3)))
        return rc;
      break;
  }
  return (0);

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
