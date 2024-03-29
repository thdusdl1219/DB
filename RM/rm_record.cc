#include "rm.h"
#include "rm_internal.h"

RM_Record::RM_Record() {
  pData = NULL;
  rid = new RID();
}

RM_Record::~RM_Record() {
  delete [] pData;
  delete rid;
}

RC RM_Record::GetData(char *&pData) const {
  if(this->pData == NULL)
    return RM_NODATA;
  pData = this->pData;
  return (0);
}

RC RM_Record::GetRid(RID &rid) const {
  rid.pn = this->rid->pn;
  rid.sn = this->rid->sn;
  return (0);
}

#ifdef RM_LOG
RC RM_Record::SetRid(RID &rid)  {
  this->rid->pn = rid.pn;
  this->rid->sn = rid.sn;
  return (0);
}

RC RM_Record::SetData(char *pData)  {
  this->pData = pData;
  return (0);
}
#endif
