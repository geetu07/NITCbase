#include "OpenRelTable.h"
#include "AttrCacheTable.h"
#include "RelCacheTable.h"
#include <cstdlib>
#include <cstring>

OpenRelTableMetaInfo OpenRelTable::tableMetaInfo[MAX_OPEN];

OpenRelTable::OpenRelTable() {
    // initialize relCache and attrCache with nullptr
    for (int i = 0; i < MAX_OPEN; i++) {
        RelCacheTable::relCache[i] = nullptr;
        AttrCacheTable::attrCache[i] = nullptr;
        OpenRelTable::tableMetaInfo[i].free = true;
    }

    /************ Setting up Relation Cache entries ************/
    // populating relation cache with entries from relation catalog and attribute
    // catalog

    /**** setting up Relation Catalog relation in the Relation Cache Table****/
    RecBuffer relCatBlock(RELCAT_BLOCK);

    Attribute relCatRecord[RELCAT_NO_ATTRS];
    relCatBlock.getRecord(relCatRecord, RELCAT_SLOTNUM_FOR_RELCAT);

    RelCacheEntry relCacheEntry;
    RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry.relCatEntry);
    relCacheEntry.recId.block = RELCAT_BLOCK;
    relCacheEntry.recId.slot = RELCAT_SLOTNUM_FOR_RELCAT;

    // allocate this on the heap because we want it to persist outside this
    // function
    RelCacheTable::relCache[RELCAT_RELID] =
      (RelCacheEntry *)malloc(sizeof(RelCacheEntry));
    *(RelCacheTable::relCache[RELCAT_RELID]) = relCacheEntry;

    /**** setting up Attribute Catalog relation in the Relation Cache Table ****/
    relCatBlock.getRecord(relCatRecord, RELCAT_SLOTNUM_FOR_ATTRCAT);
    RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry.relCatEntry);
    relCacheEntry.recId.block = RELCAT_BLOCK;
    relCacheEntry.recId.slot = RELCAT_SLOTNUM_FOR_ATTRCAT;

    // allocate this on the heap because we want it to persist outside this
    // function
    RelCacheTable::relCache[ATTRCAT_RELID] =
      (RelCacheEntry *)malloc(sizeof(RelCacheEntry));
    *(RelCacheTable::relCache[ATTRCAT_RELID]) = relCacheEntry;

    /************ Setting up Attribute cache entries ************/
    // populating attribute cache with entries from relation catalog and attribute
    // catalog

    /**** setting up Relation Catalog relation in the Attribute Cache Table ****/
    RecBuffer attrCatBlock(ATTRCAT_BLOCK);

    Attribute attrCatRecord[ATTRCAT_NO_ATTRS];

    AttrCacheEntry *prevAttr = nullptr;
    for (int i = 0; i <= 5; i++) {
        attrCatBlock.getRecord(attrCatRecord, i);
        AttrCacheEntry attrCacheEntry;
        AttrCacheTable::recordToAttrCatEntry(attrCatRecord,
                                             &attrCacheEntry.attrCatEntry);
        attrCacheEntry.recId.block = ATTRCAT_BLOCK;
        attrCacheEntry.recId.slot = i;
        if (prevAttr != nullptr) {
            prevAttr->next = (AttrCacheEntry *)malloc(sizeof(AttrCacheEntry));
            prevAttr = prevAttr->next;
        } else {
            AttrCacheTable::attrCache[RELCAT_RELID] =
              (AttrCacheEntry *)malloc(sizeof(AttrCacheEntry));
            prevAttr = AttrCacheTable::attrCache[RELCAT_RELID];
        }
        *prevAttr = attrCacheEntry;
    }
    prevAttr->next = nullptr;

    /**** setting up Attribute Catalog relation in the Attribute Cache Table ****/
    prevAttr = nullptr;
    for (int i = 6; i <= 11; i++) {
        attrCatBlock.getRecord(attrCatRecord, i);
        AttrCacheEntry attrCacheEntry;
        AttrCacheTable::recordToAttrCatEntry(attrCatRecord,
                                             &attrCacheEntry.attrCatEntry);
        attrCacheEntry.recId.block = ATTRCAT_BLOCK;
        attrCacheEntry.recId.slot = i;
        if (prevAttr != nullptr) {
            prevAttr->next = (AttrCacheEntry *)malloc(sizeof(AttrCacheEntry));
            prevAttr = prevAttr->next;
        } else {
            AttrCacheTable::attrCache[ATTRCAT_RELID] =
              (AttrCacheEntry *)malloc(sizeof(AttrCacheEntry));
            prevAttr = AttrCacheTable::attrCache[ATTRCAT_RELID];
        }
        *prevAttr = attrCacheEntry;
    }
    prevAttr->next = nullptr;

    tableMetaInfo[RELCAT_RELID].free = false;
    tableMetaInfo[ATTRCAT_RELID].free = false;
    strcpy(tableMetaInfo[RELCAT_RELID].relName, RELCAT_RELNAME);
    strcpy(tableMetaInfo[ATTRCAT_RELID].relName, ATTRCAT_RELNAME);
}

OpenRelTable::~OpenRelTable() {

    // close all open relations (rel-id = 2 to MAX_OPEN)
    for (int i = 2; i < MAX_OPEN; i++) {
        if (tableMetaInfo[i].free == false) {
            OpenRelTable::closeRel(i);
        }
    }

    // free memory allocated to rel-id = 0 and rel-id = 1 in the caches
    free(RelCacheTable::relCache[RELCAT_RELID]);
    free(RelCacheTable::relCache[ATTRCAT_RELID]);
    RelCacheTable::relCache[RELCAT_RELID] = nullptr;
    RelCacheTable::relCache[ATTRCAT_RELID] = nullptr;

    AttrCacheEntry *nextAttr = AttrCacheTable::attrCache[RELCAT_RELID];
    while (nextAttr != nullptr) {
        nextAttr = nextAttr->next;
        free(AttrCacheTable::attrCache[RELCAT_RELID]);
        AttrCacheTable::attrCache[RELCAT_RELID] = nextAttr;
    }

    nextAttr = AttrCacheTable::attrCache[ATTRCAT_RELID];
    while (nextAttr != nullptr) {
        nextAttr = nextAttr->next;
        free(AttrCacheTable::attrCache[ATTRCAT_RELID]);
        AttrCacheTable::attrCache[ATTRCAT_RELID] = nextAttr;
    }

    tableMetaInfo[RELCAT_RELID].free = true;
    tableMetaInfo[ATTRCAT_RELID].free = true;
    strcpy(tableMetaInfo[RELCAT_RELID].relName, "");
    strcpy(tableMetaInfo[ATTRCAT_RELID].relName, "");
}

int OpenRelTable::getFreeOpenRelTableEntry() {
    for (int i = 2; i < MAX_OPEN; i++) {
        if (tableMetaInfo[i].free) {
            return i;
        }
    }
    return E_CACHEFULL;
}

int OpenRelTable::getRelId(char relName[ATTR_SIZE]) {
    for (int i = 0; i < MAX_OPEN; i++) {
        if (tableMetaInfo[i].free == false) {
            if (strcmp(tableMetaInfo[i].relName, relName) == 0) {
                return i;
            }
        }
    }
    return E_RELNOTOPEN;
}

int OpenRelTable::openRel(char *relName) {
    int relId;
    if ((relId = getRelId(relName)) != E_RELNOTOPEN) {
        return relId;
    }
    if ((relId = getFreeOpenRelTableEntry()) == E_CACHEFULL) {
        return E_CACHEFULL;
    }
    // now relId stores index of a free slot

    /****** Setting up Relation Cache entry for the relation ******/ // Linear search in relation catalog block
    RelCacheTable::resetSearchIndex(RELCAT_RELID);
    Attribute relationName;
    strcpy(relationName.sVal, relName);
    char relname_attr[ATTR_SIZE];
    strcpy(relname_attr, RELCAT_ATTR_RELNAME);
    RecId relcatRecId = BlockAccess::linearSearch(RELCAT_RELID, relname_attr, relationName, EQ);
    if (relcatRecId.block == -1 && relcatRecId.slot == -1) {
        return E_RELNOTEXIST;
    }

    RecBuffer relCatBlock(relcatRecId.block);
    Attribute record[RELCAT_NO_ATTRS];
    relCatBlock.getRecord(record, relcatRecId.slot);
    RelCacheEntry relCacheEntry;
    RelCacheTable::recordToRelCatEntry(record, &relCacheEntry.relCatEntry);
    relCacheEntry.recId = relcatRecId;
    // relCacheEntry.searchIndex = {-1,-1} ??
    RelCacheTable::relCache[relId] = (RelCacheEntry *)malloc(sizeof(RelCacheEntry));
    *(RelCacheTable::relCache[relId]) = relCacheEntry;

    /****** Setting up Attribute Cache entry for the relation ******/ // Linear search in attribute catalog block(s)
    AttrCacheEntry *prevAttr = nullptr;
    int numberofAttrs = RelCacheTable::relCache[relId]->relCatEntry.numAttrs;
    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);
    for (int i = 0; i < numberofAttrs; i++) {
        RecId attrcatRecId = BlockAccess::linearSearch(ATTRCAT_RELID, relname_attr, relationName, EQ);

        RecBuffer attrCatBlock(attrcatRecId.block);
        Attribute record[ATTRCAT_NO_ATTRS];
        attrCatBlock.getRecord(record, attrcatRecId.slot);
        AttrCacheEntry attrCacheEntry;
        AttrCacheTable::recordToAttrCatEntry(record, &attrCacheEntry.attrCatEntry);
        attrCacheEntry.recId.block = attrcatRecId.block;
        attrCacheEntry.recId.slot = attrcatRecId.slot;

        if (prevAttr == nullptr) {
            AttrCacheTable::attrCache[relId] = (AttrCacheEntry *)malloc(sizeof(AttrCacheEntry));
            prevAttr = AttrCacheTable::attrCache[relId];
        } else {
            prevAttr->next = (AttrCacheEntry *)malloc(sizeof(AttrCacheEntry));
            prevAttr = prevAttr->next;
        }
        *prevAttr = attrCacheEntry;
    }
    prevAttr->next = nullptr;

    // Updating tableMetaInfo values
    tableMetaInfo[relId].free = false;
    strcpy(tableMetaInfo[relId].relName, relName);

    return relId;
}

// int OpenRelTable::closeRel(int relId) {
//     if (relId==0 || relId==1) {/* rel-id corresponds to relation catalog or attribute catalog*/
//     return E_NOTPERMITTED;
//   }

//   if (relId>=MAX_OPEN || relId<0) {/* 0 <= relId < MAX_OPEN */
//     return E_OUTOFBOUND;
//   }

//   if (tableMetaInfo[relId].free) {/* rel-id corresponds to a free slot*/
//     return E_RELNOTOPEN;
//   }

//   AttrCacheEntry* head=AttrCacheTable::attrCache[relId];
//   for(AttrCacheEntry *it=head,*next;it!=NULL;it=next){
//     next=it->next;
//     free(it);
//   }
//   tableMetaInfo[relId].free=true;
//   free(RelCacheTable::relCache[relId]);
//   RelCacheTable::relCache[relId]=nullptr;
//   AttrCacheTable::attrCache[relId]=nullptr;
//   strcpy(tableMetaInfo[relId].relName, "");

//   // free the memory allocated in the relation and attribute caches which was
//   // allocated in the OpenRelTable::openRel() function

//   // update `tableMetaInfo` to set `relId` as a free slot
//   // update `relCache` and `attrCache` to set the entry at `relId` to nullptr
//     return SUCCESS;
// }
int OpenRelTable::closeRel(int relId) {
  // confirm that rel-id fits the following conditions
  //     2 <=relId < MAX_OPEN
  //     does not correspond to a free slot
  //  (you have done this already)
        if (relId==0 || relId==1) {
            return E_NOTPERMITTED;
        }

        if (relId>=MAX_OPEN || relId<0) {
            return E_OUTOFBOUND;
        }

        if (tableMetaInfo[relId].free) {
            return E_RELNOTOPEN;
        }
  /****** Releasing the Relation Cache entry of the relation ******/

  if (RelCacheTable::relCache[relId]->dirty/* RelCatEntry of the relId-th Relation Cache entry has been modified */)
  {

    /* Get the Relation Catalog entry from RelCacheTable::relCache
    Then convert it to a record using RelCacheTable::relCatEntryToRecord(). */
    RelCatEntry relCatBuf;
    RelCacheTable::getRelCatEntry(relId,&relCatBuf);

    Attribute record[RELCAT_NO_ATTRS];
    RelCacheTable::relCatEntryToRecord(&relCatBuf,record);
    RecBuffer relCatBlock(RelCacheTable::relCache[relId]->recId.block);
    relCatBlock.setRecord(record,RelCacheTable::relCache[relId]->recId.slot);

    // declaring an object of RecBuffer class to write back to the buffer
    // Write back to the buffer using relCatBlock.setRecord() with recId.slot
  }
  free(RelCacheTable::relCache[relId]);
  RelCacheTable::relCache[relId]=NULL;
  AttrCacheEntry *attr=AttrCacheTable::attrCache[relId];
  AttrCacheEntry*next=NULL;
  while(attr!=NULL){
    next=attr->next;
    free(attr);
    attr=next;
  }
  AttrCacheTable::attrCache[relId]=NULL;

  /****** Releasing the Attribute Cache entry of the relation ******/

  // free the memory allocated in the attribute caches which was
  // allocated in the OpenRelTable::openRel() function

  // (because we are not modifying the attribute cache at this stage,
  // write-back is not required. We will do it in subsequent
  // stages when it becomes needed)


  /****** Set the Open Relation Table entry of the relation as free ******/

  // update `metainfo` to set `relId` as a free slot
  tableMetaInfo[relId].free=true;
  strcpy(tableMetaInfo[relId].relName,"");

  return SUCCESS;
}