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

#define RM_EOF 400
#define RM_RECORDBIG 401
#define RM_OPENFILE 402
#define RM_NULL 403
#define RM_RID 404
#define RM_OFFSET 405
#define RM_PAGEEMPTY 406
#define RM_RECNOTIN 407



