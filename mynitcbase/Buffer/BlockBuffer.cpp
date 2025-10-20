#include "BlockBuffer.h"
#include <iostream>
#include <cstdlib>
#include <cstring>

// the declarations for these functions can be found in "BlockBuffer.h"

// calls the parent class constructor
RecBuffer::RecBuffer(int blockNum) : BlockBuffer::BlockBuffer(blockNum) {}

// call parent non-default constructor with 'R' denoting record block.
RecBuffer::RecBuffer() : BlockBuffer('R'){}

//B+ Tree Index Constructors
// call the corresponding parent constructor
IndBuffer::IndBuffer(char blockType) : BlockBuffer(blockType){}

// call the corresponding parent constructor
IndBuffer::IndBuffer(int blockNum) : BlockBuffer(blockNum){}

// call the corresponding parent constructor
// 'I' used to denote IndInternal.
IndInternal::IndInternal() : IndBuffer('I'){}

// call the corresponding parent constructor
IndInternal::IndInternal(int blockNum) : IndBuffer(blockNum){}

// 'L' used to denote IndLeaf.
IndLeaf::IndLeaf() : IndBuffer('L'){} // this is the way to call parent non-default constructor.

//this is the way to call parent non-default constructor.
IndLeaf::IndLeaf(int blockNum) : IndBuffer(blockNum){}

int IndLeaf::getEntry(void *ptr, int indexNum) {

  // if the indexNum is not in the valid range of [0, MAX_KEYS_LEAF-1]
  //     return E_OUTOFBOUND.
  if (indexNum < 0 || indexNum >= MAX_KEYS_INTERNAL) return E_OUTOFBOUND;

  unsigned char *bufferPtr;
  /* get the starting address of the buffer containing the block
     using loadBlockAndGetBufferPtr(&bufferPtr). */
  int ret = loadBlockAndGetBufferPtr(&bufferPtr);

  // if loadBlockAndGetBufferPtr(&bufferPtr) != SUCCESS
  //     return the value returned by the call.
  if (ret != SUCCESS) return ret;

  // copy the indexNum'th Index entry in buffer to memory ptr using memcpy
  /* the indexNum'th entry will begin at an offset of
     HEADER_SIZE + (indexNum * LEAF_ENTRY_SIZE)  from bufferPtr */
  unsigned char *entryPtr = bufferPtr + HEADER_SIZE + (indexNum * LEAF_ENTRY_SIZE);
  memcpy((struct Index *)ptr, entryPtr, LEAF_ENTRY_SIZE);

  // return SUCCESS
  return SUCCESS;
}

int IndLeaf::setEntry(void *ptr, int indexNum) {

  // if the indexNum is not in the valid range of [0, MAX_KEYS_LEAF-1]
  //     return E_OUTOFBOUND.
  if (indexNum < 0 || indexNum >= MAX_KEYS_INTERNAL) return E_OUTOFBOUND;

  unsigned char *bufferPtr;
  /* get the starting address of the buffer containing the block
     using loadBlockAndGetBufferPtr(&bufferPtr). */
  int ret = loadBlockAndGetBufferPtr(&bufferPtr);

  // if loadBlockAndGetBufferPtr(&bufferPtr) != SUCCESS
  //     return the value returned by the call.
  if (ret != SUCCESS) return ret;

  // copy the Index at ptr to indexNum'th entry in the buffer using memcpy

  /* the indexNum'th entry will begin at an offset of
     HEADER_SIZE + (indexNum * LEAF_ENTRY_SIZE)  from bufferPtr */
  unsigned char *entryPtr = bufferPtr + HEADER_SIZE + (indexNum * LEAF_ENTRY_SIZE);
  memcpy(entryPtr, (struct Index *)ptr, LEAF_ENTRY_SIZE);

  // update dirty bit using setDirtyBit()
  // if setDirtyBit failed, return the value returned by the call
  StaticBuffer::setDirtyBit(this->blockNum);

  //return SUCCESS
  return SUCCESS;
}

int IndInternal::getEntry(void *ptr, int indexNum) {
  // if the indexNum is not in the valid range of [0, MAX_KEYS_INTERNAL-1]
  //     return E_OUTOFBOUND.
  if (indexNum < 0 || indexNum >= MAX_KEYS_INTERNAL) return E_OUTOFBOUND;

  unsigned char *bufferPtr;
  /* get the starting address of the buffer containing the block
     using loadBlockAndGetBufferPtr(&bufferPtr). */
  // if loadBlockAndGetBufferPtr(&bufferPtr) != SUCCESS
  //     return the value returned by the call.
  int ret = loadBlockAndGetBufferPtr(&bufferPtr);
  if (ret != SUCCESS) return ret;

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

  // return SUCCESS.
  return SUCCESS;
}

int IndInternal::setEntry(void *ptr, int indexNum) {
  
  // if the indexNum is not in the valid range of [0, MAX_KEYS_LEAF-1]
  //     return E_OUTOFBOUND.
  if (indexNum < 0 || indexNum >= MAX_KEYS_INTERNAL) return E_OUTOFBOUND;

  unsigned char *bufferPtr;
  /* get the starting address of the buffer containing the block
     using loadBlockAndGetBufferPtr(&bufferPtr). */
  int ret = loadBlockAndGetBufferPtr(&bufferPtr);

  // if loadBlockAndGetBufferPtr(&bufferPtr) != SUCCESS
  //     return the value returned by the call.
  if (ret != SUCCESS) return ret;

  // typecast the void pointer to an internal entry pointer
  struct InternalEntry *internalEntry = (struct InternalEntry *)ptr;

  /*
  - copy the entries from *internalEntry to the indexNum`th entry
  - make sure that each field is copied individually as in the following code
  - the lChild and rChild fields of InternalEntry are of type int32_t
  - int32_t is a type of int that is guaranteed to be 4 bytes across every
    C++ implementation. sizeof(int32_t) = 4
  */

  /* the indexNum'th entry will begin at an offset of
     HEADER_SIZE + (indexNum * (sizeof(int) + ATTR_SIZE) )         [why?]
     from bufferPtr */

  unsigned char *entryPtr = bufferPtr + HEADER_SIZE + (indexNum * 20);

  memcpy(entryPtr, &(internalEntry->lChild), 4);
  memcpy(entryPtr + 4, &(internalEntry->attrVal), ATTR_SIZE);
  memcpy(entryPtr + 20, &(internalEntry->rChild), 4);


  // update dirty bit using setDirtyBit()
  // if setDirtyBit failed, return the value returned by the call
  StaticBuffer::setDirtyBit(this->blockNum);

  // return SUCCESS
  return SUCCESS;
}

int compareAttrs(union Attribute attr1, union Attribute attr2, int attrType) {

  int diff=0;
  if (attrType == STRING) diff = strcmp(attr1.sVal, attr2.sVal);
  else diff = (attr1.nVal > attr2.nVal) ? 1 : (attr1.nVal < attr2.nVal) ? -1 : 0;

  
  if (diff > 0) return 1;
  if (diff < 0) return -1;
  return 0;
  
}

BlockBuffer::BlockBuffer(int blockNum) {
// initialise this.blockNum with the argument
this->blockNum=blockNum;
}

//constructor 1
BlockBuffer::BlockBuffer(char blockType){
  // allocate a block on the disk and a buffer in memory to hold the new block of
  // given type using getFreeBlock function and get the return error codes if any.
  int blockTypeConverted = UNUSED_BLK;
  if(blockType =='R'){
    blockTypeConverted = REC;
  }
  if(blockType =='L'){
    blockTypeConverted = IND_LEAF;
  }
  if(blockType =='I'){
    blockTypeConverted = IND_INTERNAL;
  }
  // set the blockNum field of the object to that of the allocated block
  // number if the method returned a valid block number,
  // otherwise set the error code returned as the block number.
  int blockNum = getFreeBlock(blockTypeConverted);
  if(blockNum<0 || blockNum>=DISK_BLOCKS){
    printf("Error: Block not available\n");
    this->blockNum = blockNum;
    return;
  }
  
  this->blockNum = blockNum;


  // (The caller must check if the constructor allocatted block successfully
  // by checking the value of block number field.)
}

//STAGE 6
/* NOTE: This function will NOT check if the block has been initialised as a
 record or an index block. It will copy whatever content is there in that
 disk block to the buffer.
 Also ensure that all the methods accessing and updating the block's data
 should call the loadBlockAndGetBufferPtr() function before the access or
 update is done. This is because the block might not be present in the
 buffer due to LRU buffer replacement. So, it will need to be bought back
 to the buffer before any operations can be done.
*/

int BlockBuffer::getBlockNum(){
  return this->blockNum;
}

int BlockBuffer::loadBlockAndGetBufferPtr(unsigned char ** buffPtr) {
/* check whether the block is already present in the buffer
   using StaticBuffer.getBufferNum() */
int bufferNum = StaticBuffer::getBufferNum(this->blockNum);
if (bufferNum == E_OUTOFBOUND) return E_OUTOFBOUND;

// if present (!=E_BLOCKNOTINBUFFER),
    // set the timestamp of the corresponding buffer to 0 and increment the
    // timestamps of all other occupied buffers in BufferMetaInfo.
if(bufferNum != E_BLOCKNOTINBUFFER){
  StaticBuffer::metainfo[bufferNum].timeStamp = 0;
  for(int i=0;i<BUFFER_CAPACITY;i++){
    if(i!=bufferNum && StaticBuffer::metainfo[i].free==false){
      StaticBuffer::metainfo[i].timeStamp++;
    }
  }
}

// else
    // get a free buffer using StaticBuffer.getFreeBuffer()

    // if the call returns E_OUTOFBOUND, return E_OUTOFBOUND here as
    // the blockNum is invalid

    // Read the block into the free buffer using readBlock()
else{
  bufferNum = StaticBuffer::getFreeBuffer(this->blockNum);
  if(bufferNum==E_OUTOFBOUND || bufferNum==FAILURE) return bufferNum;
  Disk::readBlock(StaticBuffer::blocks[bufferNum],this->blockNum);
}

// store the pointer to this buffer (blocks[bufferNum]) in *buffPtr
// return SUCCESS;
*buffPtr = StaticBuffer::blocks[bufferNum];
//Disk::writeBlock(*buffPtr, this->blockNum);
return SUCCESS;
}

// load the block header into the argument pointer
int BlockBuffer::getHeader(struct HeadInfo *head) {
  //unsigned char buffer[BLOCK_SIZE];
  // read the block at this.blockNum into the buffer
  //Disk::readBlock(buffer,this->blockNum);
  unsigned char *buffer = nullptr;
	int ret = loadBlockAndGetBufferPtr(&buffer);
	if (ret != SUCCESS) return ret;

  // populate the numEntries, numAttrs and numSlots fields in *head
  memcpy(&head->numSlots, buffer + 24, 4);
  memcpy(&head->numEntries, buffer+16, 4);
  memcpy(&head->numAttrs, buffer+20, 4);
  memcpy(&head->rblock, buffer+12, 4);
  memcpy(&head->lblock, buffer+8, 4);
  memcpy(&head->pblock, buffer+4, 4);

  return SUCCESS;
}

int BlockBuffer::setHeader(struct HeadInfo *head){

  unsigned char *bufferPtr = nullptr;
  // get the starting address of the buffer containing the block using
  // loadBlockAndGetBufferPtr(&bufferPtr).
  int ret = loadBlockAndGetBufferPtr(&bufferPtr);

  // if loadBlockAndGetBufferPtr(&bufferPtr) != SUCCESS
      // return the value returned by the call.
  if(ret!=SUCCESS) return ret;

  // cast bufferPtr to type HeadInfo*
  struct HeadInfo *bufferHeader = (struct HeadInfo *)bufferPtr;

  // copy the fields of the HeadInfo pointed to by head (except reserved) to
  // the header of the block (pointed to by bufferHeader)
  //(hint: bufferHeader->numSlots = head->numSlots )
  bufferHeader->blockType = head->blockType;
  bufferHeader->numSlots = head->numSlots;
  bufferHeader->numEntries = head->numEntries;
  bufferHeader->numAttrs = head->numAttrs;
  bufferHeader->rblock = head->rblock;
  bufferHeader->lblock = head->lblock;
  bufferHeader->pblock = head->pblock;

  // update dirty bit by calling StaticBuffer::setDirtyBit()
  // if setDirtyBit() failed, return the error code
  ret = StaticBuffer::setDirtyBit(this->blockNum);

  if(ret != SUCCESS) return ret;
  // return SUCCESS;
  return SUCCESS;
}

// load the record at slotNum into the argument pointer
int RecBuffer::getRecord(union Attribute *rec, int slotNum) {
  HeadInfo head;

  // get the header using this.getHeader() function
  this->getHeader(&head);

  int attrCount = head.numAttrs;
  int slotCount = head.numSlots;

  // read the block at this.blockNum into a buffer
  unsigned char *buffer = nullptr;
	// Disk::readBlock(buffer, this->blockNum);
	int ret = loadBlockAndGetBufferPtr(&buffer);
	if (ret != SUCCESS)
		return ret;

  /* record at slotNum will be at offset HEADER_SIZE + slotMapSize + (recordSize * slotNum)
     - each record will have size attrCount * ATTR_SIZE
     - slotMap will be of size slotCount
  */
  int recordSize = attrCount * ATTR_SIZE;
  int slotMapSize = head.numSlots;
  int recordOffset = HEADER_SIZE + slotMapSize + (recordSize * slotNum);
  unsigned char *slotPointer = buffer + recordOffset;

  // load the record into the rec data structure
  memcpy(rec, slotPointer, recordSize);

  return SUCCESS;
}

int RecBuffer::setRecord(union Attribute *rec, int slotNum) {
  unsigned char *bufferPtr = nullptr;
  /* get the starting address of the buffer containing the block
     using loadBlockAndGetBufferPtr(&bufferPtr). */
  int startAdd = loadBlockAndGetBufferPtr(&bufferPtr);

  // if loadBlockAndGetBufferPtr(&bufferPtr) != SUCCESS
      // return the value returned by the call.
  if(startAdd != SUCCESS) return startAdd;

  /* get the header of the block using the getHeader() function */
  HeadInfo head;
  getHeader(&head);

  // get number of attributes in the block.
  int noOfAttrs = head.numAttrs;

  // get the number of slots in the block.
  int noOfSlots = head.numSlots;

  // if input slotNum is not in the permitted range return E_OUTOFBOUND.
  if(slotNum >= noOfSlots) return E_OUTOFBOUND;

  /* offset bufferPtr to point to the beginning of the record at required
     slot. the block contains the header, the slotmap, followed by all
     the records. so, for example,
     record at slot x will be at bufferPtr + HEADER_SIZE + (x*recordSize)
     copy the record from `rec` to buffer using memcpy
     (hint: a record will be of size ATTR_SIZE * numAttrs)
  */
  int recordSize = noOfAttrs * ATTR_SIZE;
  int recordOffset = HEADER_SIZE + noOfSlots + (recordSize * slotNum);
  unsigned char *slotPointer = bufferPtr + recordOffset;
  memcpy(slotPointer, rec, recordSize);

  // update dirty bit using setDirtyBit()
  int flag = StaticBuffer::setDirtyBit(this->blockNum);

  /* (the above function call should not fail since the block is already
     in buffer and the blockNum is valid. If the call does fail, there
     exists some other issue in the code) */
  if(flag != SUCCESS){
    printf("error detected in code\n");
    return flag;
  }

  // return SUCCESS
  return SUCCESS;
}

//STAGE 4
/* used to get the slotmap from a record block
NOTE: this function expects the caller to allocate memory for `*slotMap`
int BlockBuffer::loadBlockAndGetBufferPtr(unsigned char** bufferPtr){
  static unsigned char buffer[BLOCK_SIZE];

  int ret = Disk::readBlock(buffer, this->blockNum);
  if (ret != SUCCESS) {
    return ret;
  }
  *bufferPtr = buffer;

  return SUCCESS;
} */


int RecBuffer::getSlotMap(unsigned char *slotMap) {
  unsigned char *bufferPtr = nullptr;

  // get the starting address of the buffer containing the block using loadBlockAndGetBufferPtr().
  int ret = loadBlockAndGetBufferPtr(&bufferPtr);
  if (ret != SUCCESS) {
    return ret;
  }

  struct HeadInfo head;
  // get the header of the block using getHeader() function
  this->getHeader(&head);

  int slotCount = head.numSlots;

  // get a pointer to the beginning of the slotmap in memory by offsetting HEADER_SIZE
  unsigned char *slotMapInBuffer = bufferPtr + HEADER_SIZE;

  // copy the values from `slotMapInBuffer` to `slotMap` (size is `slotCount`)
  memcpy(slotMap, slotMapInBuffer, slotCount);

  return SUCCESS;
}

int RecBuffer::setSlotMap(unsigned char *slotMap) {
  unsigned char *bufferPtr = nullptr;
  /* get the starting address of the buffer containing the block using
     loadBlockAndGetBufferPtr(&bufferPtr). */
  int ret = loadBlockAndGetBufferPtr(&bufferPtr);

  // if loadBlockAndGetBufferPtr(&bufferPtr) != SUCCESS
      // return the value returned by the call.
  if(ret != SUCCESS) return ret;

  // get the header of the block using the getHeader() function
  HeadInfo head;
  getHeader(&head);
  int numSlots = head.numSlots /* the number of slots in the block */;

  // the slotmap starts at bufferPtr + HEADER_SIZE. Copy the contents of the
  // argument `slotMap` to the buffer replacing the existing slotmap.
  // Note that size of slotmap is `numSlots`
  unsigned char *bufSlotMap = bufferPtr + HEADER_SIZE;
  memcpy(bufSlotMap, slotMap, numSlots);

  // update dirty bit using StaticBuffer::setDirtyBit
  // if setDirtyBit failed, return the value returned by the call
  ret = StaticBuffer::setDirtyBit(this->blockNum);
  if(ret != SUCCESS) return ret;
  return SUCCESS;
}

int BlockBuffer::setBlockType(int blockType){

  unsigned char *bufferPtr = nullptr;
  /* get the starting address of the buffer containing the block
     using loadBlockAndGetBufferPtr(&bufferPtr). */
  int ret = loadBlockAndGetBufferPtr(&bufferPtr);

  // if loadBlockAndGetBufferPtr(&bufferPtr) != SUCCESS
      // return the value returned by the call.
  if(ret != SUCCESS) return ret;

  // store the input block type in the first 4 bytes of the buffer.
  // (hint: cast bufferPtr to int32_t* and then assign it)
  // *((int32_t *)bufferPtr) = blockType;
  *((int32_t *)bufferPtr) = blockType;

  // update the StaticBuffer::blockAllocMap entry corresponding to the
  // object's block number to `blockType`.
  StaticBuffer::blockAllocMap[this->blockNum] = blockType;

  // update dirty bit by calling StaticBuffer::setDirtyBit()
  // if setDirtyBit() failed
      // return the returned value from the call
  ret = StaticBuffer::setDirtyBit(this->blockNum);
  if(ret != SUCCESS) return ret;

  // return SUCCESS
  return SUCCESS;
}

int BlockBuffer::getFreeBlock(int blockType){

  // iterate through the StaticBuffer::blockAllocMap and find the block number
  // of a free block in the disk.
  int freeBlockNum = -1;
  for(int i = 0; i < DISK_BLOCKS; i++){
    if(StaticBuffer::blockAllocMap[i] == UNUSED_BLK){
      freeBlockNum = i;
      break;
    }
  }

  // if no block is free, return E_DISKFULL.
  if(freeBlockNum == -1) return E_DISKFULL;

  // set the object's blockNum to the block number of the free block.
  this->blockNum = freeBlockNum;
  // find a free buffer using StaticBuffer::getFreeBuffer() .
  int freeBuf = StaticBuffer::getFreeBuffer(freeBlockNum);
  if(freeBuf<0 || freeBuf>=BUFFER_CAPACITY){
    printf("Error: Buffer is full\n");
    return freeBuf;
  }

  // initialize the header of the block passing a struct HeadInfo with values
  // pblock: -1, lblock: -1, rblock: -1, numEntries: 0, numAttrs: 0, numSlots: 0
  // to the setHeader() function.
  HeadInfo head;
  head.pblock=head.lblock=head.rblock=-1;
  head.numEntries=head.numAttrs=head.numSlots=0;
  setHeader(&head);

  // update the block type of the block to the input block type using setBlockType().
  setBlockType(blockType);
  // return block number of the free block.
  return freeBlockNum;
}

void BlockBuffer::releaseBlock()
{
  // if blockNum is INVALID_BLOCK (-1), or it is invalidated already, do nothing
	if (blockNum == INVALID_BLOCKNUM || StaticBuffer::blockAllocMap[blockNum] == UNUSED_BLK) return;

	//else
  /* get the buffer number of the buffer assigned to the block
  using StaticBuffer::getBufferNum().
  (this function return E_BLOCKNOTINBUFFER if the block is not
  currently loaded in the buffer)*/
	int bufferIndex = StaticBuffer::getBufferNum(blockNum);

	// if the block is present in the buffer, free the buffer
	// by setting the free flag of its StaticBuffer::tableMetaInfo entry
	// to true.
	if (bufferIndex >= 0 && bufferIndex < BUFFER_CAPACITY)
		StaticBuffer::metainfo[bufferIndex].free = true;

	// free the block in disk by setting the data type of the entry
	// corresponding to the block number in StaticBuffer::blockAllocMap
	// to UNUSED_BLK.
	StaticBuffer::blockAllocMap[blockNum] = UNUSED_BLK;

	// set the object's blockNum to INVALID_BLOCK (-1)
	this->blockNum = INVALID_BLOCKNUM;
}