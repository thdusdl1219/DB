#include "rm.h"
#include "rm_internal.h"

RM_Record::RM_Record() {
}

RM_Record::~RM_Record() {
}

RC RM_Record::GetData(char *&pData) const {
  if(pData == NULL)
    return RM_NULL;
  pData = this->pData;
  return (0);
}

RC RM_Record::GetRid(RID &rid) const {
  rid.pn = this->rid->pn;
  rid.sn = this->rid->sn;
  return (0);
}
