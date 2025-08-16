#include "Schema.h"

#include <cmath>
#include <cstring>
#include <cstdio>

int Schema::openRel(char relName[ATTR_SIZE]) {
  int ret = OpenRelTable::openRel(relName);

  // the OpenRelTable::openRel() function returns the rel-id if successful
  // a valid rel-id will be within the range 0 <= relId < MAX_OPEN and any
  // error codes will be negative
  // printf("sdgasg");
  if(ret >= 0){

  // printf("gdafag");
    return SUCCESS;
  }

  //otherwise it returns an error message
  return ret;
}

int Schema::closeRel(char relName[ATTR_SIZE]) {
  if (relName==RELCAT_RELNAME || relName==ATTRCAT_RELNAME) {
    return E_NOTPERMITTED;
  }

  // this function returns the rel-id of a relation if it is open or
  // E_RELNOTOPEN if it is not. we will implement this later.
  int relId = OpenRelTable::getRelId(relName);

  if (relId==E_RELNOTOPEN) {
    return E_RELNOTOPEN;
  }

  return OpenRelTable::closeRel(relId);
}

int Schema::renameRel(char oldRelName[ATTR_SIZE], char newRelName[ATTR_SIZE]) {
  if(strcmp(oldRelName,"RELATIONCAT")==0 || strcmp(oldRelName,"ATTRIBUTECAT")==0){
    return E_NOTPERMITTED;
  }
  if(strcmp(newRelName,"RELATIONCAT")==0 || strcmp(newRelName,"ATTRIBUTECAT")==0){
    return E_NOTPERMITTED;
  }
  int retVal=OpenRelTable::getRelId(oldRelName);
  // printf("%d\n",retVal);
  if(retVal!=E_RELNOTOPEN){
    return E_RELOPEN;
  }
  retVal=BlockAccess::renameRelation(oldRelName,newRelName);
  return retVal;

    // if the relation is open
    //    (check if OpenRelTable::getRelId() returns E_RELNOTOPEN)
    //    return E_RELOPEN

    // retVal = BlockAccess::renameRelation(oldRelName, newRelName);
    // return retVal
}

int Schema::renameAttr(char *relName, char *oldAttrName, char *newAttrName) {
    if(strcmp(relName,"RELATIONCAT")==0 || strcmp(relName,"ATTRIBUTECAT")==0){
    return E_NOTPERMITTED;
  }
    int retVal=OpenRelTable::getRelId(relName);
  if(retVal!=E_RELNOTOPEN){
    return E_RELOPEN;
  }
  retVal=BlockAccess::renameAttribute(relName,oldAttrName,newAttrName);
  return retVal;
}