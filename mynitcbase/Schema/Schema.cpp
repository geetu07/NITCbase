#include "Schema.h"

#include <cmath>
#include <cstring>
#include <cstdio>

int Schema::openRel(char relName[ATTR_SIZE]) {
  int ret = OpenRelTable::openRel(relName);
  if(ret >= 0 && ret<MAX_OPEN){
    return SUCCESS;
  }
  return ret;
}

int Schema::closeRel(char relName[ATTR_SIZE]) {
  if (relName==RELCAT_RELNAME || relName==ATTRCAT_RELNAME) {
    return E_NOTPERMITTED;
  }
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
  if(retVal!=E_RELNOTOPEN){
    return E_RELOPEN;
  }
  retVal=BlockAccess::renameRelation(oldRelName,newRelName);
  return retVal;

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