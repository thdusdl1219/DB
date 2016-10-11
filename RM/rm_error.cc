#include "rm.h"
#include "pf_internal.h"
void RM_PrintError(RC rc) {
  if(rc < 400) {
    PF_PrintError(rc);
  }
}
