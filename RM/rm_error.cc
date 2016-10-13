#include "rm.h"
#include "pf_internal.h"

static char *RM_WarnMsg[] = {
  (char *)"file scan end"
};

void RM_PrintError(RC rc) {
  if(rc < 400) {
    PF_PrintError(rc);
  }
}
