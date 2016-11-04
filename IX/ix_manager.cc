
#include "ix_internal.h"


IX_Manager::IX_Manager(PF_Manager &pfm) {

  pPfm = &pfm;
}

IX_Manager::~IX_Manager() {
  pPfm = NULL;
}

RC IX_Manager::CreateIndex(const char *fileName, int indexNo, AttrType attrType, int attrLength) {
  RC rc;
  PF_FileHandle pfFileHandle;
  PF_PageHandle pageHandle;
  char* pData;
  IX_FileHdr *fileHdr;
  char* real_fileName;

  if((rc = CheckType(attrType, attrLength)))
    goto err_return;

  real_fileName = MakeName(fileName, indexNo);

  if((rc = pPfm->CreateFile(real_fileName))) {
    delete real_fileName;
    goto err_return;
  }

  if((rc = pPfm->OpenFile(real_fileName, pfFileHandle)))
    goto err_destroy;

  if((rc = pfFileHandle.AllocatePage(pageHandle)))
    goto err_close;

  if((rc = pageHandle.GetData(pData)))
    goto err_unpin;

// IX_FileHdr 와 관련된 부분
  fileHdr = (IX_FileHdr *)pData;
  fileHdr->firstFree = IX_PAGE_LIST_END;
  fileHdr->attrType = attrType;
  fileHdr->attrLength = attrLength;


  if((rc = pfFileHandle.MarkDirty(IX_HEADER_PAGE_NUM)))
    goto err_unpin;

  if((rc = pfFileHandle.UnpinPage(IX_HEADER_PAGE_NUM)))
    goto err_close;

  if((rc = pPfm->CloseFile(pfFileHandle)))
    goto err_destroy;

  delete real_fileName;
  return (0);

err_unpin:
  pfFileHandle.UnpinPage(IX_HEADER_PAGE_NUM);
err_close:
  pPfm->CloseFile(pfFileHandle);
err_destroy:
  pPfm->DestroyFile(real_fileName);
  delete real_fileName;
err_return:
  return (rc);
}

RC IX_Manager::DestroyIndex(const char* fileName, int indexNo) {
  RC rc;
  char* r_filename = MakeName(fileName, indexNo);

  if((rc = pPfm->DestroyFile(r_filename)))
    return rc;

  return (0);
}


RC IX_Manager::OpenIndex(const char* fileName, int indexNo, IX_IndexHandle &indexHandle) {
  RC rc;
  PF_PageHandle pageHandle;
  char* pData;
  char* r_filename = MakeName(fileName, indexNo);
  if((rc = pPfm->OpenFile(r_filename, indexHandle.pfFileHandle)))
    goto err_return;

  if((rc = indexHandle.pfFileHandle.GetFirstPage(pageHandle)))
    goto err_close;

  if((rc = pageHandle.GetData(pData)))
    goto err_unpin;

  memcpy(&indexHandle.fileHdr, pData, sizeof(indexHandle.fileHdr));

  if((rc = indexHandle.pfFileHandle.UnpinPage(IX_HEADER_PAGE_NUM)))
    goto err_close;

  indexHandle.bHdrChanged = FALSE;

  return (0);

err_unpin:
  indexHandle.pfFileHandle.UnpinPage(IX_HEADER_PAGE_NUM);
err_close:
  pPfm->CloseFile(indexHandle.pfFileHandle);
err_return:
  return (rc);

}

RC IX_Manager::CloseIndex(IX_IndexHandle &indexHandle) {
  RC rc;
  if (indexHandle.bHdrChanged) {
    PF_PageHandle pageHandle;
    char* pData;
    
    if((rc = indexHandle.pfFileHandle.GetFirstPage(pageHandle)))
      goto err_return;

    if((rc = pageHandle.GetData(pData)))
      goto err_unpin;

    memcpy(pData, &indexHandle.fileHdr, sizeof(indexHandle.fileHdr));

    if((rc = indexHandle.pfFileHandle.MarkDirty(IX_HEADER_PAGE_NUM)))
      goto err_unpin;

    if((rc = indexHandle.pfFileHandle.UnpinPage(IX_HEADER_PAGE_NUM)))
      goto err_return;

    indexHandle.bHdrChanged = FALSE;
  }


  if((rc = pPfm->CloseFile(indexHandle.pfFileHandle)))
    goto err_return;

  memset(&indexHandle.fileHdr, 0, sizeof(indexHandle.fileHdr));
  indexHandle.fileHdr.firstFree = IX_PAGE_LIST_END;

  return (0);

err_unpin:
  indexHandle.pfFileHandle.UnpinPage(IX_HEADER_PAGE_NUM);
err_return:
  return (rc);

}

char* IX_Manager::MakeName(const char* fileName, int indexNo) {
  char integer_char[sizeof(int*) + 1];
  char *real_fileName = new char[strlen(fileName) + sizeof(int*) + 2];
  
  strcpy(real_fileName, fileName);
  sprintf(integer_char, ".%d", indexNo);
  strcat(real_fileName, integer_char);

  return real_fileName;

}

RC IX_Manager::CheckType(AttrType attrType, int attrLength) {
  switch(attrType) {
    case INT :
      if(attrLength != 4)
        return IX_TYPEERROR;
      break;
    case FLOAT :
      if(attrLength != 4)
        return IX_TYPEERROR;
      break;
    case STRING :
      if(attrLength < 1 || attrLength > MAXSTRINGLEN)
        return IX_TYPEERROR;
      break;
  }
  return (0);
}
