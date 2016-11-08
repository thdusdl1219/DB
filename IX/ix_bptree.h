#ifndef IX_BPTREE_H
#define IX_BPTREE_H
#include "ix_internal.h"

#define IX_BPTREE_ENTRY_SIZE (attrLength + sizeof(RID))
#define IX_BPTREE_HEADER_SIZE sizeof(IX_BpTreeNodeHdr)

template <typename T>
struct IX_BpTreeEntry {
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
    memcpy(entry.key, pData + slot * IX_BPTREE_ENTRY_SIZE, attrLength);
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

    IX_BpTreeEntry<T> e;
    for(i = 0; i < nodeHdr.count; i++) {
      getData(i, e);
      if(findGreat(e.key, entry.key))
        break;
    }
    i = i + 1;

    if((rc = pfFileHandle.GetThisPage(pageNum, pageHandle)))
      return rc;

    if((rc = pageHandle.GetData(pData))) {
      pfFileHandle.UnpinPage(pageNum);
      return rc;
    }

    memcpy(pData + (i + 1) * IX_BPTREE_ENTRY_SIZE + IX_BPTREE_HEADER_SIZE, pData + i * IX_BPTREE_ENTRY_SIZE + IX_BPTREE_HEADER_SIZE, IX_BPTREE_ENTRY_SIZE * (nodeHdr.count - i));
    memcpy(pData + i * IX_BPTREE_ENTRY_SIZE + IX_BPTREE_HEADER_SIZE, &entry, IX_BPTREE_ENTRY_SIZE);

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
    if(cmp >= 0)
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
    this->pfFileHandle = pfFileHandle;
    this->attrLength = attrLength;
    this->attrType = attrType;
    PageNum pageNum;
    PF_PageHandle pageHandle;
    pfFileHandle.AllocatePage(pageHandle);
    pageHandle.GetPageNum(pageNum);
    this->root = new IX_BpTreeNode<T>(pageNum, pfFileHandle, 1, attrLength, attrType);
    this->root->save();
  }
  RC Insert(IX_BpTreeNode<T>* nodepointer, IX_BpTreeEntry<T>* entry, IX_BpTreeEntry<T>* newchildentry) {
    RC rc;
    PF_PageHandle pageHandle;
    PageNum pageNum;

    if(nodepointer == NULL) {
      nodepointer = root;
    }
    if(!nodepointer->nodeHdr.isLeaf) {
      IX_BpTreeEntry<T> tmpentry;
      for(int i = 0; i < nodepointer->nodeHdr.count; i++) {
        nodepointer->getData(i, tmpentry);
        if(findGreat(tmpentry.key, entry->key))
           break;
      }
      tmpentry.rid.GetPageNum(pageNum); 
      IX_BpTreeNode<T>* newTree = new IX_BpTreeNode<T>(pageNum, pfFileHandle, 0, attrLength, attrType);
      if((rc = newTree->load()))
        return rc;
      Insert(newTree, entry, newchildentry);
      delete newTree;
      if(newchildentry == NULL) return (0);
      else {
      // split case
        if(nodepointer->nodeHdr.fullCount > nodepointer->nodeHdr.count) { // has space;
          nodepointer->setData(*newchildentry);
          delete newchildentry;
          newchildentry = NULL;
          return (0);
        }
        else { // no space
          //split
          IX_BpTreeNode<T>* newnode = NULL;
          Split(nodepointer, newnode, 0);
          newchildentry->rid.GetPageNum(pageNum);
          newnode->nodeHdr.seqPointer = pageNum;
          newchildentry->key = entry->key;
          newchildentry->rid = RID(newnode->pageNum, 0);
          delete newnode;
          if(nodepointer == root) {
            pfFileHandle.AllocatePage(pageHandle);
            pageHandle.GetPageNum(pageNum);
            IX_BpTreeNode<T>* newroot = new IX_BpTreeNode<T>(pageNum, pfFileHandle, 0, attrLength, attrType);
            newroot->nodeHdr.seqPointer = root->pageNum;
            newroot->save();
            newroot->setData(*newchildentry);
            delete newchildentry;
            newchildentry = NULL;
            root = newroot;
          }
        }
      }
    }
    else { // leaf node
      if(nodepointer->nodeHdr.fullCount > nodepointer->nodeHdr.count) { // has space
        nodepointer->setData(*entry);
        delete newchildentry;
        newchildentry = NULL;
        return (0);
      }
      else { // split
        IX_BpTreeNode<T>* newnode = NULL;
        IX_BpTreeEntry<T>* e = new IX_BpTreeEntry<T>();
        Split(nodepointer, newnode, 1);
        newnode->setData(*entry);
        newnode->getData(0, *e);
        e->rid = RID(newnode->pageNum, 0);
        newchildentry = e;
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

        if(nodepointer == root) {
          pfFileHandle.AllocatePage(pageHandle);
          pageHandle.GetPageNum(pageNum);
          IX_BpTreeNode<T>* newroot = new IX_BpTreeNode<T>(pageNum, pfFileHandle, 0, attrLength, attrType);
          newroot->nodeHdr.seqPointer = root->pageNum;
          newroot->save();
          newroot->setData(*newchildentry);
          delete newchildentry;
          newchildentry = NULL;
          delete root;
          root = newroot;
        }
        delete newnode;
      }
    }
    return (0);
  }
  RC Delete(IX_BpTreeNode<T>* parentpointer, IX_BpTreeNode<T>* nodepointer, IX_BpTreeEntry<T>* entry, IX_BpTreeEntry<T>* oldchildentry) {
    return (0);

  }
private:
  RC Split(IX_BpTreeNode<T>* nodepointer, IX_BpTreeNode<T>* newnode, int isLeaf) {
    PageNum pageNum;
    RC rc;
    PF_PageHandle pageHandle;
    pfFileHandle.AllocatePage(pageHandle);
    pageHandle.GetPageNum(pageNum);
    newnode = new IX_BpTreeNode<T>(pageNum, pfFileHandle, isLeaf, attrLength, attrType);
    if((rc = newnode->save()))
      return rc;
    int d = nodepointer->nodeHdr.fullCount / 2;
    for(int i = d; i < nodepointer->nodeHdr.count; i++) {
      IX_BpTreeEntry<T> e; 
      nodepointer->getData(i, e);
      newnode->setData(e);
    }
    nodepointer->nodeHdr.count = d;
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
    if(cmp >= 0)
      return 1;
    return 0;
  }
  PF_FileHandle pfFileHandle;
  IX_BpTreeNode<T>* root;
  int attrLength;
  AttrType attrType;
};

#endif
