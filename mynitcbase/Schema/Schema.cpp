#include "Schema.h"

#include <cmath>
#include <cstring>
#include <cstdio>

int Schema::openRel(char relName[ATTR_SIZE]) {
  int ret = OpenRelTable::openRel(relName);
  if(ret >= 0){
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


int Schema::createRel(char relName[],int nAttrs, char attrs[][ATTR_SIZE],int attrtype[]){

    // declare variable relNameAsAttribute of type Attribute
    Attribute relNameAsAttribute;
    strcpy(relNameAsAttribute.sVal,relName);
    RecId targetRelId;
    RelCacheTable::resetSearchIndex(RELCAT_RELID);
    targetRelId=BlockAccess::linearSearch(RELCAT_RELID,(char *)RELCAT_ATTR_RELNAME,relNameAsAttribute,EQ);
    // copy the relName into relNameAsAttribute.sVal

    // declare a variable targetRelId of type RecId

    // Reset the searchIndex using RelCacheTable::resetSearhIndex()
    // Search the relation catalog (relId given by the constant RELCAT_RELID)
    // for attribute value attribute "RelName" = relNameAsAttribute using
    // BlockAccess::linearSearch() with OP = EQ

    // if a relation with name relName already exists  ( linearSearch() does
    //                                                     not return {-1,-1} )
    //     return E_RELEXIST;
    if(targetRelId.block!=-1 ||  targetRelId.slot!=-1){
      return E_RELEXIST;
    }
    for(int i=0;i<nAttrs;i++){
      for(int j=i+1;j<nAttrs;j++){
        if(strcmp(attrs[i],attrs[j])==0){
          return E_DUPLICATEATTR;
        }
      }
    }
    // compare every pair of attributes of attrNames[] array
    // if any attribute names have same string value,
    //     return E_DUPLICATEATTR (i.e 2 attributes have same value)

    /* declare relCatRecord of type Attribute which will be used to store the
       record corresponding to the new relation which will be inserted
       into relation catalog */
    Attribute relCatRecord[RELCAT_NO_ATTRS];
    strcpy(relCatRecord[RELCAT_REL_NAME_INDEX].sVal,relName);
    relCatRecord[RELCAT_NO_ATTRIBUTES_INDEX].nVal=nAttrs;
    relCatRecord[RELCAT_NO_RECORDS_INDEX].nVal=0;
    relCatRecord[RELCAT_FIRST_BLOCK_INDEX].nVal=-1;
    relCatRecord[RELCAT_LAST_BLOCK_INDEX].nVal=-1;
    relCatRecord[RELCAT_NO_SLOTS_PER_BLOCK_INDEX].nVal=floor((2016 / (16 * nAttrs + 1)));
   
    int retVal=BlockAccess::insert(RELCAT_RELID,relCatRecord);
    if(retVal<0){
      return retVal;
    }

    // retVal = BlockAccess::insert(RELCAT_RELID(=0), relCatRecord);
    // if BlockAccess::insert fails return retVal
    // (this call could fail if there is no more space in the relation catalog)

    for(int i=0;i<nAttrs;i++)
    {
        /* declare Attribute attrCatRecord[6] to store the attribute catalog
           record corresponding to i'th attribute of the argument passed*/
          Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
        // (where i is the iterator of the loop)
        // fill attrCatRecord fields as given below
        strcpy(attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal,relName);
        strcpy(attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal,attrs[i]);
        attrCatRecord[ATTRCAT_ATTR_TYPE_INDEX].nVal=attrtype[i];
        attrCatRecord[ATTRCAT_PRIMARY_FLAG_INDEX].nVal=-1;
        attrCatRecord[ATTRCAT_ROOT_BLOCK_INDEX].nVal=-1;
        attrCatRecord[ATTRCAT_OFFSET_INDEX].nVal=i;
        retVal=BlockAccess::insert(ATTRCAT_RELID,attrCatRecord);
        if(retVal<0){
          Schema::deleteRel(relName);
          return E_DISKFULL;
        }

        // retVal = BlockAccess::insert(ATTRCAT_RELID(=1), attrCatRecord);
        /* if attribute catalog insert fails:
             delete the relation by calling deleteRel(targetrel) of schema layer
             return E_DISKFULL
             // (this is necessary because we had already created the
             //  relation catalog entry which needs to be removed)
        */
    }

    return SUCCESS;
}

int Schema::deleteRel(char *relName) {
  if(strcmp(relName,RELCAT_RELNAME)==0 || strcmp(relName,ATTRCAT_RELNAME)==0){
    return E_NOTPERMITTED;
  }
    // if the relation to delete is either Relation Catalog or Attribute Catalog,
    //     return E_NOTPERMITTED
        // (check if the relation names are either "RELATIONCAT" and "ATTRIBUTECAT".
        // you may use the following constants: RELCAT_RELNAME and ATTRCAT_RELNAME)
    int relId=OpenRelTable::getRelId(relName);
    if(relId>0 and relId<MAX_OPEN)return E_RELOPEN;
    int ret=BlockAccess::deleteRelation(relName);
    return ret;
    // get the rel-id using appropriate method of OpenRelTable class by
    // passing relation name as argument

    // if relation is opened in open relation table, return E_RELOPEN

    // Call BlockAccess::deleteRelation() with appropriate argument.

    // return the value returned by the above deleteRelation() call

    /* the only that should be returned from deleteRelation() is E_RELNOTEXIST.
       The deleteRelation call may return E_OUTOFBOUND from the call to
       loadBlockAndGetBufferPtr, but if your implementation so far has been
       correct, it should not reach that point. That error could only occur
       if the BlockBuffer was initialized with an invalid block number.
    */
}

//stage 11

int Schema::createIndex(char relName[ATTR_SIZE],char attrName[ATTR_SIZE]){

    if(strcmp(relName,RELCAT_RELNAME)==0 || strcmp(relName,ATTRCAT_RELNAME)==0)
      return E_NOTPERMITTED;
    
    int relId=OpenRelTable::getRelId(relName);
    if(relId==E_RELNOTOPEN)return E_RELNOTOPEN;

    return BPlusTree::bPlusCreate(relId, attrName);
}

int Schema::dropIndex(char *relName, char *attrName) {
    if(strcmp(relName,RELCAT_RELNAME)==0 || strcmp(relName,ATTRCAT_RELNAME)==0)
      return E_NOTPERMITTED;
    
    int relId=OpenRelTable::getRelId(relName);
    if(relId==E_RELNOTOPEN)return E_RELNOTOPEN;

    AttrCatEntry attrCatEntry;
    int ret=AttrCacheTable::getAttrCatEntry(relId,attrName,&attrCatEntry);
    if(ret!=SUCCESS)return E_ATTRNOTEXIST;


    int rootBlock = attrCatEntry.rootBlock;

    if (rootBlock == -1) {
        return E_NOINDEX;
    }

    BPlusTree::bPlusDestroy(rootBlock);
    attrCatEntry.rootBlock=-1;
    AttrCacheTable::setAttrCatEntry(relId,attrName,&attrCatEntry);
    return SUCCESS;
}