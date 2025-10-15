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

//stage-8........
OpenRelTable::~OpenRelTable() {

    for(int i=2;i<MAX_OPEN;i++)
    {
        if(!tableMetaInfo[i].free)
        {
            OpenRelTable::closeRel(i);
        }
    }

    if (RelCacheTable::relCache[ATTRCAT_RELID]->dirty) {
      RelCatEntry relCatEntry;
      RelCacheTable::getRelCatEntry(ATTRCAT_RELID,&relCatEntry);
      Attribute relCatRecord[ATTRCAT_NO_ATTRS];
      RelCacheTable::relCatEntryToRecord(&relCatEntry,relCatRecord);
      RecId recId=RelCacheTable::relCache[ATTRCAT_RELID]->recId;

        RecBuffer relCatBlock(recId.block);
        relCatBlock.setRecord(relCatRecord,recId.slot);

    }
    free(RelCacheTable::relCache[ATTRCAT_RELID]);

    if(RelCacheTable::relCache[RELCAT_RELID]->dirty) {
      RelCatEntry relCatEntry;

      RelCacheTable::getRelCatEntry(RELCAT_RELID,&relCatEntry);
      Attribute relCatRecord[RELCAT_NO_ATTRS];
      RelCacheTable::relCatEntryToRecord(&relCatEntry,relCatRecord);
      RecId recId=RelCacheTable::relCache[RELCAT_RELID]->recId;

        RecBuffer relCatBlock(recId.block);
        relCatBlock.setRecord(relCatRecord,recId.slot);

    }
    free(RelCacheTable::relCache[RELCAT_RELID]);
    for(int i=0;i<=1;i++){
      AttrCacheEntry* head=AttrCacheTable::attrCache[i];
      while(head!=nullptr){
        AttrCacheEntry* temp=head->next;
        if(head->dirty){
          AttrCatEntry attrCatEntry=head->attrCatEntry;
          Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
          AttrCacheTable::attrCatEntryToRecord(&attrCatEntry,attrCatRecord);
          RecId recId=head->recId;
          RecBuffer attrCatBlock(recId.block);
          attrCatBlock.setRecord(attrCatRecord,recId.slot);
        }
        free(head);
        head=temp;
      }
    }
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

  /****** Releasing the Attribute Cache entry of the relation ******/

    // for all the entries in the linked list of the relIdth Attribute Cache entry.
    for(AttrCacheEntry *entry=AttrCacheTable::attrCache[relId];entry!=NULL;entry=entry->next){
        if(entry->dirty==true)
        {
            /* Get the Attribute Catalog entry from attrCache
             Then convert it to a record using AttrCacheTable::attrCatEntryToRecord().
             Write back that entry by instantiating RecBuffer class. Use recId
             member field and recBuffer.setRecord() */
             AttrCatEntry attrCatEntry;
             attrCatEntry=entry->attrCatEntry;
             Attribute record[ATTRCAT_NO_ATTRS];
             AttrCacheTable::attrCatEntryToRecord(&attrCatEntry,record);
             RecBuffer attrCatBlock(entry->recId.block);
             attrCatBlock.setRecord(record,entry->recId.slot);
        }
    }

    /****** Updating metadata in the Open Relation Table of the relation  ******/

    //free the relIdth entry of the tableMetaInfo.

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
  tableMetaInfo[relId].free=true;
  strcpy(tableMetaInfo[relId].relName,"");

  return SUCCESS;
}