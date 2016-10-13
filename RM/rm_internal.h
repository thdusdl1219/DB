#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>

using namespace std;

#define RM_PAGE_LIST_END -1
#define RM_PAGE_FULL -2
#define RM_PAGE_SIZE (PF_PAGE_SIZE-sizeof(int))

#define M_SET 0

#define RM_EOF (START_RM_WARN + 0)
#define RM_RECORDSIZEBIG (START_RM_WARN + 1)
#define RM_FILENOTOPEN (START_RM_WARN + 2)
#define RM_NODATA (START_RM_WARN + 3)
#define RM_RIDPAGEINVALID (START_RM_WARN + 4)
#define RM_RIDSLOTINVALID (START_RM_WARN + 5)
#define RM_INDEXINVALID (START_RM_WARN + 6)
#define RM_RECNOTIN (START_RM_WARN + 7)


