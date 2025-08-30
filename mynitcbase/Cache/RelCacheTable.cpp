#include "RelCacheTable.h"

#include <cstring>

RelCacheEntry* RelCacheTable::relCache[MAX_OPEN];

int RelCacheTable::getRelCatEntry(int relId, RelCatEntry* relCatBuff) {
  if (relId < 0 || relId >= MAX_OPEN) {
    return E_OUTOFBOUND;
  }

  if (relCache[relId] == nullptr) {
    return E_RELNOTOPEN;
  }
  *relCatBuff = relCache[relId]->relCatEntry;

  return SUCCESS;
}


void RelCacheTable::recordToRelCatEntry(union Attribute record[RELCAT_NO_ATTRS], RelCatEntry* relCatEntry) {
  strcpy(relCatEntry->relName, record[RELCAT_REL_NAME_INDEX].sVal);
  relCatEntry->numAttrs = (int)record[RELCAT_NO_ATTRIBUTES_INDEX].nVal;
    relCatEntry->numRecs=(int)record[RELCAT_NO_RECORDS_INDEX].nVal;
    relCatEntry->firstBlk=(int)record[RELCAT_FIRST_BLOCK_INDEX].nVal;
    relCatEntry->lastBlk=(int)record[RELCAT_LAST_BLOCK_INDEX].nVal;
    relCatEntry->numSlotsPerBlk=(int)record[RELCAT_NO_SLOTS_PER_BLOCK_INDEX].nVal;

  /* fill the rest of the relCatEntry struct with the values at
      RELCAT_NO_RECORDS_INDEX,
      RELCAT_FIRST_BLOCK_INDEX,
      RELCAT_LAST_BLOCK_INDEX,
      RELCAT_NO_SLOTS_PER_BLOCK_INDEX */
}

/* will return the searchIndex for the relation corresponding to `relId
NOTE: this function expects the caller to allocate memory for `*searchIndex`
*/
int RelCacheTable::getSearchIndex(int relId, RecId* searchIndex) {
  // check if 0 <= relId < MAX_OPEN and return E_OUTOFBOUND otherwise
  if(relId<0 || relId>=MAX_OPEN)
    return E_OUTOFBOUND;

  // check if relCache[relId] == nullptr and return E_RELNOTOPEN if true
  if(relCache[relId]==nullptr)
    return E_RELNOTOPEN;
  // copy the searchIndex field of the Relation Cache entry corresponding
  //   to input relId to the searchIndex variable.

  *searchIndex = relCache[relId]->searchIndex;
  return SUCCESS;
}

// sets the searchIndex for the relation corresponding to relId
int RelCacheTable::setSearchIndex(int relId, RecId* searchIndex) {

  // check if 0 <= relId < MAX_OPEN and return E_OUTOFBOUND otherwise
  if(relId<0 || relId>=MAX_OPEN)
    return E_OUTOFBOUND;

  // check if relCache[relId] == nullptr and return E_RELNOTOPEN if true
  if(relCache[relId]==nullptr)
    return E_RELNOTOPEN;
  // update the searchIndex value in the relCache for the relId to the searchIndex argument
  relCache[relId]->searchIndex=*searchIndex;
  return SUCCESS;
}

int RelCacheTable::resetSearchIndex(int relId) {
  // use setSearchIndex to set the search index to {-1, -1}
  RecId s={-1,-1};

  return setSearchIndex(relId,&s);
}



//stage-7

int RelCacheTable::setRelCatEntry(int relId, RelCatEntry *relCatBuf) {

  if(relId<0 || relId>=MAX_OPEN/*relId is outside the range [0, MAX_OPEN-1]*/) {
    return E_OUTOFBOUND;
  }

  if(relCache[relId]==NULL/*entry corresponding to the relId in the Relation Cache Table is free*/) {
    return E_RELNOTOPEN;
  }

  // copy the relCatBuf to the corresponding Relation Catalog entry in
  // the Relation Cache Table.
  relCache[relId]->relCatEntry=*relCatBuf;
  relCache[relId]->dirty=true;

  // set the dirty flag of the corresponding Relation Cache entry in
  // the Relation Cache Table.

  return SUCCESS;
}

void RelCacheTable::relCatEntryToRecord(RelCatEntry *relCatEntry, union Attribute record[RELCAT_NO_ATTRS]){
  strcpy(record[RELCAT_REL_NAME_INDEX].sVal,relCatEntry->relName);
  record[RELCAT_FIRST_BLOCK_INDEX].nVal=relCatEntry->firstBlk;
  record[RELCAT_LAST_BLOCK_INDEX].nVal=relCatEntry->lastBlk;
  record[RELCAT_NO_ATTRIBUTES_INDEX].nVal=relCatEntry->numAttrs;
  record[RELCAT_NO_RECORDS_INDEX].nVal=relCatEntry->numRecs;
  record[RELCAT_NO_SLOTS_PER_BLOCK_INDEX].nVal=relCatEntry->numSlotsPerBlk;
}