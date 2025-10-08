#include "StaticBuffer.h"
#include <algorithm>

unsigned char StaticBuffer::blocks[BUFFER_CAPACITY][BLOCK_SIZE];
struct BufferMetaInfo StaticBuffer::metainfo[BUFFER_CAPACITY];

int StaticBuffer::count=0;

// declare the blockAllocMap array
unsigned char StaticBuffer::blockAllocMap[DISK_BLOCKS];

StaticBuffer::StaticBuffer() {
  // copy blockAllocMap blocks from disk to buffer (using readblock() of disk)
  // blocks 0 to 3
  for(int i=0;i<=3;i++){
    Disk::readBlock(&blockAllocMap[BLOCK_SIZE*i],i);
  }

  for (int bufferIndex=0;bufferIndex<BUFFER_CAPACITY;bufferIndex++) {
    metainfo[bufferIndex].free = true;
    metainfo[bufferIndex].dirty=false;
    metainfo[bufferIndex].timeStamp=-1;
    metainfo[bufferIndex].blockNum=-1;
  }
}

StaticBuffer::~StaticBuffer() {
  // copy blockAllocMap blocks from buffer to disk(using writeblock() of disk)
  for(int i=0;i<=3;i++){
    Disk::writeBlock(&blockAllocMap[BLOCK_SIZE*i],i);
  }
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

int StaticBuffer::getBufferNum(int blockNum) {
  if(blockNum<0 || blockNum>DISK_BLOCKS){
    return E_OUTOFBOUND;
  }
  for(int i=0;i<BUFFER_CAPACITY;i++){
    if(metainfo[i].blockNum==blockNum){
        return i;
    }
  }
  return E_BLOCKNOTINBUFFER;
}


int StaticBuffer::setDirtyBit(int blockNum){
    int bufferIndex=getBufferNum(blockNum);
    if(bufferIndex==E_BLOCKNOTINBUFFER){
      return E_BLOCKNOTINBUFFER;
    }
    if(bufferIndex==E_OUTOFBOUND){
      return E_OUTOFBOUND;
    }
    metainfo[bufferIndex].dirty=true;
    return SUCCESS;
}

//stage 10
int StaticBuffer::getStaticBlockType(int blockNum){
    // Check if blockNum is valid (non zero and less than number of disk blocks)
    // and return E_OUTOFBOUND if not valid.
    if(blockNum<0 || blockNum>DISK_BLOCKS-1)
      return E_OUTOFBOUND;

    // Access the entry in block allocation map corresponding to the blockNum argument
    // and return the block type after type casting to integer.
    return (int)blockAllocMap[blockNum];
}