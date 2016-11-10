
#include <cstdio>
#include <iostream>
#include "ix_internal.h"

using namespace std;

static const char *IX_WarnMsg[] = {
  (char *)"attrLength doesn't match with attrType",
  (char *)"pData is NULL",
  (char *)"count overflow",
  (char *)"EOF",
  (char *)"scan is already opened",
  (char *)"file is already closed",
  (char *)"compOp is invalid",
  (char *)"scan is already closed",
  (char *)"insert same entry",
  (char *)"curSlotNum Error",
  (char *)"indexscan doesn't support NE_OP",
  (char *)"try to delete invalid entry"
};

void IX_PrintError(RC rc) {
  if(rc >= START_IX_WARN && rc <= IX_LASTWARN)
    cerr << "IX warning: " << IX_WarnMsg[rc - START_IX_WARN] << endl;
  else if (rc == 0)
    cerr << "IX_PrintError called with return code of 0" << endl;
  else 
    cerr << "IX error: " << rc << " is out of bounds" << endl;
}
