#include "ix_internal.h"
#include "ix_bptree.h"

IX_IndexScan::IX_IndexScan() {
  bScanOpen = FALSE;

  pIndexHandle = NULL;
  compOp = NO_OP;
  value = NULL;
}

IX_IndexScan::~IX_IndexScan() {
}

RC IX_IndexScan::OpenScan(const IX_IndexHandle &indexHandle, CompOp compOp, void *value, ClientHint pinHint) {
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
  
  bScanOpen = TRUE;

  return (0);
}

RC IX_IndexScan::GetNextEntry(RID &rid) {
  RC rc;
  PF_PageHandle pageHandle;
  char *pData;
  RID *curRid;

  if(!bScanOpen)
    return IX_CLOSEDSCAN;

  if(pIndexHandle->fileHdr.attrLength == 0)
    return IX_CLOSEDFILE;



  return (0);
}

RC IX_IndexScan::CloseScan() {
  return (0);
}

RC IX_IndexScan::Find(void *value) {
  RC rc;
  switch(pIndexHandle->fileHdr.attrType) {
    case INT:
      if((rc = intt->Find((int *) value)))
        return rc;
      break;
    case FLOAT:
      if((rc = floatt->Find((float *) value)))
        return rc;
      break;
    case STRING:
      if((rc = chart->Find((char *) value)))
        return rc;
      break;
  }
  return (0);
}
