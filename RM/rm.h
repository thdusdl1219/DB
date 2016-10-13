//
// rm.h
//
//   Record Manager component interface
//
// This file does not include the interface for the RID class.  This is
// found in rm_rid.h
//

#ifndef RM_H
#define RM_H

// Please DO NOT include any files other than redbase.h and pf.h in this
// file.  When you submit your code, the test program will be compiled
// with your rm.h and your redbase.h, along with the standard pf.h that
// was given to you.  Your rm.h, your redbase.h, and the standard pf.h
// should therefore be self-contained (i.e., should not depend upon
// declarations in any other file).

// Do not change the following includes
#include "redbase.h"
#include "rm_rid.h"
#include "pf.h"


// RM_FileHdr : header format for RM
struct RM_FileHdr {
  int recordSize;
  int numPage;
  int firstFree;
};

struct RM_PageHdr {
  int nextFree;
};



//
// RM_Record: RM Record interface
//
class RM_Record {
  friend class RM_FileHandle;
  friend class RM_FileScan;
public:
    RM_Record ();
    ~RM_Record();

    // Return the data corresponding to the record.  Sets *pData to the
    // record contents.
    RC GetData(char *&pData) const;

    // Return the RID associated with the record
    RC GetRid (RID &rid) const;
    // for Debug
#ifdef RM_LOG
    RC SetRid (RID &rid);
    RC SetData (char *pData);
#endif

private:
    char* pData;
    RID* rid;
};

struct ScanRecord {
  RM_Record cur;
  struct ScanRecord *next;
};

//
// RM_FileHandle: RM File interface
//
class RM_FileHandle {
  friend class RM_Manager;
  friend class RM_FileScan;
public:
    RM_FileHandle ();
    ~RM_FileHandle();

    // Given a RID, return the record
    RC GetRec     (const RID &rid, RM_Record &rec) const;

    RC InsertRec  (const char *pData, RID &rid);       // Insert a new record

    RC DeleteRec  (const RID &rid);                    // Delete a record
    RC UpdateRec  (const RM_Record &rec);              // Update a record

    // Forces a page (along with any contents stored in this class)
    // from the buffer pool to disk.  Default value forces all pages.
    RC ForcePages (PageNum pageNum = ALL_PAGES);
private:
    int GetBit(int* map, int index) const;
    void SetBit(int* map, int index);
    void UnsetBit(int* map, int index);
    void XorBit(int* map, int index);
    int FindFree(int* map) const;
    bool isEmpty(int* map) const;
    bool isFull(int * map) const;
    int* GetMap(char* data) const;
    RC GetData(PageNum pn, char *&buf) const;

    PF_FileHandle* pfh;
    RM_FileHdr hdr;
    int recordNum;
    int bitmapSize;
    int totalRecordNum;
    int fd;
};

//
// RM_FileScan: condition-based scan of records in the file
//
class RM_FileScan {
public:
    RM_FileScan  ();
    ~RM_FileScan ();

    RC OpenScan  (const RM_FileHandle &fileHandle,
                  AttrType   attrType,
                  int        attrLength,
                  int        attrOffset,
                  CompOp     compOp,
                  void       *value,
                  ClientHint pinHint = NO_HINT); // Initialize a file scan
    RC GetNextRec(RM_Record &rec);               // Get next matching record
    RC CloseScan ();                             // Close the scan
private:
    RC CheckType(AttrType attrType, int attrLength);
    bool Operating(AttrType attrType, CompOp compOp, void* value1, void* value2, int n);
    bool Equal(AttrType attrType, void* value1, void* value2, int n);
    bool Less(AttrType attrType, void* value1, void* value2, int n);
    bool Greater(AttrType attrType, void* value1, void* value2, int n);
    
    struct ScanRecord* first;
    int recordSize;
};

//
// RM_Manager: provides RM file management
//
class RM_Manager {
public:
    RM_Manager    (PF_Manager &pfm);
    ~RM_Manager   ();

    RC CreateFile (const char *fileName, int recordSize);
    RC DestroyFile(const char *fileName);
    RC OpenFile   (const char *fileName, RM_FileHandle &fileHandle);

    RC CloseFile  (RM_FileHandle &fileHandle);
private:
    PF_Manager* pfm;
    int recordSize;
};


//
// Print-error function
//
void RM_PrintError(RC rc);

#endif
