#include <cstring>
#include <iostream>
#include "BlockAccess.h"
#include <cstring>
#include <stdlib.h>
#include <cstdio>


RecId BlockAccess::linearSearch(int relId, char attrName[ATTR_SIZE], union Attribute attrVal, int op) {
    RecId prevRecId;
    RelCacheTable::getSearchIndex(relId, &prevRecId);
    int block = -1;
    int slot = -1;

    RelCatEntry relCatBuf;
    RelCacheTable::getRelCatEntry(relId, &relCatBuf);

    if (prevRecId.block == -1 && prevRecId.slot == -1) {
        

        block = relCatBuf.firstBlk;
        slot = 0;
    }
    else {
        block = prevRecId.block;
        slot = prevRecId.slot + 1;
    }
    while (block != -1) {
        RecBuffer recBuffer(block);
        Attribute recordEntry[relCatBuf.numAttrs];
        HeadInfo header;

        recBuffer.getHeader(&header);
        unsigned char slotMap[header.numSlots];
        recBuffer.getSlotMap(slotMap);


        if (slot >= header.numSlots) {
            block = header.rblock;
            slot = 0;
            continue;
        }

        if (slotMap[slot] == SLOT_UNOCCUPIED) {
            slot++;
            continue;
        }

        recBuffer.getRecord(recordEntry, slot);
        AttrCatEntry attrCatBuf;
        int ret = AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatBuf);
        Attribute currRecordAttr = recordEntry[attrCatBuf.offset];

        int cmpVal = compareAttrs(currRecordAttr, attrVal, attrCatBuf.attrType);

        if (
            (op == NE && cmpVal != 0) ||    // if op is "not equal to"
            (op == LT && cmpVal < 0) ||     // if op is "less than"
            (op == LE && cmpVal <= 0) ||    // if op is "less than or equal to"
            (op == EQ && cmpVal == 0) ||    // if op is "equal to"
            (op == GT && cmpVal > 0) ||     // if op is "greater than"
            (op == GE && cmpVal >= 0)       // if op is "greater than or equal to"
        ) 
        {
            RecId searchIndex = {block, slot};
            RelCacheTable::setSearchIndex(relId, &searchIndex);
            return searchIndex;
        }
        
        slot++;
    }

    return RecId({-1, -1});
}



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

    RecBuffer recBuffer(searchId.block);
    Attribute rec[RELCAT_NO_ATTRS];
    recBuffer.getRecord(rec,searchId.slot);

    /* get the relation catalog record of the relation to rename using a RecBuffer
       on the relation catalog [RELCAT_BLOCK] and RecBuffer.getRecord function
    */
    /* update the relation name attribute in the record with newName.
       (use RELCAT_REL_NAME_INDEX) */
    // set back the record value using RecBuffer.setRecord
    strcpy(rec[RELCAT_REL_NAME_INDEX].sVal,newName);
    recBuffer.setRecord(rec,searchId.slot);
    /*
    update all the attribute catalog entries in the attribute catalog corresponding
    to the relation with relation name oldName to the relation name newName
    */

    /* reset the searchIndex of the attribute catalog using
       RelCacheTable::resetSearchIndex() */
    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

    for(int i=0;i<RELCAT_NO_ATTRS;i++){
    strcpy(relationName, ATTRCAT_ATTR_RELNAME);

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
        RecBuffer attrcatbuffer(newId.block);
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


//stage-7
int BlockAccess::insert(int relId, Attribute *record) {
    
    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(relId,&relCatEntry);

    int blockNum = relCatEntry.firstBlk;

    RecId rec_id = {-1, -1};
    int numOfSlots = relCatEntry.numSlotsPerBlk;
    int numOfAttributes =relCatEntry.numAttrs ;

    int prevBlockNum = -1;

    while (blockNum != -1) {
        RecBuffer block(blockNum);
        HeadInfo head;
        block.getHeader(&head);
        unsigned char slotmap[head.numSlots];
        block.getSlotMap(slotmap);
           for(int i=0;i<head.numSlots;i++){
            if(slotmap[i]==SLOT_UNOCCUPIED){
                rec_id.block=blockNum;
                rec_id.slot=i;
                break;
            }
           }
           if(rec_id.block!=-1 && rec_id.slot!=-1){
            break;
           }
           else{
            prevBlockNum=blockNum;
            blockNum=head.rblock;
           }
    }

    if(rec_id.block==-1 && rec_id.slot==-1){
        if(strcmp(relCatEntry.relName,RELCAT_RELNAME)==0)
            return E_MAXRELATIONS;

        RecBuffer newb;
        int ret=newb.getBlockNum();
        if (ret == E_DISKFULL) {
            return E_DISKFULL;
        }
        rec_id.block=ret;
        rec_id.slot=0;

       HeadInfo header;
       newb.getHeader(&header);
       header.blockType=REC;
       header.rblock=-1;
       header.numAttrs=numOfAttributes;
       header.numSlots=numOfSlots;
       header.pblock=-1;
       header.numEntries=0;
       header.lblock=prevBlockNum;
        newb.setHeader(&header);
 
       unsigned char newSlotMap[numOfSlots];
        for (int i = 0; i < numOfSlots; i++) {
            newSlotMap[i] = SLOT_UNOCCUPIED;
        }
        newb.setSlotMap(newSlotMap);

        if(prevBlockNum!=-1)
        {
            RecBuffer prev(prevBlockNum);
            HeadInfo head;
            prev.getHeader(&head);
            head.rblock=rec_id.block;
            prev.setHeader(&head);
        }
        else
        {
            relCatEntry.firstBlk=rec_id.block;
        }
            relCatEntry.lastBlk=rec_id.block;
            RelCacheTable::setRelCatEntry(relId,&relCatEntry);
    }

    RecBuffer block(rec_id.block);
    block.setRecord(record,rec_id.slot);
    unsigned char slotmp[numOfSlots];
    block.RecBuffer::getSlotMap(slotmp);
    slotmp[rec_id.slot]=SLOT_OCCUPIED;
    block.RecBuffer::setSlotMap(slotmp);

    HeadInfo header;
    block.getHeader(&header);
    header.numEntries+=1;
    block.setHeader(&header);

    relCatEntry.numRecs+=1;
    RelCacheTable::setRelCatEntry(relId,&relCatEntry);

    return SUCCESS;
}

//stage 8
// int BlockAccess::search(int relId, Attribute *record, char attrName[ATTR_SIZE], Attribute attrVal, int op) {
//     //RelCacheTable::resetSearchIndex(relId);
//     RecId recId=linearSearch(relId,attrName,attrVal,op);
//     if(recId.block==-1 && recId.slot==-1)
//         return E_NOTFOUND;
//     RecBuffer blck(recId.block);
//     blck.getRecord(record,recId.slot);
//     return SUCCESS;
// }
int BlockAccess::deleteRelation(char relName[ATTR_SIZE]) {
    if(strcmp(relName,RELCAT_RELNAME)==0 || strcmp(relName,ATTRCAT_RELNAME)==0){
        return E_NOTPERMITTED;
    }
    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute relNameAttr; 
    strcpy(relNameAttr.sVal,relName);// (stores relName as type union Attribute)
    // assign relNameAttr.sVal = relName
    RecId recid;
    recid=linearSearch(RELCAT_RELID,(char *)RELCAT_ATTR_RELNAME,relNameAttr,EQ);
    //  linearSearch on the relation catalog for RelName = relNameAttr
    if(recid.block==-1 || recid.slot==-1) return E_RELNOTEXIST;

    Attribute relCatEntryRecord[RELCAT_NO_ATTRS];
    RecBuffer recBuffer(recid.block);
    recBuffer.getRecord(relCatEntryRecord,recid.slot);
    int firstBlk=relCatEntryRecord[RELCAT_FIRST_BLOCK_INDEX].nVal;
    int numAttrs=relCatEntryRecord[RELCAT_NO_ATTRIBUTES_INDEX].nVal;

    /*
     Delete all the record blocks of the relation
    */
  
    int currblk=firstBlk;
   
   while(currblk!=-1){
     HeadInfo head;
     RecBuffer currBuffer(currblk);

    currBuffer.getHeader(&head);
    currblk=head.rblock;
    currBuffer.releaseBlock();
   }
   RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

    // reset the searchIndex of the attribute catalog

    int numberOfAttributesDeleted = 0;

    RecId attrCatRecId;
    while(true) {
        attrCatRecId=linearSearch(ATTRCAT_RELID,(char *)ATTRCAT_ATTR_RELNAME,relNameAttr,EQ);
        if(attrCatRecId.block==-1 || attrCatRecId.slot==-1) break;
        // attrCatRecId = linearSearch on attribute catalog for RelName = relNameAttr

        // if no more attributes to iterate over (attrCatRecId == {-1, -1})
        //     break;

        numberOfAttributesDeleted++;
        RecBuffer attrCatBlockBuffer(attrCatRecId.block);

		HeadInfo attrCatHeader;
		attrCatBlockBuffer.getHeader(&attrCatHeader);

		Attribute attrCatRecord [ATTRCAT_NO_ATTRS];
		attrCatBlockBuffer.getRecord(attrCatRecord, attrCatRecId.slot);

        int rootBlock =attrCatRecord[ATTRCAT_ROOT_BLOCK_INDEX].nVal;
        unsigned char slotmap [attrCatHeader.numSlots];
		attrCatBlockBuffer.getSlotMap(slotmap);

		slotmap[attrCatRecId.slot] = SLOT_UNOCCUPIED;
		attrCatBlockBuffer.setSlotMap(slotmap);
        attrCatHeader.numEntries--;
		attrCatBlockBuffer.setHeader(&attrCatHeader);

        if (attrCatHeader.numAttrs==0) {
            RecBuffer prevBlock (attrCatHeader.lblock);
			
			HeadInfo leftHeader;
			prevBlock.getHeader(&leftHeader);

			leftHeader.rblock = attrCatHeader.rblock;
			prevBlock.setHeader(&leftHeader);

            if (attrCatHeader.rblock!=INVALID_BLOCKNUM) {
                RecBuffer nextBlock (attrCatHeader.rblock);
				
				HeadInfo rightHeader;
				nextBlock.getHeader(&rightHeader);

				rightHeader.lblock = attrCatHeader.lblock;
				nextBlock.setHeader(&rightHeader);


            } else {
                RelCatEntry relCatEntryBuffer;
				RelCacheTable::getRelCatEntry(ATTRCAT_RELID, &relCatEntryBuffer);

				relCatEntryBuffer.lastBlk = attrCatHeader.lblock;
                RelCacheTable::setRelCatEntry(ATTRCAT_RELID, &relCatEntryBuffer);
            }

            attrCatBlockBuffer.releaseBlock();
        }

        if (rootBlock != -1) {
            // delete the bplus tree rooted at rootBlock using BPlusTree::bPlusDestroy()
        }
    }

    HeadInfo relCatHeader;
	recBuffer.getHeader(&relCatHeader);

	relCatHeader.numEntries--;
	recBuffer.setHeader(&relCatHeader);

	unsigned char slotmap [relCatHeader.numSlots];
	recBuffer.getSlotMap(slotmap);

	slotmap[recid.slot] = SLOT_UNOCCUPIED;
	recBuffer.setSlotMap(slotmap);


	RelCatEntry relCatEntryBuffer;
	RelCacheTable::getRelCatEntry(RELCAT_RELID, &relCatEntryBuffer);

	relCatEntryBuffer.numRecs--;
	RelCacheTable::setRelCatEntry(RELCAT_RELID, &relCatEntryBuffer);

	RelCacheTable::getRelCatEntry(ATTRCAT_RELID, &relCatEntryBuffer);
	relCatEntryBuffer.numRecs -= numberOfAttributesDeleted;
	RelCacheTable::setRelCatEntry(ATTRCAT_RELID, &relCatEntryBuffer);

    return SUCCESS;
}


//stage-9

/*
NOTE: the caller is expected to allocate space for the argument `record` based
      on the size of the relation. This function will only copy the result of
      the projection onto the array pointed to by the argument.
*/
int BlockAccess::project(int relId, Attribute *record) {
    // get the previous search index of the relation relId from the relation
    // cache (use RelCacheTable::getSearchIndex() function)
    RecId prevRecId;
    int ret=RelCacheTable::getSearchIndex(relId,&prevRecId);
    if(ret!=SUCCESS)return ret;

    // declare block and slot which will be used to store the record id of the
    // slot we need to check.
    int block, slot;

    /* if the current search index record is invalid(i.e. = {-1, -1})
       (this only happens when the caller reset the search index)
    */
    if (prevRecId.block == -1 && prevRecId.slot == -1)
    {
        // (new project operation. start from beginning)

        // get the first record block of the relation from the relation cache
        // (use RelCacheTable::getRelCatEntry() function of Cache Layer)
        RelCatEntry relcatEntry;
        RelCacheTable::getRelCatEntry(relId,&relcatEntry);
        block=relcatEntry.firstBlk;
        slot = 0;
    }
    else
    {
        // (a project/search operation is already in progress)

        // block = previous search index's block
        // slot = previous search index's slot + 1
        block=prevRecId.block;
        slot=prevRecId.slot+1;
    }


    // The following code finds the next record of the relation
    /* Start from the record id (block, slot) and iterate over the remaining
       records of the relation */
    while (block != -1)
    {
        // create a RecBuffer object for block (using appropriate constructor!)
        RecBuffer blk(block);
        HeadInfo head;
        blk.getHeader(&head);
        unsigned char slotmp[head.numSlots];
        blk.getSlotMap(slotmp);

        if(slot >=head.numSlots)
        {
            // (no more slots in this block)
            // update block = right block of block
            block=head.rblock;
            slot=0;
            // update slot = 0
            // (NOTE: if this is the last block, rblock would be -1. this would
            //        set block = -1 and fail the loop condition )
        }
        else if (slotmp[slot]==SLOT_UNOCCUPIED/* slot is free */)
        { // (i.e slot-th entry in slotMap contains SLOT_UNOCCUPIED)
            slot++;
            // increment slot
        }
        else {
            // (the next occupied slot / record has been found)
            break;
        }
    }

    if (block == -1){
        // (a record was not found. all records exhausted)
        return E_NOTFOUND;
    }

    // declare nextRecId to store the RecId of the record found
    RecId nextRecId{block, slot};
    RelCacheTable::setSearchIndex(relId,&nextRecId);

    // set the search index to nextRecId using RelCacheTable::setSearchIndex
    RecBuffer blk(nextRecId.block);
    blk.getRecord(record,nextRecId.slot);
    /* Copy the record with record id (nextRecId) to the record buffer (record)
       For this Instantiate a RecBuffer class object by passing the recId and
       call the appropriate method to fetch the record
    */

    return SUCCESS;
}


//stage 10
int BlockAccess::search(int relId, Attribute *record, char attrName[ATTR_SIZE], Attribute attrVal, int op) {
    // Declare a variable called recid to store the searched record
    RecId recId;
    AttrCatEntry attrcatEntry;
    int ret=AttrCacheTable::getAttrCatEntry(relId,attrName,&attrcatEntry);
    if(ret!=SUCCESS)return ret;

    /* get the attribute catalog entry from the attribute cache corresponding
    to the relation with Id=relid and with attribute_name=attrName  */

    // if this call returns an error, return the appropriate error code
    int rootblck=attrcatEntry.rootBlock;

    // get rootBlock from the attribute catalog entry
    /* if Index does not exist for the attribute (check rootBlock == -1) */
    if(rootblck==-1) {
        /* search for the record id (recid) corresponding to the attribute with
           attribute name attrName, with value attrval and satisfying the
           condition op using linearSearch()
        */
        recId=BlockAccess::linearSearch(relId,attrName,attrVal,op);
    }
    else{
        // (index exists for the attribute)
        recId = BPlusTree::bPlusSearch(relId, attrName, attrVal, op);
        /* search for the record id (recid) correspoding to the attribute with
        attribute name attrName and with value attrval and satisfying the
        condition op using BPlusTree::bPlusSearch() */
    }


    // if there's no record satisfying the given condition (recId = {-1, -1})
    //     return E_NOTFOUND;
    if(recId.block==-1 && recId.slot==-1)return E_NOTFOUND;
    RecBuffer rec(recId.block);
    rec.getRecord(record,recId.slot);

    /* Copy the record with record id (recId) to the record buffer (record).
       For this, instantiate a RecBuffer class object by passing the recId and
       call the appropriate method to fetch the record
    */

    return SUCCESS;
}