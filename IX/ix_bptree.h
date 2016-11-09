#ifndef IX_BPTREE_H
#define IX_BPTREE_H
#include "ix_internal.h"

#define IX_BPTREE_ENTRY_SIZE (attrLength + sizeof(RID))
#define IX_BPTREE_HEADER_SIZE sizeof(IX_BpTreeNodeHdr)

template <typename T>
struct IX_BpTreeEntry {
  IX_BpTreeEntry() {
    key = NULL;
  }
  ~IX_BpTreeEntry() {
    delete[] key;
  }
  bool operator==(const IX_BpTreeEntry<T> &e) const {
    return (this->rid == e.rid);
  }
  T* key;
  RID rid;
};

struct IX_BpTreeNodeHdr {
  int isLeaf;
  int fullCount;
  int count;
  PageNum seqPointer; // prev in LEAF
  PageNum next;
};

template<typename T>
class IX_BpTreeNode {
  template<typename U>
  friend class IX_BpTree;
public:
  IX_BpTreeNode() {
  }
  IX_BpTreeNode(int pageNum, PF_FileHandle& pfFileHandle, int isLeaf, int attrLength, AttrType attrType) {
    this->pageNum = pageNum;
    this->pfFileHandle = pfFileHandle;
    this->attrLength = attrLength;
    this->attrType = attrType;
    nodeHdr.isLeaf = isLeaf;
    nodeHdr.fullCount = (PF_PAGE_SIZE - IX_BPTREE_HEADER_SIZE) / IX_BPTREE_ENTRY_SIZE;
    nodeHdr.count = 0;
    nodeHdr.seqPointer = IX_NODE_END;
    nodeHdr.next = IX_NODE_END;
//    save();
  }
  ~IX_BpTreeNode() {
    save();
  }
  RC save() {
    RC rc;
    PF_PageHandle pageHandle;
    char *pData;
    
    if((rc = pfFileHandle.GetThisPage(pageNum, pageHandle)))
      return rc;
  
    if((rc = pageHandle.GetData(pData))) {
      pfFileHandle.UnpinPage(pageNum);
      return rc;
    }
    
    memcpy(pData, &nodeHdr, IX_BPTREE_HEADER_SIZE);
    
    if((rc = pfFileHandle.MarkDirty(pageNum))) {
      pfFileHandle.UnpinPage(pageNum);
      return rc;
    }

    if((rc = pfFileHandle.UnpinPage(pageNum))) {
      return rc;
    }

    return (0);
  }
  RC load() {
    RC rc;
    PF_PageHandle pageHandle;
    char *pData;

    if((rc = pfFileHandle.GetThisPage(pageNum, pageHandle)))
      return rc;

    if((rc = pageHandle.GetData(pData))) {
      pfFileHandle.UnpinPage(pageNum);
      return rc;
    }
    memcpy(&nodeHdr, pData, IX_BPTREE_HEADER_SIZE);

    if((rc = pfFileHandle.UnpinPage(pageNum)))
      return rc;
    
    return (0);
  }
  RC getData(int slot, IX_BpTreeEntry<T>& entry) {
    RC rc;
    PF_PageHandle pageHandle;
    char *pData;

    if((rc = pfFileHandle.GetThisPage(pageNum, pageHandle)))
      return rc;

    if((rc = pageHandle.GetData(pData))) {
      pfFileHandle.UnpinPage(pageNum);
      return rc;
    }

    int pn, sn;
    memcpy(&pn, pData + attrLength + slot * IX_BPTREE_ENTRY_SIZE + IX_BPTREE_HEADER_SIZE, sizeof(int));
    memcpy(&sn, pData + attrLength + sizeof(int) + slot * IX_BPTREE_ENTRY_SIZE + IX_BPTREE_HEADER_SIZE, sizeof(int));

    entry.rid = RID(pn, sn);
    entry.key = (T*)(new char[attrLength]);
    memcpy(entry.key, pData + slot * IX_BPTREE_ENTRY_SIZE + IX_BPTREE_HEADER_SIZE, attrLength);
    if((rc = pfFileHandle.UnpinPage(pageNum)))
      return rc;

    return (0);

  }
  RC setData(IX_BpTreeEntry<T>& entry) {
    RC rc;
    PF_PageHandle pageHandle;
    char* pData;
    int i = 0;

    if(nodeHdr.fullCount <= nodeHdr.count)
      return IX_OVERFLOW;

    for(i = 0; i < nodeHdr.count; i++) {
      IX_BpTreeEntry<T> e;
      getData(i, e);
      if(findGreat(e.key, entry.key)) {
        break;
      }
    }

    if((rc = pfFileHandle.GetThisPage(pageNum, pageHandle)))
      return rc;

    if((rc = pageHandle.GetData(pData))) {
      pfFileHandle.UnpinPage(pageNum);
      return rc;
    }

    if(nodeHdr.count > 0) {
      memcpy(pData + (i + 1) * IX_BPTREE_ENTRY_SIZE + IX_BPTREE_HEADER_SIZE, pData + i * IX_BPTREE_ENTRY_SIZE + IX_BPTREE_HEADER_SIZE, IX_BPTREE_ENTRY_SIZE * (nodeHdr.count - i));
      memcpy(pData + i * IX_BPTREE_ENTRY_SIZE + IX_BPTREE_HEADER_SIZE, entry.key, attrLength);
      memcpy(pData + attrLength + i * IX_BPTREE_ENTRY_SIZE + IX_BPTREE_HEADER_SIZE, &entry.rid, sizeof(RID));

    }
    else {
      memcpy(pData + IX_BPTREE_HEADER_SIZE, entry.key, attrLength);
      memcpy(pData + attrLength + IX_BPTREE_HEADER_SIZE, &entry.rid, sizeof(RID));
    }
    nodeHdr.count++;

    memcpy(pData, &nodeHdr, IX_BPTREE_HEADER_SIZE);

    if((rc = pfFileHandle.MarkDirty(pageNum))) {
      pfFileHandle.UnpinPage(pageNum);
      return rc;
    }

    if((rc = pfFileHandle.UnpinPage(pageNum)))
      return rc;

    return (0);
  }
private:
  int findGreat(T* key1, T* key2) {
    float cmp = 0; 
    switch(attrType) {
      case INT:
        cmp = *((int *) key1) - *((int *) key2);
        break;
      case FLOAT:
        cmp = *((float *)key1) - *((float *) key2);
        break;
      case STRING:
        cmp = memcmp(key1, key2, attrLength);
        break;
    }
    if(cmp > 0)
      return 1;
    return 0;
  }

  int pageNum;
  IX_BpTreeNodeHdr nodeHdr;
  PF_FileHandle pfFileHandle;
  int attrLength;
  AttrType attrType;
};

template<typename T>
class IX_BpTree {
public:
  IX_BpTree(PF_FileHandle& pfFileHandle, AttrType attrType, int attrLength) {
    this->attrLength = attrLength;
    this->attrType = attrType;
    PageNum pageNum;
    PF_PageHandle pageHandle;
    pfFileHandle.AllocatePage(pageHandle);
    pageHandle.GetPageNum(pageNum);
    this->root = new IX_BpTreeNode<T>(pageNum, pfFileHandle, 1, attrLength, attrType);
    this->root->save();
    pfFileHandle.UnpinPage(pageNum);
    this->pfFileHandle = pfFileHandle;
  }
  RC Insert(IX_BpTreeNode<T>* nodepointer, IX_BpTreeEntry<T>* entry, IX_BpTreeEntry<T>** newchildentry) {
    RC rc;
    PF_PageHandle pageHandle;
    PageNum pageNum;

    if(nodepointer == NULL) {
      nodepointer = root;
    }
    if(!nodepointer->nodeHdr.isLeaf) {
      int i = 0;
      for(i = 0; i < nodepointer->nodeHdr.count; i++) {
      IX_BpTreeEntry<T> tmpentry;
        nodepointer->getData(i, tmpentry);
        int ret = findGreat(tmpentry.key, entry->key);
        if(ret == 1)
          break;
        if(ret == 2) {
          if(tmpentry == *entry)
            return IX_SAMEINDEX;
        }
          
      }
      i = i - 1;
      if(i == -1)
        pageNum = nodepointer->nodeHdr.seqPointer;
      else {
        IX_BpTreeEntry<T> tmpentry;
        nodepointer->getData(i, tmpentry);
        tmpentry.rid.GetPageNum(pageNum); 
      }
      IX_BpTreeNode<T>* newTree = new IX_BpTreeNode<T>(pageNum, pfFileHandle, 0, attrLength, attrType);
      if((rc = newTree->load()))
        return rc;
      Insert(newTree, entry, newchildentry);
      delete newTree;
      if(*newchildentry == NULL) return (0);
      else {
      // split case
        if(nodepointer->nodeHdr.fullCount > nodepointer->nodeHdr.count) { // has space;
          nodepointer->setData(**newchildentry);
          delete *newchildentry;
          *newchildentry = NULL;
          return (0);
        }
        else { // no space
          //split
          IX_BpTreeNode<T> *newnode = NULL;
          if((rc = Split(nodepointer, &newnode, 0)))
            return rc;
          (*newchildentry)->rid.GetPageNum(pageNum);
          newnode->nodeHdr.seqPointer = pageNum;
          IX_BpTreeEntry<T>* e = new IX_BpTreeEntry<T> ();

          e->key = (T*)new char[attrLength];
          memcpy(e->key, entry->key, attrLength);
          e->rid = RID(newnode->pageNum, 0);

          *newchildentry = e;
          if(nodepointer == root) {
            if((rc = pfFileHandle.AllocatePage(pageHandle)))
              return rc;
            if((rc = pageHandle.GetPageNum(pageNum)))
              return rc;
            IX_BpTreeNode<T>* newroot = new IX_BpTreeNode<T>(pageNum, pfFileHandle, 0, attrLength, attrType);
            newroot->nodeHdr.seqPointer = root->pageNum;
            newroot->save();
            if((rc = pfFileHandle.UnpinPage(pageNum)))
              return rc;
            newroot->setData(**newchildentry);
            delete *newchildentry;
            *newchildentry = NULL;
            delete root;
            root = newroot;
          }
        }
      }
    }
    else { // leaf node
      if(nodepointer->nodeHdr.fullCount > nodepointer->nodeHdr.count) { // has space
        nodepointer->setData(*entry);
        delete *newchildentry;
        *newchildentry = NULL;
        return (0);
      }
      else { // split
        IX_BpTreeNode<T>* newnode = NULL;
        IX_BpTreeEntry<T>* e = new IX_BpTreeEntry<T>();
        if((rc = Split(nodepointer, &newnode, 1)))
          return rc;
        newnode->setData(*entry);
        newnode->getData(0, *e);
        e->rid = RID(newnode->pageNum, 0);
        *newchildentry = e;
        // set list pointer
      
        if(nodepointer->nodeHdr.next == IX_NODE_END) {
          nodepointer->nodeHdr.next = newnode->pageNum;
          newnode->nodeHdr.seqPointer = nodepointer->pageNum;
        }
        else {
          IX_BpTreeNode<T> nextNode = IX_BpTreeNode<T>(nodepointer->nodeHdr.next, pfFileHandle, 1, attrLength, attrType);
          nextNode.load();
          nextNode.nodeHdr.seqPointer = newnode->pageNum;
          nodepointer->nodeHdr.next = newnode->pageNum;
          newnode->nodeHdr.seqPointer = nodepointer->pageNum;
          newnode->nodeHdr.next = nextNode.pageNum;
        }
        delete newnode;
        if(nodepointer == root) {
          if((rc = pfFileHandle.AllocatePage(pageHandle)))
            return rc;
          pageHandle.GetPageNum(pageNum);
          IX_BpTreeNode<T>* newroot = new IX_BpTreeNode<T>(pageNum, pfFileHandle, 0, attrLength, attrType);
          newroot->nodeHdr.seqPointer = root->pageNum;
          newroot->save();
          if((rc = pfFileHandle.UnpinPage(pageNum)))
            return rc;
          newroot->setData(**newchildentry);
          delete *newchildentry;
          *newchildentry = NULL;
          delete root;
          root = newroot;
        }
      }
    }
    return (0);
  }
  RC Delete(IX_BpTreeNode<T>* parentpointer, IX_BpTreeNode<T>* nodepointer, IX_BpTreeEntry<T>* entry, IX_BpTreeEntry<T>* oldchildentry) {
    return (0);
  }
  RC Find(T* value) {
    IX_BpTreeNode<T>* curTree = root;
    while(!curTree->nodeHdr.isLeaf) {

    }

  

    return (0);
  }
private:
  RC Split(IX_BpTreeNode<T>* nodepointer, IX_BpTreeNode<T>** newnode, int isLeaf) {
    PageNum pageNum;
    RC rc;
    PF_PageHandle pageHandle;
    if((rc = pfFileHandle.AllocatePage(pageHandle)))
      return rc;
    pageHandle.GetPageNum(pageNum);
    *newnode = new IX_BpTreeNode<T>(pageNum, pfFileHandle, isLeaf, attrLength, attrType);
    if((rc = (*newnode)->save()))
      return rc;
    if((rc = pfFileHandle.UnpinPage(pageNum)))
      return rc;
    int d = nodepointer->nodeHdr.fullCount / 2;
    for(int i = d; i < nodepointer->nodeHdr.count; i++) {
      IX_BpTreeEntry<T> e; 
      nodepointer->getData(i, e);
      (*newnode)->setData(e);
    }
    nodepointer->nodeHdr.count = d;
    if((rc = nodepointer->save()))
      return rc;
    return (0);
  }
  int findGreat(T* key1, T* key2) {
    float cmp = 0; 
    switch(attrType) {
      case INT:
        cmp = *((int *) key1) - *((int *) key2);
        break;
      case FLOAT:
        cmp = *((float *)key1) - *((float *) key2);
        break;
      case STRING:
        cmp = memcmp(key1, key2, attrLength);
        break;
    }
    if(cmp > 0)
      return 1;
    if(cmp == 0)
      return 2;
    return 0;
  }
  PF_FileHandle pfFileHandle;
  IX_BpTreeNode<T>* root;
  int attrLength;
  AttrType attrType;
};

#endif
