#include "rm.h"
#include "rm_internal.h"
#include <iostream>
using namespace std;

RM_Manager::RM_Manager(PF_Manager &pfm) {
  this->pfm = pfm;
}

RM_Manager::~RM_Manager() {
}

RC RM_Manager::CreateFile(const char *fileName, int recordSize) {
  if(recordSize > RM_PAGE_SIZE)
  {
    return RM_RECORDBIG;
  }
  int fd;
  int rc;
  if((rc = pfm.CreateFile(fileName)))
    return rc;
  if((fd = open(fileName,
#ifdef PC
        O_BINARY |
#endif
        O_WRONLY)) < 0)
    return (PF_UNIX);

  char hdrPage[PF_PAGE_SIZE];
  memset(hdrPage, 0, PF_PAGE_SIZE);
  RM_FileHdr *hdr = (RM_FileHdr*)hdrPage;
  hdr->recordSize = recordSize;
  hdr->firstFree = RM_PAGE_LIST_END; 
  hdr->numPage = 0;
  int numBytes, offset = sizeof(PF_FileHdr);
  lseek(fd, offset, L_SET); 
  if((numBytes = write(fd, hdrPage, PF_PAGE_SIZE)) != PF_PAGE_SIZE) {
    close(fd);
    unlink(fileName);
    if(numBytes < 0)
      return PF_UNIX;
    else
      return PF_HDRWRITE;
  }
  if(close(fd) < 0)
    return PF_UNIX;
  this->recordSize = recordSize;
  return (0);
}

RC RM_Manager::DestroyFile(const char* fileName) {
  int rc;
  if((rc = pfm.DestroyFile(fileName)))
    return rc;
  return (0);
}

RC RM_Manager::OpenFile(const char *fileName, RM_FileHandle &fileHandle) {
  PF_FileHandle *pfh = new PF_FileHandle();
  int rc, fd;
  if((rc = pfm.OpenFile(fileName, *pfh)))
    return (rc);
  if((fd = open(fileName, 
#ifdef PC
          O_BINARY |
#endif
          O_RDWR)) < 0)
    return (PF_UNIX);
  {
    lseek(fd, sizeof(PF_FileHdr), L_SET);
    int numBytes = read(fd, (char *)&fileHandle.hdr, sizeof(RM_FileHdr));
    if(numBytes != sizeof(RM_FileHdr)) {
      rc = (numBytes < 0) ? PF_UNIX : PF_HDRREAD;
      goto err;
    }
  }
  fileHandle.pfh = pfh;
  fileHandle.bitmapSize = (((RM_PAGE_SIZE / fileHandle.hdr.recordSize) / 8 + 1) / sizeof(int) + 1);
  fileHandle.recordNum = (RM_PAGE_SIZE - fileHandle.bitmapSize * sizeof(int)) / fileHandle.hdr.recordSize;
  return (0);
err:
  return rc;
}

RC RM_Manager::CloseFile(RM_FileHandle &fileHandle) {
  int rc;
  cout << fileHandle.hdr.numPage << endl;
  /* for(int i = 0; i < fileHandle.hdr.numPage; i++) {
    if((rc = fileHandle.pfh->UnpinPage(i)))
      return rc;
  } */
  cout << "hi1" << endl;
  if((rc = pfm.CloseFile(*fileHandle.pfh)))
    return (rc);
  cout << "hi2" << endl;
  return (0);
}

