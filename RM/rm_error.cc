#include "rm.h"
#include "pf_internal.h"
#include "rm_internal.h"

static char *RM_WarnMsg[] = {
  (char *)"file scan end",
  (char *)"recordSize is too big",
  (char *)"file doesn't open",
  (char *)"file doesn't have data",
  (char *)"page number in rid is invalid",
  (char *)"slot number in rid is invalid",
  (char *)"allocating page is needed",
  (char *)"record doesn't exist",
  (char *)"openscan type error",
  (char *)"openscan doesn't open",

};

static char *RM_ErrMsg[] = {
};

void RM_PrintError(RC rc) {
  if(abs(rc) < END_PF_WARN) {
    PF_PrintError(rc);
    return;
  }
  if( rc >= START_RM_WARN && rc <= RM_LASTWARN)
    cerr << "RM warning: " << RM_WarnMsg[rc - START_RM_WARN] << "\n";
  else if(-rc >= -START_RM_ERR && -rc < -RM_LASTERROR)
    cerr << "RM error: " << RM_ErrMsg[-rc + START_RM_ERR] << "\n";
  else 
    cerr << "RM error: " << rc << " is out of bounds\n";

}
