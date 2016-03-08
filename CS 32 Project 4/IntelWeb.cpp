#include "DiskMultiMap.h"
#include "BinaryFile.h"
#include "IntelWeb.h"
#include <string>

IntelWeb::IntelWeb()
{
	// TODO: initialize any private data members if needed
}

IntelWeb::~IntelWeb()
{
	close();
}

bool IntelWeb::createNew(const std::string& filePrefix, unsigned int maxDataItems)
{
	close(); // close any existing open files first
	
	bool success = true; // return this
	// create file names for each hashtable of data
	string name1 = filePrefix + "download.dat";
	string name2 = filePrefix + "create.dat";
	string name3 = filePrefix + "contact.dat";
	// TODO: create number of buckets that result in load factor <= 0.75
	m_downloadHashTable.createNew(name1, maxDataItems); // all 3 prob wrong
	m_createHashTable.createNew(name2, maxDataItems);
	m_contactHashTable.createNew(name3, maxDataItems);

	// if anything fails, must close all data files and return false
	if (!success)
		close();
	return success; // return the return bool
}

bool IntelWeb::openExisting(const std::string & filePrefix)
{
	// close any current files
	close();

	bool success = true; // initialize return bool
	string name1 = filePrefix + "download.dat";
	string name2 = filePrefix + "create.dat";
	string name3 = filePrefix + "contact.dat";
	if (!m_downloadHashTable.openExisting(name1))
		success = false;
	if (!m_createHashTable.openExisting(name2))
		success = false;
	if (!m_contactHashTable.openExisting(name3))
		success = false;
	if (!success)
		close();
	return success;
}

void IntelWeb::close()
{
	m_downloadHashTable.close();
	m_createHashTable.close();
	m_contactHashTable.close();
}

bool IntelWeb::ingest(const std::string & telemetryFile)
{
	return false;
}

unsigned int IntelWeb::crawl(const std::vector<std::string>& indicators, unsigned int minPrevalenceToBeGood, std::vector<std::string>& badEntitiesFound, std::vector<InteractionTuple>& interactions)
{
	return 0;
}

bool IntelWeb::purge(const std::string & entity)
{
	return false;
}
