#include "StaticBuffer.h"
#include <algorithm>

// the declarations for this class can be found at "StaticBuffer.h"

unsigned char StaticBuffer::blocks[BUFFER_CAPACITY][BLOCK_SIZE];
struct BufferMetaInfo StaticBuffer::metainfo[BUFFER_CAPACITY];

StaticBuffer::StaticBuffer() {

  // initialise all blocks as free
  for (int bufferIndex=0;bufferIndex<BUFFER_CAPACITY;bufferIndex++) {
    metainfo[bufferIndex].free = true;
    metainfo[bufferIndex].dirty=false;
    metainfo[bufferIndex].timeStamp=-1;
    metainfo[bufferIndex].blockNum=-1;


  }
}

/*
At this stage, we are not writing back from the buffer to the disk since we are
not modifying the buffer. So, we will define an empty destructor for now. In
subsequent stages, we will implement the write-back functionality here.
*/
StaticBuffer::~StaticBuffer() {
  for(int i=0;i<BUFFER_CAPACITY;i++){
    if(metainfo[i].free==false && metainfo[i].dirty==true){
      Disk::writeBlock(blocks[i],metainfo[i].blockNum);
    }
  }

}

int StaticBuffer::getFreeBuffer(int blockNum) {
  if (blockNum < 0 || blockNum > DISK_BLOCKS) {
    return E_OUTOFBOUND;
  }
  int maxTime=0;
  for(int i=0;i<BUFFER_CAPACITY;i++){
    if(!metainfo[i].free){
      metainfo[i].timeStamp++;
      if(metainfo[i].timeStamp>metainfo[maxTime].timeStamp){
        maxTime=i;
      }
    }
  }
  int bufferNum=-1;

  // iterate through all the blocks in the StaticBuffer
  // find the first free block in the buffer (check metainfo)
  // assign allocatedBuffer = index of the free block

  for(int i=0;i<BUFFER_CAPACITY;i++){
    if(metainfo[i].free==true){
        bufferNum=i;
        break;
    }
  }
  if(bufferNum==-1){
    if(metainfo[maxTime].dirty){
      Disk::writeBlock(blocks[maxTime],metainfo[maxTime].blockNum);
    }
    bufferNum=maxTime;
  }


  

  metainfo[bufferNum].free = false;
  metainfo[bufferNum].blockNum = blockNum;
  metainfo[bufferNum].dirty=false;
  metainfo[bufferNum].blockNum=blockNum;

  return bufferNum;
}

/* Get the buffer index where a particular block is stored
   or E_BLOCKNOTINBUFFER otherwise
*/
int StaticBuffer::getBufferNum(int blockNum) {
  // Check if blockNum is valid (between zero and DISK_BLOCKS)
  // and return E_OUTOFBOUND if not valid.

  if(blockNum<0 || blockNum>DISK_BLOCKS){
    return E_OUTOFBOUND;
  }

  // find and return the bufferIndex which corresponds to blockNum (check metainfo)
  for(int i=0;i<BUFFER_CAPACITY;i++){
    if(metainfo[i].blockNum==blockNum){
        return i;
    }
  }

  // if block is not in the buffer
  return E_BLOCKNOTINBUFFER;
}


int StaticBuffer::setDirtyBit(int blockNum){
    int bufferIndex=getBufferNum(blockNum);
    // find the buffer index corresponding to the block using getBufferNum().
    if(bufferIndex==E_BLOCKNOTINBUFFER){
      return E_BLOCKNOTINBUFFER;
    }

    // if block is not present in the buffer (bufferNum = E_BLOCKNOTINBUFFER)
    //     return E_BLOCKNOTINBUFFER
    if(bufferIndex==E_OUTOFBOUND){
      return E_OUTOFBOUND;
    }
    // if blockNum is out of bound (bufferNum = E_OUTOFBOUND)
    //     return E_OUTOFBOUND
    metainfo[bufferIndex].dirty=true;
    // else
    //     (the bufferNum is valid)
    //     set the dirty bit of that buffer to true in metainfo

    // return SUCCESS
    return SUCCESS;
}