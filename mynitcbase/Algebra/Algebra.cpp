#include "Algebra.h"
#include<iostream>
#include <cstring>
using namespace std;

bool isNumber(char *str) {
    int len;
    float ignore;
    int ret = sscanf(str, "%f %n", &ignore, &len);
    return ret == 1 && len == strlen(str);
}




int Algebra::insert(char relName[ATTR_SIZE], int nAttrs, char record[][ATTR_SIZE]){
    if (strcmp(relName, RELCAT_RELNAME) == 0 || strcmp(relName, ATTRCAT_RELNAME) == 0)
    {
        return E_NOTPERMITTED;
    }
    int relId = OpenRelTable::getRelId(relName);

    if (relId == E_RELNOTOPEN)
    {
        return E_RELNOTOPEN;
    }
    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(relId, &relCatEntry);
    if (relCatEntry.numAttrs != nAttrs)
    {
        return E_NATTRMISMATCH;
    }
    Attribute recordValues[nAttrs];
    for (int i = 0; i < nAttrs; i++)
    {
        AttrCatEntry attrCatEntry;
        AttrCacheTable::getAttrCatEntry(relId, i, &attrCatEntry);
        int type = attrCatEntry.attrType;
        if (type == NUMBER)
        {
            if (isNumber(record[i]))
            {
                recordValues[i].nVal = atof(record[i]);
                //cout<<record[i];
            }
            else
            {
                return E_ATTRTYPEMISMATCH;
            }
        }
        else if (type == STRING)
        {
            strcpy(recordValues[i].sVal, record[i]); 
            //cout<<record[i];
        }
    }
    return BlockAccess::insert(relId, recordValues);

}



int Algebra::select(char srcRel[ATTR_SIZE], char targetRel[ATTR_SIZE], char attr[ATTR_SIZE], int op, char strVal[ATTR_SIZE]) {
    
	int srcRelId = OpenRelTable::getRelId(srcRel);      
    if (srcRelId == E_RELNOTOPEN) 
    {
		return E_RELNOTOPEN;
	}
	AttrCatEntry attrCatEntry;
	int ret = AttrCacheTable::getAttrCatEntry(srcRelId, attr, &attrCatEntry);
	if (ret != SUCCESS)
    {
        return E_ATTRNOTEXIST;
    }

    int type = attrCatEntry.attrType;
	Attribute attrVal;

	if (type == NUMBER) 
    {
		if (isNumber(strVal)) 
        {
            attrVal.nVal = atof(strVal);
		} 
        else 
        {
			return E_ATTRTYPEMISMATCH;
		}
	} 
    else if (type == STRING) 
    {
		strcpy(attrVal.sVal, strVal);
	}
    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(srcRelId, &relCatEntry);
    int src_nAttrs = relCatEntry.numAttrs;

    char attr_names[src_nAttrs][ATTR_SIZE];
    int attr_types[src_nAttrs];
    for (int i = 0; i < src_nAttrs; i++) 
    {
        AttrCatEntry attrCatEntry;
        AttrCacheTable::getAttrCatEntry(srcRelId, i, &attrCatEntry);

        strcpy(attr_names[i], attrCatEntry.attrName);
        attr_types[i] = attrCatEntry.attrType;
    }

    ret = Schema::createRel(targetRel, src_nAttrs, attr_names, attr_types);
    if (ret != SUCCESS)
    {
        return ret;
    }
    int targetRelId = OpenRelTable::openRel(targetRel);
    if (targetRelId < 0){
        Schema::deleteRel(targetRel);
        return ret;
    }

    Attribute record[src_nAttrs];
    RelCacheTable::resetSearchIndex(srcRelId);
    RelCacheTable::resetSearchIndex(targetRelId);
    AttrCacheTable::resetSearchIndex(srcRelId, attr);
    StaticBuffer::count=0;
    ret = BlockAccess::search(srcRelId, record, attr, attrVal, op);
    while (ret==SUCCESS) 
    {
        ret = BlockAccess::insert(targetRelId, record);
        if (ret != SUCCESS) 
        {
            Schema::closeRel(targetRel);
            Schema::deleteRel(targetRel);
            return ret;
        }
        ret = BlockAccess::search(srcRelId, record, attr, attrVal, op);
    }
    Schema::closeRel(targetRel);
    //cout<<StaticBuffer::count;
    StaticBuffer::count=0;
    return SUCCESS;
}

int Algebra::project(char srcRel[ATTR_SIZE], char targetRel[ATTR_SIZE]) {
    int srcRelId =  OpenRelTable::getRelId(srcRel);
    if (srcRelId == E_RELNOTOPEN)
    {
        return E_RELNOTOPEN;
    }

    RelCatEntry srcRelCatEntry;
    RelCacheTable::getRelCatEntry(srcRelId, &srcRelCatEntry);
    int numAttrs = srcRelCatEntry.numAttrs;
    char attrNames[numAttrs][ATTR_SIZE];
    int attrTypes[numAttrs];
    
    for (int i = 0; i < numAttrs; i++)
    {
        AttrCatEntry attrCatEntry;
        AttrCacheTable::getAttrCatEntry(srcRelId, i, &attrCatEntry);
        strcpy(attrNames[i], attrCatEntry.attrName);
        attrTypes[i] = attrCatEntry.attrType;
    }

    int ret = Schema::createRel(targetRel, numAttrs, attrNames, attrTypes);
    if (ret != SUCCESS)
    {
        return ret;
    }
    int targetRelId = OpenRelTable::openRel(targetRel);
    if (targetRelId < 0)
    {
        Schema::deleteRel(targetRel);
        return targetRelId;
    }
    
    RelCacheTable::resetSearchIndex(srcRelId);

    Attribute record[numAttrs];

    ret = BlockAccess::project(srcRelId, record);
    while (ret== SUCCESS)
    {
        ret = BlockAccess::insert(targetRelId, record);
        if (ret != SUCCESS) {
            Schema::closeRel(targetRel);
            Schema::deleteRel(targetRel);
            return ret;
        }
        ret = BlockAccess::project(srcRelId, record);
    }

    Schema::closeRel(targetRel);
    return SUCCESS;
}



int Algebra::project(char srcRel[ATTR_SIZE], char targetRel[ATTR_SIZE], int tar_nAttrs, char tar_Attrs[][ATTR_SIZE]) {
    int srcRelId = OpenRelTable::getRelId(srcRel);
    if (srcRelId == E_RELNOTOPEN)
    {
        return E_RELNOTOPEN;
    }
    RelCatEntry srcRelCatEntry;
    RelCacheTable::getRelCatEntry(srcRelId, &srcRelCatEntry);
    int src_nAttrs = srcRelCatEntry.numAttrs;
    
    int attr_offset[tar_nAttrs];
    int attr_types[tar_nAttrs];
    for (int i = 0; i < tar_nAttrs; i++)
    {
        AttrCatEntry attrCatEntry;
        int ret = AttrCacheTable::getAttrCatEntry(srcRelId, tar_Attrs[i], &attrCatEntry);
        if (ret != SUCCESS)
        {
            return E_ATTRNOTEXIST;
        }

        attr_offset[i] = attrCatEntry.offset;
        attr_types[i] = attrCatEntry.attrType;
    }

    int ret = Schema::createRel(targetRel, tar_nAttrs, tar_Attrs, attr_types); 
    if (ret != SUCCESS)
    {
        return ret;
    }
    
    int targetRelId = OpenRelTable::openRel(targetRel);
    if(targetRelId < 0)
    {
        Schema::deleteRel(targetRel);
        return targetRelId;
    }

    RelCacheTable::resetSearchIndex(srcRelId);

    Attribute record[src_nAttrs];
    ret = BlockAccess::project(srcRelId, record);
    while (ret== SUCCESS) {
        Attribute proj_record[tar_nAttrs];
        for (int i = 0; i < tar_nAttrs; i++)
        {
            proj_record[i] = record[attr_offset[i]];
        }
        ret = BlockAccess::insert(targetRelId, proj_record);

        if (ret != SUCCESS) {
            Schema::closeRel(targetRel);
            ret = Schema::deleteRel(targetRel);
            return ret;
        }
        ret = BlockAccess::project(srcRelId, record);
    }

    ret = Schema::closeRel(targetRel);
    if (ret != SUCCESS)
    {
        printf("Invalid Relation ID.\n");
        exit(1);
    }

    return SUCCESS;
}


int Algebra::join(char srcRelation1[ATTR_SIZE], char srcRelation2[ATTR_SIZE], char targetRelation[ATTR_SIZE], char attribute1[ATTR_SIZE], char attribute2[ATTR_SIZE]) {

    // get the srcRelation1's rel-id using OpenRelTable::getRelId() method
    int src1RelId=OpenRelTable::getRelId(srcRelation1);
    int src2RelId=OpenRelTable::getRelId(srcRelation2);
    // get the srcRelation2's rel-id using OpenRelTable::getRelId() method

    // if either of the two source relations is not open
    //     return E_RELNOTOPEN
    if(src1RelId==E_RELNOTOPEN || src2RelId==E_RELNOTOPEN)
        return E_RELNOTOPEN;

    AttrCatEntry attrCatEntry1, attrCatEntry2;
    // get the attribute catalog entries for the following from the attribute cache
    // (using AttrCacheTable::getAttrCatEntry())
    // - attrCatEntry1 = attribute1 of srcRelation1
    // - attrCatEntry2 = attribute2 of srcRelation2
    int ret=AttrCacheTable::getAttrCatEntry(src1RelId,attribute1,&attrCatEntry1);
    if(ret==E_ATTRNOTEXIST){
        //cout<<attribute1;
        return ret;
    }
    ret=AttrCacheTable::getAttrCatEntry(src2RelId,attribute2,&attrCatEntry2);
    if(ret==E_ATTRNOTEXIST){
        //cout<<attribute2;
        return ret;
    }
    // if attribute1 is not present in srcRelation1 or attribute2 is not
    // present in srcRelation2 (getAttrCatEntry() returned E_ATTRNOTEXIST)
    //     return E_ATTRNOTEXIST.

    // if attribute1 and attribute2 are of different types return E_ATTRTYPEMISMATCH
    if(attrCatEntry1.attrType!=attrCatEntry2.attrType)
        return E_ATTRTYPEMISMATCH;
    // iterate through all the attributes in both the source relations and check if
    // there are any other pair of attributes other than join attributes
    // (i.e. attribute1 and attribute2) with duplicate names in srcRelation1 and
    // srcRelation2 (use AttrCacheTable::getAttrCatEntry())
    // If yes, return E_DUPLICATEATTR

    RelCatEntry relCatEntry1,relCatEntry2;
    RelCacheTable::getRelCatEntry(src1RelId,&relCatEntry1);
    RelCacheTable::getRelCatEntry(src2RelId,&relCatEntry2);
    AttrCatEntry t1,t2;
    for(int i=0;i<relCatEntry1.numAttrs;i++){
        if(i==attrCatEntry1.offset)continue;
        AttrCacheTable::getAttrCatEntry(src1RelId,i,&t1);
        for(int j=0;j<relCatEntry2.numAttrs;j++){
            AttrCacheTable::getAttrCatEntry(src2RelId,j,&t2);
            if(strcmp(t1.attrName,t2.attrName)==0)
                return E_DUPLICATEATTR;
        }
    }

    // get the relation catalog entries for the relations from the relation cache
    // (use RelCacheTable::getRelCatEntry() function)

    int numOfAttributes1 = relCatEntry1.numAttrs/* number of attributes in srcRelation1 */;
    int numOfAttributes2 = relCatEntry2.numAttrs/* number of attributes in srcRelation2 */;

    // if rel2 does not have an index on attr2
    //     create it using BPlusTree:bPlusCreate()
    //     if call fails, return the appropriate error code
    //     (if your implementation is correct, the only error code that will
    //      be returned here is E_DISKFULL)
    if(attrCatEntry2.rootBlock==-1){
        ret=BPlusTree::bPlusCreate(src2RelId,attribute2);
        if(ret!=SUCCESS)return ret;
    }

    int numOfAttributesInTarget = numOfAttributes1 + numOfAttributes2 - 1;
    // Note: The target relation has number of attributes one less than
    // nAttrs1+nAttrs2 (Why?)

    // declare the following arrays to store the details of the target relation
    char targetRelAttrNames[numOfAttributesInTarget][ATTR_SIZE];
    int targetRelAttrTypes[numOfAttributesInTarget];

    // iterate through all the attributes in both the source relations and
    // update targetRelAttrNames[],targetRelAttrTypes[] arrays excluding attribute2
    // in srcRelation2 (use AttrCacheTable::getAttrCatEntry())

    for(int i=0;i<relCatEntry1.numAttrs;i++){
        AttrCacheTable::getAttrCatEntry(src1RelId,i,&t1);
        strcpy(targetRelAttrNames[i],t1.attrName);
        targetRelAttrTypes[i]=t1.attrType;
    }
    for(int j=0;j<attrCatEntry2.offset;j++){
        AttrCacheTable::getAttrCatEntry(src2RelId,j,&t2);
        strcpy(targetRelAttrNames[numOfAttributes1+j],t2.attrName);
        targetRelAttrTypes[numOfAttributes1+j]=t2.attrType;
    }
    for(int j=attrCatEntry2.offset+1;j<relCatEntry2.numAttrs;j++){
        AttrCacheTable::getAttrCatEntry(src2RelId,j,&t2);
        strcpy(targetRelAttrNames[numOfAttributes1+j-1],t2.attrName);
        targetRelAttrTypes[numOfAttributes1+j-1]=t2.attrType;
    }

    // create the target relation using the Schema::createRel() function
    // if createRel() returns an error, return that error
    ret=Schema::createRel(targetRelation,numOfAttributesInTarget,targetRelAttrNames,targetRelAttrTypes);
    if(ret!=SUCCESS)return ret;
    // Open the targetRelation using OpenRelTable::openRel()

    int targetRelId=OpenRelTable::openRel(targetRelation);
    // if openRel() fails (No free entries left in the Open Relation Table)
    if(targetRelId==E_CACHEFULL){
        // delete target relation by calling Schema::deleteRel()
        // return the error code
        Schema::deleteRel(targetRelation);
        return ret;
    }

    Attribute record1[numOfAttributes1];
    Attribute record2[numOfAttributes2];
    Attribute targetRecord[numOfAttributesInTarget];
    RelCacheTable::resetSearchIndex(src1RelId);

    // this loop is to get every record of the srcRelation1 one by one
    while (BlockAccess::project(src1RelId, record1) == SUCCESS) {

        // reset the search index of `srcRelation2` in the relation cache
        // using RelCacheTable::resetSearchIndex()
        RelCacheTable::resetSearchIndex(src2RelId);
        AttrCacheTable::resetSearchIndex(src2RelId,attribute2);
        // reset the search index of `attribute2` in the attribute cache
        // using AttrCacheTable::resetSearchIndex()

        // this loop is to get every record of the srcRelation2 which satisfies
        //the following condition:
        // record1.attribute1 = record2.attribute2 (i.e. Equi-Join condition)
        while (BlockAccess::search(
            src2RelId, record2, attribute2, record1[attrCatEntry1.offset], EQ
        ) == SUCCESS ) {

            // copy srcRelation1's and srcRelation2's attribute values(except
            // for attribute2 in rel2) from record1 and record2 to targetRecord
            for(int i=0;i<numOfAttributes1;i++)
                targetRecord[i]=record1[i];
            for(int j=0;j<attrCatEntry2.offset;j++)
                targetRecord[numOfAttributes1+j]=record2[j];
            for(int j=attrCatEntry2.offset+1;j<numOfAttributes2;j++)
                targetRecord[numOfAttributes1+j-1]=record2[j];

            // insert the current record into the target relation by calling
            // BlockAccess::insert()
            ret=BlockAccess::insert(targetRelId,targetRecord);
            if(ret!=SUCCESS/* insert fails (insert should fail only due to DISK being FULL) */) {

                // close the target relation by calling OpenRelTable::closeRel()
                // delete targetRelation (by calling Schema::deleteRel())
                OpenRelTable::closeRel(targetRelId);
                Schema::deleteRel(targetRelation);
                return E_DISKFULL;
            }
        }
        //printf("y");
    }

    // close the target relation by calling OpenRelTable::closeRel()
    OpenRelTable::closeRel(targetRelId);
    return SUCCESS;
} 
