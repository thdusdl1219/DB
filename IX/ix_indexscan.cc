#include "ix_internal.h"
#include "ix_bptree.h"

IX_IndexScan::IX_IndexScan() {
  bScanOpen = FALSE;
  curPageNum = IX_HEADER_PAGE_NUM;
  curSlotNum = 0;

  pIndexHandle = NULL;
  compOp = NO_OP;
  value = NULL;
}

IX_IndexScan::~IX_IndexScan() {
}

RC IX_IndexScan::OpenScan(const IX_IndexHandle &indexHandle, CompOp compOp, void *value, ClientHint pinHint) {
  RC rc;
  if(bScanOpen)
    return IX_SCANOPEN;

  if(indexHandle.fileHdr.attrLength == 0) //tricky solution
    return IX_CLOSEDFILE;

  switch (compOp) {
   case EQ_OP:
   case LT_OP:
   case GT_OP:
   case LE_OP:
   case GE_OP:
   case NE_OP:
   case NO_OP:
      break;

   default:
      return (IX_INVALIDCOMPOP);
   }

  if(compOp != NO_OP) {
    if(value == NULL)
      return IX_NULLPOINTER;
  }

  pIndexHandle = (IX_IndexHandle *)&indexHandle;
  this->compOp = compOp;
  this->value = value;
  this->attrLength = pIndexHandle->fileHdr.attrLength;
  this->intt = indexHandle.intt;
  this->floatt = indexHandle.floatt;
  this->chart = indexHandle.chart;

  bScanOpen = TRUE;

  //return ok
  return (0);
}

RC IX_IndexScan::GetNextEntry(RID &rid) {
  RC rc;
  PF_PageHandle pageHandle;
  char *pData;
  RID ret;
  int thispageNum;

  if(!bScanOpen)
    return IX_CLOSEDSCAN;

  if(pIndexHandle->fileHdr.attrLength == 0)
    return IX_CLOSEDFILE;

  if(curPageNum == IX_NODE_END)
    return IX_EOF;

  printf("curPageNum, curSlotNum: %d, %d\n", curPageNum, curSlotNum);
  if(curPageNum == IX_HEADER_PAGE_NUM) {
    RID rid;
    if((rc = Find(rid)))
      return rc;
    rid.GetPageNum(curPageNum);
    rid.GetSlotNum(curSlotNum);
  }

  thispageNum = curPageNum;
  printf("curPageNum, curSlotNum: %d, %d\n", curPageNum, curSlotNum);
  if((rc = pIndexHandle->pfFileHandle.GetThisPage(thispageNum, pageHandle)))
    goto err_return;
  printf("rc1 : %d\n", rc);

  if((rc = pageHandle.GetData(pData)))
    goto err_unpin;


  if((rc = Operation(rid, pData)))
    goto err_unpin;

  printf("curPageNum: %d\n", curPageNum);
  if((rc = pIndexHandle->pfFileHandle.UnpinPage(thispageNum)))
    goto err_return;
  printf("rc2 : %d\n", rc);

  return (0);
err_unpin:
  pIndexHandle->pfFileHandle.UnpinPage(thispageNum);
err_return:
  return (rc);
}

RC IX_IndexScan::CloseScan() {
  if(!bScanOpen)
    return IX_CLOSEDSCAN;

  bScanOpen = FALSE;
  curPageNum = IX_HEADER_PAGE_NUM;
  curSlotNum = 0;
  pIndexHandle = NULL;
  compOp = NO_OP;
  value = NULL;
  attrLength = sizeof(int);
  intt = NULL;
  floatt = NULL;
  chart = NULL;
  
  return (0);
}

RC IX_IndexScan::Operation(RID& rid, char* pData) {
  RID ret;
  IX_BpTreeNodeHdr nodeHdr;
  memcpy(&nodeHdr, pData, IX_BPTREE_HEADER_SIZE);
  int c = 0;
  switch(compOp) {
    case EQ_OP:
      c = Compare(pData + curSlotNum * IX_BPTREE_ENTRY_SIZE + IX_BPTREE_HEADER_SIZE, value);
      if(c != 2)
        return IX_EOF;
      memcpy(&ret, pData + attrLength + curSlotNum * IX_BPTREE_ENTRY_SIZE + IX_BPTREE_HEADER_SIZE, sizeof(RID));
      rid = ret;
      curSlotNum++;
      if(curSlotNum == nodeHdr.count) {
        curPageNum = nodeHdr.next;
      }
      else if(curSlotNum > nodeHdr.count) {
        return IX_SOMETHINGWRONG;
      }
      break;
    case LT_OP:
      break;
    case GT_OP:
      break;
    case LE_OP:
      break;
    case NO_OP:
    case GE_OP:  
      memcpy(&ret, pData + attrLength + curSlotNum * IX_BPTREE_ENTRY_SIZE + IX_BPTREE_HEADER_SIZE, sizeof(RID));
      rid = ret;
      curSlotNum++;
      if(curSlotNum == nodeHdr.count) {
        curPageNum = nodeHdr.next;
      }
      else if(curSlotNum > nodeHdr.count) {
        return IX_SOMETHINGWRONG;
      }
      break;
    case NE_OP:
      return IX_NEOP;
  }

  return (0);
}

int IX_IndexScan::Compare(void* key1, void* key2) {
  float cmp = 0;
  switch(pIndexHandle->fileHdr.attrType) {
    case INT:
      cmp = *(int *)key1 - *(int *)key2;
      break;
    case FLOAT:
      cmp = *(float *)key1 - *(float *)key2;
      break;
    case STRING:
      cmp = memcmp(key1, key2, attrLength);
      break;
  }
  if(cmp > 0)
    return 1;
  if(cmp == 0)
    return 2;
  return 0;
}

RC IX_IndexScan::Find(RID& rid) {
  RC rc;
  switch(pIndexHandle->fileHdr.attrType) {
    case INT:
      if((rc = intt->Find((int *) value, rid)))
        return rc;
      break;
    case FLOAT:
      if((rc = floatt->Find((float *) value, rid)))
        return rc;
      break;
    case STRING:
      if((rc = chart->Find((char *) value, rid)))
        return rc;
      break;
  }
  return (0);
}
