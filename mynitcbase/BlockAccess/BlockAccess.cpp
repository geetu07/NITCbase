#include "BlockAccess.h"

#include <cstring>
#include <iostream>
#include "BlockAccess.h"
#include <cstring>
#include <stdlib.h>
#include <cstdio>

RecId BlockAccess::linearSearch(int relId, char attrName[ATTR_SIZE], union Attribute attrVal, int op) {
    // get the previous search index of the relation relId from the relation cache
    // (use RelCacheTable::getSearchIndex() function)
    RecId prevRecId{-1,-1};
    int ret= RelCacheTable::getSearchIndex(relId,&prevRecId);
    if(ret!=SUCCESS){
        return prevRecId;
    }

    // let block and slot denote the record id of the record being currently checked
    int block=-1,slot=-1;
    RelCatEntry relCatBuf;
    RelCacheTable::getRelCatEntry(relId, &relCatBuf);
    // if the current search index record is invalid(i.e. both block and slot = -1)
    if (prevRecId.block == -1 && prevRecId.slot == -1)
    {
        // (no hits from previous search; search should start from the
        // first record itself)

        // get the first record block of the relation from the relation cache
        // (use RelCacheTable::getRelCatEntry() function of Cache Layer)
        block=relCatBuf.firstBlk;
        slot=0;
        // block = first record block of the relation
        // slot = 0
    }
    else
    {
        // (there is a hit from previous search; search should start from
        // the record next to the search index record)
        block=prevRecId.block;
        slot=prevRecId.slot+1;
        // block = search index's block
        // slot = search index's slot + 1
    }

    /* The following code searches for the next record in the relation
       that satisfies the given condition
       We start from the record id (block, slot) and iterate over the remaining
       records of the relation
    */
    while (block != -1)
    {
        /* create a RecBuffer object for block (use RecBuffer Constructor for
           existing block) */
        HeadInfo header;
        Attribute CatRecord[relCatBuf.numAttrs];
        RecBuffer buffer(block);
        // get the record with id (block, slot) using RecBuffer::getRecord()
        buffer.getRecord(CatRecord,slot);
        // get header of the block using RecBuffer::getHeader() function
        buffer.getHeader(&header);
        // get slot map of the block using RecBuffer::getSlotMap() function
        unsigned char *slotMap =(unsigned char *)malloc(sizeof(unsigned char) * header.numSlots);
        buffer.getSlotMap(slotMap);

        // If slot >= the number of slots per block(i.e. no more slots in this block)
        if(slot>=header.numSlots){
            // update block = right block of block
            // update slot = 0
            block=header.rblock;
            slot=0;
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
        AttrCatEntry attrCatBuf;
        ret = AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatBuf);
        // if(ret!=SUCCESS){
        //     return {-1,-1};
        // }
        Attribute currRecordAttr = CatRecord[attrCatBuf.offset];
        /* use the attribute offset to get the value of the attribute from
           current record */

        int cmpVal=compareAttrs(currRecordAttr,attrVal,attrCatBuf.attrType);  // will store the difference between the attributes
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
           RecId newIndex;
           newIndex.block=block;
           newIndex.slot=slot;
           RelCacheTable::setSearchIndex(relId,&newIndex);

            return RecId{block, slot};
        }

        slot++;
    }

    // no record in the relation with Id relid satisfies the given condition
    return RecId{-1, -1};
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
int BlockAccess::search(int relId, Attribute *record, char attrName[ATTR_SIZE], Attribute attrVal, int op) {
    RelCacheTable::resetSearchIndex(relId);
    RecId recId=linearSearch(relId,attrName,attrVal,op);
    if(recId.block==-1 && recId.slot==-1)
        return E_NOTFOUND;
    RecBuffer blck(recId.block);
    blck.getRecord(record,recId.slot);
    return SUCCESS;
}
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

    return SUCCESS;
}