#include "BlockAccess.h"

#include <cstring>
#include <iostream>
RecId BlockAccess::linearSearch(int relId, char attrName[ATTR_SIZE], union Attribute attrVal, int op) {
    // get the previous search index of the relation relId from the relation cache
    // (use RelCacheTable::getSearchIndex() function)
    RecId prevRecId;
    prevRecId.block = -1; prevRecId.slot = -1;
    int ret=RelCacheTable::getSearchIndex(relId,&prevRecId);
     if(ret != SUCCESS){
        return prevRecId;
    }
    // std::cout<<prevRecId.block<<" "<<prevRecId.slot<<std::endl;



    // let block and slot denote the record id of the record being currently checked
    int block,slot;

    // if the current search index record is invalid(i.e. both block and slot = -1)
    if (prevRecId.block == -1 && prevRecId.slot == -1)
    {
        // (no hits from previous search; search should start from the
        // first record itself)
        RelCatEntry relCatEntry;
        RelCacheTable::getRelCatEntry(relId,&relCatEntry);
        block=relCatEntry.firstBlk;
        slot=0;

        // get the first record block of the relation from the relation cache
        // (use RelCacheTable::getRelCatEntry() function of Cache Layer)

        // block = first record block of the relation
        // slot = 0

    }
    else
    {
        // (there is a hit from previous search; search should start from
        // the record next to the search index record)

        // block = search index's block
        // slot = search index's slot + 1
        block=prevRecId.block;
        slot=prevRecId.slot+1;
    }
    



    /* The following code searches for the next record in the relation
       that satisfies the given condition
       We start from the record id (block, slot) and iterate over the remaining
       records of the relation
       
    */
            //    std::cout<<block<<" "<<slot<<std::endl;


    while (block != -1)
    {
        /* create a RecBuffer object for block (use RecBuffer Constructor for
           existing block) */
        RecBuffer recBuffer(block);
        
        HeadInfo head;
        recBuffer.getHeader(&head);
        Attribute record[head.numAttrs];
        recBuffer.getRecord(record,slot);

        unsigned char slotMap[head.numSlots];
        recBuffer.getSlotMap(slotMap);
        // get the record with id (block, slot) using RecBuffer::getRecord()
        // get header of the block using RecBuffer::getHeader() function
        // get slot map of the block using RecBuffer::getSlotMap() function

        // If slot >= the number of slots per block(i.e. no more slots in this block)
            // std::cout<<block<<" "<<slot<<std::endl;


        if(slot>=head.numSlots)
        {
            // update block = right block of block
            block=head.rblock;
            slot=0;
            // update slot = 0
            continue;  // continue to the beginning of this while loop
        }

        // if slot is free skip the loop
        // (i.e. check if slot'th entry in slot map of block contains SLOT_UNOCCUPIED)
        if(slotMap[slot]==SLOT_UNOCCUPIED){
            // increment slot and continue to the next record slot
            slot++;
            continue;
        }

        // compare record's attribute value to the the given attrVal as below:
        /*
            firstly get the attribute offset for the attrName attribute
            from the attribute cache entry of the relation using
            AttrCacheTable::getAttrCatEntry()
        */
        /* use the attribute offset to get the value of the attribute from
           current record */

        AttrCatEntry attrCatEntry;
        AttrCacheTable::getAttrCatEntry(relId,attrName,&attrCatEntry);


        int cmpVal=compareAttrs(record[attrCatEntry.offset],attrVal,attrCatEntry.attrType);  // will store the difference between the attributes
        // set cmpVal using compareAttrs()

        /* Next task is to check whether this record satisfies the given condition.
           It is determined based on the output of previous comparison and
           the op value received.
           The following code sets the cond variable if the condition is satisfied.
        */
        if (
            (op == NE && cmpVal != 0) ||    // if op is "not equal to"
            (op == LT && cmpVal < 0) ||     // if op is "less than"
            (op == LE && cmpVal <= 0) ||    // if op is "less than or equal to"
            (op == EQ && cmpVal == 0) ||    // if op is "equal to"
            (op == GT && cmpVal > 0) ||     // if op is "greater than"
            (op == GE && cmpVal >= 0)       // if op is "greater than or equal to"
        ) {
            /*
            set the search index in the relation cache as
            the record id of the record that satisfies the given condition
            (use RelCacheTable::setSearchIndex function)

            */
           prevRecId.block=block;
           prevRecId.slot=slot;
            RelCacheTable::setSearchIndex(relId,&prevRecId);
            return RecId{block, slot};
        }

        slot++;
    }

    // no record in the relation with Id relid satisfies the given condition
    return RecId{-1, -1};
}

//stage-6--------------
int BlockAccess::renameRelation(char oldName[ATTR_SIZE], char newName[ATTR_SIZE]){
    /* reset the searchIndex of the relation catalog using
       RelCacheTable::resetSearchIndex() */
    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute newRelationName;    // set newRelationName with newName
    strcpy(newRelationName.sVal,newName);

    char relationName[ATTR_SIZE];
    strcpy(relationName, RELCAT_ATTR_RELNAME);


    RecId searchId=linearSearch(RELCAT_RELID,relationName,newRelationName,EQ);

    // search the relation catalog for an entry with "RelName" = newRelationName
    if(searchId.block!=-1 && searchId.slot!=-1){
        return E_RELEXIST;
    }
    // printf("%d %d\n",searchId.block,searchId.slot);

    // If relation with name newName already exists (result of linearSearch
    //                                               is not {-1, -1})
    //    return E_RELEXIST;


    /* reset the searchIndex of the relation catalog using
       RelCacheTable::resetSearchIndex() */
    RelCacheTable::resetSearchIndex(RELCAT_RELID);


    Attribute oldRelationName;    // set oldRelationName with oldName
    strcpy(oldRelationName.sVal,oldName);
    strcpy(relationName, RELCAT_ATTR_RELNAME);


    searchId=linearSearch(RELCAT_RELID,relationName,oldRelationName,EQ);


    // search the relation catalog for an entry with "RelName" = oldRelationName

    // If relation with name oldName does not exist (result of linearSearch is {-1, -1})
    //    return E_RELNOTEXIST;
    if(searchId.block==-1 && searchId.slot==-1){
        return E_RELNOTEXIST;
    }
    // printf("%d %d\n",searchId.block,searchId.slot);

    RecBuffer recBuffer(searchId.block);
    Attribute rec[RELCAT_NO_ATTRS];
    recBuffer.getRecord(rec,searchId.slot);
    // printf("%s\n",rec[RELCAT_REL_NAME_INDEX].sVal);

    /* get the relation catalog record of the relation to rename using a RecBuffer
       on the relation catalog [RELCAT_BLOCK] and RecBuffer.getRecord function
    */
    /* update the relation name attribute in the record with newName.
       (use RELCAT_REL_NAME_INDEX) */
    // set back the record value using RecBuffer.setRecord
    strcpy(rec[RELCAT_REL_NAME_INDEX].sVal,newName);
    recBuffer.setRecord(rec,searchId.slot);
    // printf("setted record\n");

    /*
    update all the attribute catalog entries in the attribute catalog corresponding
    to the relation with relation name oldName to the relation name newName
    */

    /* reset the searchIndex of the attribute catalog using
       RelCacheTable::resetSearchIndex() */
    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

    for(int i=0;i<RELCAT_NO_ATTRS;i++){
    strcpy(relationName, ATTRCAT_ATTR_RELNAME);
    // printf("hey");

    searchId = linearSearch(ATTRCAT_RELID, relationName, oldRelationName, EQ);
    if (searchId.block == -1 && searchId.slot == -1) break;

    RecBuffer attrcatBlock(searchId.block);
    Attribute attrCatRec[ATTRCAT_NO_ATTRS];
    attrcatBlock.getRecord(attrCatRec,searchId.slot);

    // SAFELY copy newName into the record
    strncpy(attrCatRec[ATTRCAT_REL_NAME_INDEX].sVal, newName, ATTR_SIZE-1);
    attrCatRec[ATTRCAT_REL_NAME_INDEX].sVal[ATTR_SIZE-1] = '\0';

    attrcatBlock.setRecord(attrCatRec,searchId.slot);
}

    //for i = 0 to numberOfAttributes :
    //    linearSearch on the attribute catalog for relName = oldRelationName
    //    get the record using RecBuffer.getRecord
    //
    //    update the relName field in the record to newName
    //    set back the record using RecBuffer.setRecord

    return SUCCESS;
}

int BlockAccess::renameAttribute(char relName[ATTR_SIZE], char oldName[ATTR_SIZE], char newName[ATTR_SIZE]) {

    /* reset the searchIndex of the relation catalog using
       RelCacheTable::resetSearchIndex() */
       RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute relNameAttr;    // set relNameAttr to relName
    strcpy(relNameAttr.sVal,relName);
    char relationName[ATTR_SIZE];
    strcpy(relationName, RELCAT_ATTR_RELNAME);

    RecId searchId=linearSearch(RELCAT_RELID,relationName,relNameAttr,EQ);
    if(searchId.block==-1 && searchId.slot==-1){
        return E_RELNOTEXIST;
    }

    // Search for the relation with name relName in relation catalog using linearSearch()
    // If relation with name relName does not exist (search returns {-1,-1})
    //    return E_RELNOTEXIST;
    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

    /* reset the searchIndex of the attribute catalog using
       RelCacheTable::resetSearchIndex() */

    /* declare variable attrToRenameRecId used to store the attr-cat recId
    of the attribute to rename */
    RecId attrToRenameRecId{-1, -1};
    Attribute attrCatEntryRecord[ATTRCAT_NO_ATTRS];
    

    /* iterate over all Attribute Catalog Entry record corresponding to the
       relation to find the required attribute */
    while (true) {
        // linear search on the attribute catalog for RelName = relNameAttr
        strcpy(relationName, ATTRCAT_ATTR_RELNAME);
        RecId newId=linearSearch(ATTRCAT_RELID,relationName,relNameAttr,EQ);
        if(newId.block==-1 && newId.slot==-1){
            break;
        }

        // if there are no more attributes left to check (linearSearch returned {-1,-1})
        //     break;
        
        /* Get the record from the attribute catalog using RecBuffer.getRecord
          into attrCatEntryRecord */
        RecBuffer attrcatbuffer(    newId.block);
        attrcatbuffer.getRecord(attrCatEntryRecord,newId.slot);
        if(strcmp(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal,oldName)==0){
            attrToRenameRecId.block=newId.block;
            attrToRenameRecId.slot=newId.slot;
        }
        if(strcmp(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal,newName)==0){
            return E_ATTREXIST;
        }

        // if attrCatEntryRecord.attrName = oldName
        //     attrToRenameRecId = block and slot of this record

        // if attrCatEntryRecord.attrName = newName
        //     return E_ATTREXIST;
    }
    if(attrToRenameRecId.block==-1 && attrToRenameRecId.slot==-1){
        return E_ATTRNOTEXIST;
    }

    // if attrToRenameRecId == {-1, -1}
    //     return E_ATTRNOTEXIST;
    RecBuffer recBuffer(attrToRenameRecId.block);
    recBuffer.getRecord(attrCatEntryRecord,attrToRenameRecId.slot);
    strcpy(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal,newName);
    recBuffer.setRecord(attrCatEntryRecord,attrToRenameRecId.slot);


    // Update the entry corresponding to the attribute in the Attribute Catalog Relation.
    /*   declare a RecBuffer for attrToRenameRecId.block and get the record at
         attrToRenameRecId.slot */
    //   update the AttrName of the record with newName
    //   set back the record with RecBuffer.setRecord

    return SUCCESS;
}