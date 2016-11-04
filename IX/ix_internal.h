
#ifndef IX_INTERNAL_H
#define IX_INTERNAL_H

#include <cstdio>
#include <cstring>
#include "ix.h"

const int IX_HEADER_PAGE_NUM = 0;

#define IX_PAGE_LIST_END  -1
#define IX_PAGE_FULL      -2

struct IX_PageHdr {
  PageNum nextFree;
};

#endif
