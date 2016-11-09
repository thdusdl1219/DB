//
// ix.h
//
//   Index Manager Component Interface
//

#ifndef IX_H
#define IX_H

// Please do not include any other files than the ones below in this file.

#include "redbase.h"  // Please don't change these lines
#include "rm_rid.h"  // Please don't change these lines
#include "pf.h"

// IX_FileHdr
// file을 위한 헤더
//
struct IX_FileHdr {
  PageNum firstFree;
  AttrType attrType;
  int attrLength;
};


template<typename T> class IX_BpTree;

//
// IX_IndexHandle: IX Index File interface
//
class IX_IndexHandle {
    friend class IX_Manager;
    friend class IX_IndexScan;
public:
    IX_IndexHandle();
    ~IX_IndexHandle();

    // Insert a new index entry
    RC InsertEntry(void *pData, const RID &rid);

    // Delete a new index entry
    RC DeleteEntry(void *pData, const RID &rid);

    // Force index files to disk
    RC ForcePages();
private:
    PF_FileHandle pfFileHandle;
    IX_FileHdr fileHdr;
    int bHdrChanged;
    IX_BpTree<int>* intt;
    IX_BpTree<float>* floatt;
    IX_BpTree<char>* chart;
};

//
// IX_IndexScan: condition-based scan of index entries
//
class IX_IndexScan {
public:
    IX_IndexScan();
    ~IX_IndexScan();

    // Open index scan
    RC OpenScan(const IX_IndexHandle &indexHandle,
                CompOp compOp,
                void *value,
                ClientHint  pinHint = NO_HINT);

    // Get the next matching entry return IX_EOF if no more matching
    // entries.
    RC GetNextEntry(RID &rid);

    // Close index scan
    RC CloseScan();
private:
    RC Find(RID& rid);
    RC Operation(RID& rid, char* pData);
    int Compare(void* key1, void* key2);
    int bScanOpen;
    IX_IndexHandle* pIndexHandle;
    CompOp compOp;
    void* value;
    IX_BpTree<int>* intt;
    IX_BpTree<float>* floatt;
    IX_BpTree<char>* chart;
    PageNum curPageNum;
    SlotNum curSlotNum;
    int attrLength;
};

//
// IX_Manager: provides IX index file management
//
class IX_Manager {
public:
    IX_Manager(PF_Manager &pfm);
    ~IX_Manager();

    // Create a new Index
    RC CreateIndex(const char *fileName, int indexNo,
                   AttrType attrType, int attrLength);

    // Destroy and Index
    RC DestroyIndex(const char *fileName, int indexNo);

    // Open an Index
    RC OpenIndex(const char *fileName, int indexNo,
                 IX_IndexHandle &indexHandle);

    // Close an Index
    RC CloseIndex(IX_IndexHandle &indexHandle);
private:
    RC CheckType(AttrType attrType, int attrLength);
    char* MakeName(const char* fileName, int indexNo);
    PF_Manager* pPfm;
};

//
// Print-error function
//
void IX_PrintError(RC rc);


#define IX_TYPEERROR 1111
#define IX_NULLPOINTER 1112
#define IX_OVERFLOW 1113
#define IX_EOF 1114
#define IX_SCANOPEN 1115
#define IX_CLOSEDFILE 1116
#define IX_INVALIDCOMPOP 1117
#define IX_CLOSEDSCAN 1118
#define IX_SAMEINDEX 1119
#define IX_SOMETHINGWRONG 1120
#define IX_NEOP 1121

#endif
