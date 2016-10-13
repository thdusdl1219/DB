#include "rm.h"
#include "rm_internal.h"

RM_FileScan::RM_FileScan() {
  first = NULL;
}

RM_FileScan::~RM_FileScan() {
  if(first != NULL)
    delete first;
}

RC RM_FileScan::OpenScan( const RM_FileHandle &fileHandle, AttrType attrType, int attrLength, int attrOffset, CompOp compOp, void* value, ClientHint pinHint) {
  RC rc;
  if((rc = CheckType(attrType, attrLength)))
    return rc;

  for(int i = 0; i < fileHandle.totalRecordNum; i++) {
    int pn = i / fileHandle.recordNum;
    int sn = i % fileHandle.recordNum;
    RID rid(pn, sn);
    RM_Record rec;
    if((rc = fileHandle.GetRec(rid, rec)))
      return rc;
    char* recData;
    if((rc = rec.GetData(recData)))
      return rc;
    char* tmp = new char[attrLength];
    memcpy(tmp, recData + attrOffset, attrLength);
    bool result = false;
    result = Operating(attrType, compOp, tmp, value, attrLength);
    
    if(result) {
      struct ScanRecord* tmpfirst = first;
      first = new ScanRecord();
      first->cur.rid = new RID(pn, sn);
      first->cur.pData = recData;
      first->next = tmpfirst; 
    }
  }
  return (0);
}

RC RM_FileScan::GetNextRec( RM_Record &rec) {
  RC rc;
  if(first == NULL)
    return RM_EOF;

  if((rc = first->cur.GetData(rec.pData)))
    return rc;
  if((rc = first->cur.GetRid(*rec.rid)))
    return rc;

  first = first->next;
  return (0);
}

RC RM_FileScan::CloseScan() {
  delete [] first;
  first = NULL;
}

RC RM_FileScan::CheckType(AttrType attrType, int attrLength) {
  switch(attrType) {
    case INT :
      if(attrLength != 4)
        return RM_TYPEERROR;
      break;
    case FLOAT :
      if(attrLength != 4)
        return RM_TYPEERROR;
      break;
    case STRING :
      if(attrLength < 1 || attrLength > MAXSTRINGLEN)
        return RM_TYPEERROR;
      break;
  }
  return (0);
}

bool RM_FileScan::Operating(AttrType attrType, CompOp compOp, void* value1, void* value2, int n) {
  bool result = false;
  switch(compOp) {
    case NO_OP :
      result = true;
      break;
    case EQ_OP :
      result = Equal(attrType, value1, value2, n);
      break;
    case NE_OP :
      result = !Equal(attrType, value1, value2, n);
      break;
    case LT_OP :
      result = Less(attrType, value1, value2, n);
      break;
    case GT_OP :
      result = Greater(attrType, value1, value2, n);
      break;
    case LE_OP :
      result = (Less(attrType, value1, value2, n) || Equal(attrType, value1, value2, n));
      break;
    case GE_OP :
      result = (Greater(attrType, value1, value2, n) || Equal(attrType, value1, value2, n));
      break;
  }
  return result;
}

bool RM_FileScan::Equal(AttrType attrType, void* value1, void* value2, int n) {
  bool result = false;
  switch(attrType) {
    case INT :
      if(*((int *)value1) == *((int *)value2))
        result = true;
      break;
    case FLOAT :
      if(*((float *)value1) == *((float *)value2))
        result = true;
      break;
    case STRING :
      if(!memcmp(value1, value2, n))
        result = true;
      break;
  }
  return result;
}

bool RM_FileScan::Less(AttrType attrType, void* value1, void* value2, int n) {
  bool result = false;
  switch(attrType) {
    case INT :
      if(*((int *)value1) < *((int *)value2))
        result = true;
      break;
    case FLOAT :
      if(*((float *)value1) < *((float *)value2))
        result = true;
      break;
    case STRING :
      if(memcmp(value1, value2, n) < 0)
        result = true;
      break;
  }
  return result;
}

bool RM_FileScan::Greater(AttrType attrType, void* value1, void* value2, int n) {
  bool result = false;
  switch(attrType) {
    case INT :
      if(*((int *)value1) > *((int *)value2))
        result = true;
      break;
    case FLOAT :
      if(*((float *)value1) > *((float *)value2))
        result = true;
      break;
    case STRING :
      if(memcmp(value1, value2, n) > 0)
        result = true;
      break;
  }
  return result;
}
