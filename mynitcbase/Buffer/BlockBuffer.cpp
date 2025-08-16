#include "BlockBuffer.h"

#include <cstdlib>
#include <cstring>
#include <cstdio>

// the declarations for these functions can be found in "BlockBuffer.h"

BlockBuffer::BlockBuffer(int blockNum) {
  this->blockNum=blockNum;
}

// calls the parent class constructor
RecBuffer::RecBuffer(int blockNum) : BlockBuffer::BlockBuffer(blockNum) {}

// load the block header into the argument pointer
//stage 2

int BlockBuffer::getHeader(struct HeadInfo *head) {
  // unsigned char buffer[BLOCK_SIZE];

  // read the block at this.blockNum into the buffer
  // Disk::readBlock(buffer,this->blockNum);

  unsigned char *bufferPtr;
  int ret = loadBlockAndGetBufferPtr(&bufferPtr);
  if (ret != SUCCESS) {
    return ret;   // return any errors that might have occured in the process
  }
  

  // populate the numEntries, numAttrs and numSlots fields in *head
  memcpy(&head->numSlots, bufferPtr + 24, 4);
  memcpy(&head->numEntries, bufferPtr + 16, 4);
  memcpy(&head->numAttrs, bufferPtr + 20, 4);
  memcpy(&head->rblock, bufferPtr + 12, 4);
  memcpy(&head->lblock,bufferPtr + 8, 4);

  return SUCCESS;
}




int RecBuffer::setRecord(union Attribute *rec,int slotNum){
  // struct HeadInfo head;
  // this->getHeader(&head);
  // int attrCount=head.numAttrs;
  // int slotCount=head.numSlots;
  // unsigned char buffer[BLOCK_SIZE];
  // Disk::readBlock(buffer,this->blockNum);

  // int recordSize = attrCount * ATTR_SIZE;
  // int offset=32+slotCount+(recordSize*slotNum);

  // memcpy(buffer+offset,rec,recordSize);
  //   // printf("setRecord: writing to block %d at slot %d\n", this->blockNum, slotNum);

  // Disk::writeBlock(buffer,this->blockNum);
  // return SUCCESS;
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

  // get the header using this.getHeader() function
  this->getHeader(&head);

  int attrCount = head.numAttrs;
  int slotCount = head.numSlots;
  // read the block at this.blockNum into a buffer
  // printf("from getrecord");
  unsigned char *bufferPtr;
  int ret = loadBlockAndGetBufferPtr(&bufferPtr);
  if (ret != SUCCESS) {
    return ret;
  }

  

  /* record at slotNum will be at offset HEADER_SIZE + slotMapSize + (recordSize * slotNum)
     - each record will have size attrCount * ATTR_SIZE
     - slotMap will be of size slotCount
  */
  int recordSize = attrCount * ATTR_SIZE;
  int offset=32+slotCount+(recordSize*slotNum);
  unsigned char *slotPointer = bufferPtr+offset;

  // load the record into the rec data structure
  memcpy(rec, slotPointer, recordSize);

  return SUCCESS;
}

int BlockBuffer::loadBlockAndGetBufferPtr(unsigned char **buffPtr) {
  // check whether the block is already present in the buffer using StaticBuffer.getBufferNum()
  // printf("from loadblock");
  int bufferNum = StaticBuffer::getBufferNum(this->blockNum);
    // printf("after getbuffer loadblock");
  if(bufferNum!=E_BLOCKNOTINBUFFER){
    for (int i = 0; i < BUFFER_CAPACITY; i++) {
            if (!StaticBuffer::metainfo[i].free) {
                StaticBuffer::metainfo[i].timeStamp += 1;
            }  
        }
    StaticBuffer::metainfo[bufferNum].timeStamp==0;
  }


  if (bufferNum == E_BLOCKNOTINBUFFER) {
    bufferNum = StaticBuffer::getFreeBuffer(this->blockNum);

    if (bufferNum == E_OUTOFBOUND) {
      return E_OUTOFBOUND;
    }

    Disk::readBlock(StaticBuffer::blocks[bufferNum], this->blockNum);
  }

  // store the pointer to this buffer (blocks[bufferNum]) in *buffPtr
  *buffPtr = StaticBuffer::blocks[bufferNum];

  return SUCCESS;
}

//stage-4-----
/* used to get the slotmap from a record block
NOTE: this function expects the caller to allocate memory for *slotMap
*/
int RecBuffer::getSlotMap(unsigned char *slotMap) {
  unsigned char *bufferPtr;

  // get the starting address of the buffer containing the block using loadBlockAndGetBufferPtr().
  int ret = loadBlockAndGetBufferPtr(&bufferPtr);
  if (ret != SUCCESS) {
    return ret;
  }

  struct HeadInfo head;
  // get the header of the block using getHeader() function
  BlockBuffer::getHeader(&head);

  int slotCount = head.numSlots;

  // get a pointer to the beginning of the slotmap in memory by offsetting HEADER_SIZE
  unsigned char *slotMapInBuffer = bufferPtr + HEADER_SIZE;

  // copy the values from slotMapInBuffer to slotMap (size is slotCount)
  memcpy(slotMap,slotMapInBuffer,slotCount);

  return SUCCESS;
}


int compareAttrs(union Attribute attr1, union Attribute attr2, int attrType) {

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