#include "BlockBuffer.h"

#include <cstdlib>
#include <cstring>
#include <cstdio>


BlockBuffer::BlockBuffer(int blockNum) {
  this->blockNum=blockNum;
}

RecBuffer::RecBuffer(int blockNum) : BlockBuffer::BlockBuffer(blockNum) {}


int BlockBuffer::getHeader(struct HeadInfo *head) {
  unsigned char *bufferPtr;
  int ret = loadBlockAndGetBufferPtr(&bufferPtr);
  if (ret != SUCCESS) {
    return ret;   
  }
  
  memcpy(&head->numSlots, bufferPtr + 24, 4);
  memcpy(&head->numEntries, bufferPtr + 16, 4);
  memcpy(&head->numAttrs, bufferPtr + 20, 4);
  memcpy(&head->rblock, bufferPtr + 12, 4);
  memcpy(&head->lblock,bufferPtr + 8, 4);

  return SUCCESS;
}

int RecBuffer::setRecord(union Attribute *rec,int slotNum){
  unsigned char *bufferPtr;
  int blockaddr=loadBlockAndGetBufferPtr(&bufferPtr);
  if(blockaddr!=SUCCESS) return blockaddr;
  HeadInfo head;
  this->getHeader(&head);
  int attrCount=head.numAttrs;
  int slotCount=head.numSlots;
  if(slotNum<0 || slotNum>slotCount){
    return E_OUTOFBOUND;
  }
  int recordSize = attrCount * ATTR_SIZE;
  int offset=32+slotCount+(recordSize*slotNum);
  memcpy(bufferPtr+offset,rec,recordSize);
  StaticBuffer::setDirtyBit(this->blockNum);

return SUCCESS;
}

// load the record at slotNum into the argument pointer
int RecBuffer::getRecord(union Attribute *rec, int slotNum) {
  struct HeadInfo head;
  this->getHeader(&head);

  int attrCount = head.numAttrs;
  int slotCount = head.numSlots;
  unsigned char *bufferPtr;
  int ret = loadBlockAndGetBufferPtr(&bufferPtr);
  if (ret != SUCCESS) {
    return ret;
  }

  int recordSize = attrCount * ATTR_SIZE;
  int offset=32+slotCount+(recordSize*slotNum);
  unsigned char *slotPointer = bufferPtr+offset;

  memcpy(rec, slotPointer, recordSize);

  return SUCCESS;
}

int BlockBuffer::loadBlockAndGetBufferPtr(unsigned char **buffPtr) {

  int bufferNum = StaticBuffer::getBufferNum(this->blockNum);
  if(bufferNum!=E_BLOCKNOTINBUFFER){
    for (int i = 0; i < BUFFER_CAPACITY; i++){
            if (!StaticBuffer::metainfo[i].free){
                StaticBuffer::metainfo[i].timeStamp += 1;
            }  
        }
    StaticBuffer::metainfo[bufferNum].timeStamp=0;
  }

  if (bufferNum == E_BLOCKNOTINBUFFER) {
    bufferNum = StaticBuffer::getFreeBuffer(this->blockNum);

    if (bufferNum == E_OUTOFBOUND) {
      return E_OUTOFBOUND;
    }

    Disk::readBlock(StaticBuffer::blocks[bufferNum], this->blockNum);
  }

  *buffPtr = StaticBuffer::blocks[bufferNum];

  return SUCCESS;
}

int RecBuffer::getSlotMap(unsigned char *slotMap) {
  unsigned char *bufferPtr;

  int ret = loadBlockAndGetBufferPtr(&bufferPtr);
  if (ret != SUCCESS) {
    return ret;
  }

  struct HeadInfo head;
  BlockBuffer::getHeader(&head);

  int slotCount = head.numSlots;
  unsigned char *slotMapInBuffer = bufferPtr + HEADER_SIZE;
  memcpy(slotMap,slotMapInBuffer,slotCount);

  return SUCCESS;
}


int compareAttrs(union Attribute attr1, union Attribute attr2, int attrType) {

  if (attrType == NUMBER){
        StaticBuffer::count ++;
    }
    double diff;
    if(attrType==STRING){
      diff=strcmp(attr1.sVal,attr2.sVal);
    }
    else{
      diff=attr1.nVal-attr2.nVal;
    }
    
    if (diff > 0) return 1;
    if (diff < 0) return -1;
    if (diff = 0) return 0;
    return 0;
}

//stage-7 
int BlockBuffer::setHeader(struct HeadInfo *head){

    unsigned char *bufferPtr;
    // get the starting address of the buffer containing the block using
    // loadBlockAndGetBufferPtr(&bufferPtr).
    int x=loadBlockAndGetBufferPtr(&bufferPtr);
    if(x!=SUCCESS)
      return x;

    // if loadBlockAndGetBufferPtr(&bufferPtr) != SUCCESS
        // return the value returned by the call.

    // cast bufferPtr to type HeadInfo*
    struct HeadInfo *bufferHeader = (struct HeadInfo *)bufferPtr;
    bufferHeader->blockType=head->blockType;
    bufferHeader->lblock=head->lblock;
    bufferHeader->rblock=head->rblock;
    bufferHeader->pblock=head->pblock;
    bufferHeader->numAttrs=head->numAttrs;
    bufferHeader->numEntries=head->numEntries;
    bufferHeader->numSlots=head->numSlots;

    // copy the fields of the HeadInfo pointed to by head (except reserved) to
    // the header of the block (pointed to by bufferHeader)
    //(hint: bufferHeader->numSlots = head->numSlots )
    x=StaticBuffer::setDirtyBit(this->blockNum);
    if(x!=SUCCESS)return x;

    // update dirty bit by calling StaticBuffer::setDirtyBit()
    // if setDirtyBit() failed, return the error code

    return SUCCESS;
}

int BlockBuffer::setBlockType(int blockType){

    unsigned char *bufferPtr;
    int ret=loadBlockAndGetBufferPtr(&bufferPtr);
    if(ret!=SUCCESS)return ret;
  
    /* get the starting address of the buffer containing the block
       using loadBlockAndGetBufferPtr(&bufferPtr). */

    // if loadBlockAndGetBufferPtr(&bufferPtr) != SUCCESS
        // return the value returned by the call.

    // store the input block type in the first 4 bytes of the buffer.
    // (hint: cast bufferPtr to int32_t* and then assign it)
    // *((int32_t *)bufferPtr) = blockType;
    *((int32_t *)bufferPtr)=blockType;

    // update the StaticBuffer::blockAllocMap entry corresponding to the
    // object's block number to `blockType`.
  StaticBuffer::blockAllocMap[this->blockNum]=blockType;
  ret=StaticBuffer::setDirtyBit(this->blockNum);
  if(ret!=SUCCESS)return ret;
    // update dirty bit by calling StaticBuffer::setDirtyBit()
    // if setDirtyBit() failed
        // return the returned value from the call

    return SUCCESS;
}

int BlockBuffer::getFreeBlock(int blockType){

    // iterate through the StaticBuffer::blockAllocMap and find the block number
    // of a free block in the disk.
    int bno=-1;
    for(int i=0;i<DISK_BLOCKS;i++){
      if(StaticBuffer::blockAllocMap[i]==UNUSED_BLK){
        bno=i;
        break;
      }
    }
    if(bno==-1)return E_DISKFULL;
    // if no block is free, return E_DISKFULL.
    this->blockNum=bno;
    // set the object's blockNum to the block number of the free block.
    int bufferno=StaticBuffer::getFreeBuffer(bno);
    // find a free buffer using StaticBuffer::getFreeBuffer() .
    struct HeadInfo head;
    head.pblock=-1;
    head.lblock=-1;
    head.rblock=-1;
    head.numAttrs=0;
    head.numEntries=0;
    head.numSlots=0;
    BlockBuffer::setHeader(&head);
    BlockBuffer::setBlockType(blockType);
    // initialize the header of the block passing a struct HeadInfo with values
    // pblock: -1, lblock: -1, rblock: -1, numEntries: 0, numAttrs: 0, numSlots: 0
    // to the setHeader() function.

    // update the block type of the block to the input block type using setBlockType().

    // return block number of the free block.
    return bno;
}

BlockBuffer::BlockBuffer(char blockType){
    // allocate a block on the disk and a buffer in memory to hold the new block of
    // given type using getFreeBlock function and get the return error codes if any.
    int blockt;
    if(blockType=='R')
       blockt = REC;
    else if(blockType=='I')
       blockt = IND_INTERNAL;
    else
       blockt = IND_LEAF;
    
    int bl=getFreeBlock(blockt);
    this->blockNum=bl;
    // set the blockNum field of the object to that of the allocated block
    // number if the method returned a valid block number,
    // otherwise set the error code returned as the block number.

    // (The caller must check if the constructor allocatted block successfully
    // by checking the value of block number field.)
}

RecBuffer::RecBuffer() : BlockBuffer('R'){}
// call parent non-default constructor with 'R' denoting record block.


int RecBuffer::setSlotMap(unsigned char *slotMap) {
    unsigned char *bufferPtr;
    /* get the starting address of the buffer containing the block using
       loadBlockAndGetBufferPtr(&bufferPtr). */
       int ret=loadBlockAndGetBufferPtr(&bufferPtr);
       if(ret!=SUCCESS)return ret;

    // if loadBlockAndGetBufferPtr(&bufferPtr) != SUCCESS
        // return the value returned by the call.
    HeadInfo head;
    BlockBuffer::getHeader(&head);
    // get the header of the block using the getHeader() function

    int numSlots = head.numSlots/* the number of slots in the block */;

    // the slotmap starts at bufferPtr + HEADER_SIZE. Copy the contents of the
    // argument `slotMap` to the buffer replacing the existing slotmap.
    // Note that size of slotmap is `numSlots`
    memcpy(bufferPtr + HEADER_SIZE, slotMap, numSlots);

    ret=StaticBuffer::setDirtyBit(this->blockNum);
    if(ret!=SUCCESS)return ret;
    // update dirty bit using StaticBuffer::setDirtyBit
    // if setDirtyBit failed, return the value returned by the call

    return SUCCESS;
}

int BlockBuffer::getBlockNum(){
    return this->blockNum;
    //return corresponding block number.
}

//stage-8 

void BlockBuffer::releaseBlock(){
    if(this->blockNum==INVALID_BLOCKNUM || StaticBuffer::blockAllocMap[this->blockNum]==UNUSED_BLK)
{
  return;
}
  int buffNum=StaticBuffer::getBufferNum(this->blockNum);
  if(buffNum!=E_BLOCKNOTINBUFFER){
    StaticBuffer::metainfo[buffNum].free=true;
    StaticBuffer::blockAllocMap[this->blockNum]=UNUSED_BLK;
    this->blockNum=INVALID_BLOCKNUM;
    return;
  }

}
//stage 10


// call the corresponding parent constructor
IndBuffer::IndBuffer(char blockType) : BlockBuffer(blockType){}

// call the corresponding parent constructor
IndBuffer::IndBuffer(int blockNum) : BlockBuffer(blockNum){}

IndInternal::IndInternal() : IndBuffer('I'){}
// call the corresponding parent constructor
// 'I' used to denote IndInternal.

IndInternal::IndInternal(int blockNum) : IndBuffer(blockNum){}
// call the corresponding parent constructor

IndLeaf::IndLeaf() : IndBuffer('L'){} // this is the way to call parent non-default constructor.
                      // 'L' used to denote IndLeaf.

//this is the way to call parent non-default constructor.
IndLeaf::IndLeaf(int blockNum) : IndBuffer(blockNum){}

int IndInternal::getEntry(void *ptr, int indexNum) {
    // if the indexNum is not in the valid range of [0, MAX_KEYS_INTERNAL-1]
    //     return E_OUTOFBOUND.
    if(indexNum<0 || indexNum>MAX_KEYS_INTERNAL-1)
      return E_OUTOFBOUND;

    unsigned char *bufferPtr;
    /* get the starting address of the buffer containing the block
       using loadBlockAndGetBufferPtr(&bufferPtr). */
    int ret=loadBlockAndGetBufferPtr(&bufferPtr);
    if(ret!= SUCCESS)
      return ret;

    // typecast the void pointer to an internal entry pointer
    struct InternalEntry *internalEntry = (struct InternalEntry *)ptr;

    /*
    - copy the entries from the indexNum`th entry to *internalEntry
    - make sure that each field is copied individually as in the following code
    - the lChild and rChild fields of InternalEntry are of type int32_t
    - int32_t is a type of int that is guaranteed to be 4 bytes across every
      C++ implementation. sizeof(int32_t) = 4
    */

    /* the indexNum'th entry will begin at an offset of
       HEADER_SIZE + (indexNum * (sizeof(int) + ATTR_SIZE) )         [why?]
       from bufferPtr */
    unsigned char *entryPtr = bufferPtr + HEADER_SIZE + (indexNum * 20);

    memcpy(&(internalEntry->lChild), entryPtr, sizeof(int32_t));
    memcpy(&(internalEntry->attrVal), entryPtr + 4, sizeof(Attribute));
    memcpy(&(internalEntry->rChild), entryPtr + 20, 4);

    return SUCCESS;
}


int IndLeaf::getEntry(void *ptr, int indexNum) {

    // if the indexNum is not in the valid range of [0, MAX_KEYS_LEAF-1]
    //     return E_OUTOFBOUND.
    if(indexNum<0 || indexNum>MAX_KEYS_INTERNAL-1)
      return E_OUTOFBOUND;

    unsigned char *bufferPtr;;
    /* get the starting address of the buffer containing the block
       using loadBlockAndGetBufferPtr(&bufferPtr). */

    // if loadBlockAndGetBufferPtr(&bufferPtr) != SUCCESS
    //     return the value returned by the call.
    int ret=loadBlockAndGetBufferPtr(&bufferPtr);
    if(ret!= SUCCESS)
      return ret;

    // copy the indexNum'th Index entry in buffer to memory ptr using memcpy

    /* the indexNum'th entry will begin at an offset of
       HEADER_SIZE + (indexNum * LEAF_ENTRY_SIZE)  from bufferPtr */
    unsigned char *entryPtr = bufferPtr + HEADER_SIZE + (indexNum * LEAF_ENTRY_SIZE);
    memcpy((struct Index *)ptr, entryPtr, LEAF_ENTRY_SIZE);

    return SUCCESS;
}
int IndInternal::setEntry(void *ptr, int indexNum) {
  return 0;
}

int IndLeaf::setEntry(void *ptr, int indexNum) {
  return 0;
}