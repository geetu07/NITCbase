#include "StaticBuffer.h"
#include <algorithm>

unsigned char StaticBuffer::blocks[BUFFER_CAPACITY][BLOCK_SIZE];
struct BufferMetaInfo StaticBuffer::metainfo[BUFFER_CAPACITY];

StaticBuffer::StaticBuffer() {
  for (int bufferIndex=0;bufferIndex<BUFFER_CAPACITY;bufferIndex++) {
    metainfo[bufferIndex].free = true;
    metainfo[bufferIndex].dirty=false;
    metainfo[bufferIndex].timeStamp=-1;
    metainfo[bufferIndex].blockNum=-1;
  }
}

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