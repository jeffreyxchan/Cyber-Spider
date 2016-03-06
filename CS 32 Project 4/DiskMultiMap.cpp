#include "DiskMultiMap.h"
#include "BinaryFile.h"
#include "MultiMapTuple.h"

DiskMultiMap::DiskMultiMap()
{
	// must initialize object
}

DiskMultiMap::~DiskMultiMap()
{
	m_hashTable.close(); // close data file associated with DiskMultiMap
}

bool DiskMultiMap::createNew(const std::string & filename, unsigned int numBuckets)
{
	// creates and open hash table in a binary disk file with filename
	// and specified number of empty buckets
	m_hashTable.close(); // close current data file to save data
	bool success = true; // return this bool
	if (!m_hashTable.createNew(filename)) // create new data file
		success = false;
	if (!m_hashTable.write(numBuckets, 0)) // write numBuckets into first spot
		success = false;

	return success; // if method fails for wutever reason, return false
					// else must return true
}

bool DiskMultiMap::openExisting(const std::string & filename)
{
	m_hashTable.close(); // close current data file to save data
	bool success = true; // return this bool
	if (!openExisting(filename)) // open existing data file if possible
		success = false;
	return success; // if method fales for wutever reason, return flase
					// else it must return true
}

void DiskMultiMap::close()
{
	m_hashTable.close(); // closes the data file
}