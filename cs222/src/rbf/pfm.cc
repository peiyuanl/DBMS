#include "pfm.h"
#include <cstdio>
//#include <iostream>
using namespace std;

PagedFileManager* PagedFileManager::_pf_manager = 0;

PagedFileManager* PagedFileManager::instance()
{
    if(!_pf_manager)
        _pf_manager = new PagedFileManager();

    return _pf_manager;
}


PagedFileManager::PagedFileManager()
{
}


PagedFileManager::~PagedFileManager()
{
	_pf_manager=NULL;
}


RC PagedFileManager::createFile(const string &fileName)
{
	FILE *f;
	if((f=fopen(fileName.c_str(),"r"))==NULL){
		f=fopen(fileName.c_str(),"w");
		fclose(f);
		return 0;
	}
    return -1;
}


RC PagedFileManager::destroyFile(const string &fileName)
{
	FILE *f;
	if((f=fopen(fileName.c_str(),"r"))!=NULL)
    {
	    fclose(f);
	    remove(fileName.c_str());
	    return 0;
	}
	return -1;
}


RC PagedFileManager::openFile(const string &fileName, FileHandle &fileHandle)
{
	if(fileHandle.f!=NULL)
		return -1;
	if((fileHandle.f = fopen(fileName.c_str(), "r"))!=NULL){
		fclose(fileHandle.f);
		fileHandle.f = fopen(fileName.c_str(), "r+");
		return 0;
	}
	return -1;
}


RC PagedFileManager::closeFile(FileHandle &fileHandle)
{
    if(fileHandle.f != NULL){
		fclose(fileHandle.f);
		return 0;
    }
	return -1;
}


FileHandle::FileHandle()
{
	f=NULL;
	readPageCounter = 0;
	writePageCounter = 0;
	appendPageCounter = 0;
}


FileHandle::~FileHandle()
{
}


RC FileHandle::readPage(PageNum pageNum, void *data)
{
	if(f != NULL){
		fseek(f, PAGE_SIZE * pageNum, SEEK_SET);
		size_t readByte = fread(data, 1, PAGE_SIZE, f);
    	fseek(f, 0, SEEK_SET);
		if(readByte == PAGE_SIZE){
            this->readPageCounter += 1;
			return 0;
		}
	}
	return -1;
}


RC FileHandle::writePage(PageNum pageNum, const void *data)
{
   	fseek(f, PAGE_SIZE * pageNum, SEEK_SET);
   	size_t writeByte = fwrite (data, 1, PAGE_SIZE, f);
	fseek(f, 0, SEEK_SET);
   	if(writeByte == PAGE_SIZE) {
        this->writePageCounter += 1;
   		return 0;
   	}
   	return -1;
}


RC FileHandle::appendPage(const void *data)
{
	fseek(f, 0, SEEK_END);
	size_t writeByte = fwrite (data, 1, PAGE_SIZE, f);
	fseek(f, 0, SEEK_SET);
	if (writeByte == PAGE_SIZE) {
        this->appendPageCounter += 1;
		return 0;
	}
	return -1;
}


unsigned FileHandle::getNumberOfPages()
{
    if(f != NULL){
    	fseek(f, 0, SEEK_END);
    	long size = ftell(f);
    	fseek(f, 0, SEEK_SET);
    	unsigned result = (unsigned)(size / PAGE_SIZE);
    	return result;
    }
    return 0;
}


RC FileHandle::collectCounterValues(unsigned &readPageCount, unsigned &writePageCount, unsigned &appendPageCount)
{
    readPageCount = this->readPageCounter;
    writePageCount = this->writePageCounter;
    appendPageCount = this->appendPageCounter;
    return 0;
}
