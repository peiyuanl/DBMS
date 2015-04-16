
#include "rbfm.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <iostream>
#include <queue>

using namespace std;

RecordBasedFileManager* RecordBasedFileManager::_rbf_manager = 0;

RecordBasedFileManager* RecordBasedFileManager::instance()
{
    if(!_rbf_manager)
        _rbf_manager = new RecordBasedFileManager();

    return _rbf_manager;
}

RecordBasedFileManager::RecordBasedFileManager()
{
	pfm=PagedFileManager::instance();
}

RecordBasedFileManager::~RecordBasedFileManager()
{
	pfm=NULL;
	_rbf_manager=NULL;
}

RC RecordBasedFileManager::createFile(const string &fileName) {
	return pfm->createFile(fileName);
}

RC RecordBasedFileManager::destroyFile(const string &fileName) {
	return pfm->destroyFile(fileName);
}

RC RecordBasedFileManager::openFile(const string &fileName, FileHandle &fileHandle) {
	return pfm->openFile(fileName,fileHandle);
}

RC RecordBasedFileManager::closeFile(FileHandle &fileHandle) {
	return pfm->closeFile(fileHandle);
}

//calculate the length of the data
int RecordBasedFileManager::dataLength(const vector<Attribute> &recordDescriptor, const void *data) {
    int len = 0;
    int var_len;
    double size = (double) recordDescriptor.size();
    int indicatorBytes = (int) ceil(size / 8);
    unsigned char indicator;
    queue<int> q; //put the positions of the null indicators into the queue for further operations
    for (int i = 0; i < indicatorBytes; i++){
    	memcpy(&indicator, (char *) data + i, 1);
    	for(int j = 8 * i; j < 8 * i + 8; j++){
    		if((indicator & 0x80) == 0x80){
    			q.push(j);
    		}
    		indicator <<= 1;
    	}
    	len++;
    }
    int nullField = -1;
    if(!q.empty()){
    	nullField = q.front();
    	q.pop();
    }
    for (int i = 0; i < (int) recordDescriptor.size(); i++) {
    	//check if it is a null field, if so, continue
    	if(i == nullField){
    	    if(!q.empty()){
    	    	nullField = q.front();
    	    	q.pop();
    	    }
    		continue;
    	}
        Attribute attr = recordDescriptor.at(i);
        switch (attr.type) {
			case (TypeVarChar): {
                memcpy(&var_len, (char *) data + len, 4);
                len += 4;
                len += var_len;
				break;
			}
			//TypeInt or TypeReal
			default: {
                len += 4;
				break;
			}
		}
    }
    return len;
}

//initialize a new page to store the data
void RecordBasedFileManager::NewPage(FileHandle &fileHandle) {
	int offset = 0;
	int slotNum = 0;
	void *data = malloc(PAGE_SIZE);
	//in each page (except the index page), the last eight bytes
	//are used to store the free space offset and the slot number of the page.
	//initialize the free space offset and the slot number
	memcpy((char *) data + PAGE_SIZE - sizeof(int), &offset, sizeof(int)); //size of int is four bytes
	memcpy((char *) data + PAGE_SIZE - 2 * sizeof(int), &slotNum, sizeof(int));
	fileHandle.appendPage(data);
	free(data);
}

//initialize a new index page and initialize a new page after it to store the data
//we have an index page in every 1024 pages, in which we use 2048 bytes to store the free space of the 1024 pages
int RecordBasedFileManager::NewIndexPage(FileHandle &fileHandle) {
	short page0_space = 0;
	short page1_space = PAGE_SIZE - 2 * sizeof(int);
	void *data = malloc(PAGE_SIZE);
	memcpy((char *) data, &page0_space, sizeof(short));
	memcpy((char *) data + sizeof(short), &page1_space, sizeof(short));
	fileHandle.appendPage(data);
	free(data);
	NewPage(fileHandle);
	return fileHandle.getNumberOfPages();
}

//find a page which has enough space to insert the data (also update the index page)
int RecordBasedFileManager::findPage(FileHandle &fileHandle, int length) {
	short freeSpace;
	int i;
	unsigned int j;
	void *data = malloc(PAGE_SIZE);
	unsigned int totalPageNum = fileHandle.getNumberOfPages();
	int numOfIndexPage = (totalPageNum - 1) / (PAGE_SIZE / sizeof(int)) + 1; // PAGE_SIZE / sizeof(int) is 1024
	for (i = 0; i < numOfIndexPage; i++) {
		fileHandle.readPage(i * (PAGE_SIZE / sizeof(int)), data);
		for (j = 1; j < (PAGE_SIZE / sizeof(int)); j++) {
			if (i * (PAGE_SIZE / sizeof(int)) + j < totalPageNum) {
				memcpy(&freeSpace, (char *) data + j * sizeof(short), sizeof(short));
				if (freeSpace >= length)
					freeSpace -= length;
				else
					//do not have enough space, continue to find the page
					continue;
			}
			else {
				//need to append a new page
				NewPage(fileHandle);
				freeSpace = PAGE_SIZE - length - 2 * sizeof(int);
			}
			//update the index page use the freeSpace after insert
			memcpy((char *) data + j * sizeof(short), &freeSpace, sizeof(short));
			fileHandle.writePage(i * (PAGE_SIZE / sizeof(int)), data);
			free(data);
			return i * ((PAGE_SIZE / sizeof(int))) + j;
		}
	}
	//need to append an index page
	NewIndexPage(fileHandle);
	int count = fileHandle.getNumberOfPages();
	fileHandle.readPage(count - 2, data);
	freeSpace = PAGE_SIZE - length - 2 * sizeof(int);
	memcpy((char *) data + sizeof(short), &freeSpace, sizeof(short));
	fileHandle.writePage(count - 2, data);
	free(data);
	return count - 1;
}

//update the page (append the data to the page with the specified pageNum)
int RecordBasedFileManager::updatePage(FileHandle &fileHandle, int length, const void *data, int pageNum) {
	int freeSpaceOffset;
	int slotNum;
	void *data_page = malloc(PAGE_SIZE);
	fileHandle.readPage(pageNum, data_page);
	//get the offset and slot number before appending the data
	memcpy(&freeSpaceOffset, (char *) data_page + PAGE_SIZE - sizeof(int), sizeof(int));
	memcpy(&slotNum, (char *) data_page + PAGE_SIZE - 2 * sizeof(int), sizeof(int));
	slotNum++; //the first slot has the slot number 1
	memcpy((char *) data_page + freeSpaceOffset, (char *) data, length);
	//eight bytes are used to store the offset and length of each record at the end of the free space of the page (stored one by one)
	memcpy((char *) data_page + PAGE_SIZE - (2 * slotNum + 1) * sizeof(int), &freeSpaceOffset, sizeof(int));
	memcpy((char *) data_page + PAGE_SIZE - (2 * slotNum + 2) * sizeof(int), &length, sizeof(int));
	freeSpaceOffset += length;
	//update the freeSpaceOffset and slotNum after appending the data
	memcpy((char *) data_page + PAGE_SIZE - sizeof(int), &freeSpaceOffset, sizeof(int));
	memcpy((char *) data_page + PAGE_SIZE - 2 * sizeof(int), &slotNum, sizeof(int));
	fileHandle.writePage(pageNum, data_page);
	free(data_page);
	return slotNum;
}

RC RecordBasedFileManager::insertRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const void *data, RID &rid) {
	int recordLength;
	recordLength = dataLength(recordDescriptor, data);
	if (recordLength <= 0) {
		return -1;
	}
	if (fileHandle.getNumberOfPages() == 0) {
		NewIndexPage(fileHandle);
	}
	int N = findPage(fileHandle, recordLength + 2 * sizeof(int));
	rid.pageNum = N;
	rid.slotNum = updatePage(fileHandle, recordLength, data, rid.pageNum);
	return 0;
}

RC RecordBasedFileManager::readRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const RID &rid, void *data) {
    int offset;
    int length;
    void *page_data = malloc(PAGE_SIZE);
    if(fileHandle.readPage(rid.pageNum, page_data) == 0)
    {
        memcpy(&offset, (char *) page_data + PAGE_SIZE - (2 * rid.slotNum + 1) * sizeof(int), sizeof(int));
        memcpy(&length, (char *) page_data + PAGE_SIZE - (2 * rid.slotNum + 2) * sizeof(int), sizeof(int));
        if(length > 0)
        {
            memcpy((char *) data, (char *) page_data + offset, length);
            free(page_data);
            return 0;
        }
    }
	free(page_data);
	return -1;
}

RC RecordBasedFileManager::printRecord(const vector<Attribute> &recordDescriptor, const void *data) {
	if (data == NULL)
		return -1;
	int length = 0;
	int offset = 0;
	double size= (double) recordDescriptor.size();
	int indicatorBytes = ceil(size / 8);
	unsigned char indicator;
	queue<int> q;
	for (int i = 0; i < indicatorBytes; i++){
		memcpy(&indicator, (char *)data + i, 1);
		for(int j = 8 * i; j < 8 * i + 8; j++){
			if((indicator & 0x80)== 0x80){
				q.push(j);
			}
			indicator <<= 1;
		}
		offset++;
	}
	int nullField = -1;
	if(!q.empty()){
		nullField = q.front();
		q.pop();
	}
	for (int i = 0; i < (int)recordDescriptor.size(); i++) {
		Attribute attr = recordDescriptor.at(i);
		cout << attr.name <<": ";
		if(i==nullField){
			if(!q.empty()){
				nullField = q.front();
				q.pop();
			}
			cout << "NULL ； ";
			continue;
		}
		switch (attr.type) {
			case (TypeInt): {
				int number;
				memcpy(&number, (char *) data + offset, 4);
				offset += 4;
				cout << number << " ； ";
				break;
			}
			case (TypeReal): {
				float number;
				memcpy(&number, (char *) data + offset, 4);
				offset += 4;
				cout << number << " ； ";
				break;
			}
			case (TypeVarChar): {
				memcpy(&length, (char *) data + offset, 4);
				offset += 4;
				char * str = new char[length + 1];
				str[length] = '\0';
				memcpy(str, (char *) data + offset, length);
				offset += length;
				cout << str << " ； ";
				break;
			}
		}
	}
	cout << endl;
	return 0;
}
