#include "DiskMultiMap.h"
#include "BinaryFile.h"
#include "MultiMapTuple.h"
#include <string>
#include <cstring>
#include <functional>

const int MAX_DATA_LENGTH = 120;

unsigned int DiskMultiMap::stringHashFunction(const std::string& hashMe)
{
	std::hash<std::string> str_hash; // creates a string hasher
	unsigned int hashValue = str_hash(hashMe); // hash the string
	Header h; // crate a header struct
	m_hashTable.read(h, 0); // read in header struct into h
	unsigned int nBuckets = h.nBuckets; // grab the number of buckets
	unsigned int bucketNumber = hashValue % nBuckets; // use modulo
	return bucketNumber;
}

DiskMultiMap::DiskMultiMap()
{
	m_header.nBuckets = 0; // initialize to 0 buckets
	m_header.freeList = 0; // initialize to nullptr (empty list of free space)
}

DiskMultiMap::~DiskMultiMap()
{
	m_hashTable.close(); // close data file associated with DiskMultiMap
}

bool DiskMultiMap::createNew(const std::string & filename, unsigned int numBuckets)
{
	// creates and open hash table in a binary disk file with filename
	// and specified number of empty buckets
	m_hashTable.close();					// close current data file to save data
	bool success = true;					// return this bool
	if (!m_hashTable.createNew(filename))	// create new data file
		success = false;
	Header m;								// create a new header struct
	m.nBuckets = numBuckets;				// assign number of buckets to header
	m.freeList = 0;							// initialize to head pointer
	if (!m_hashTable.write(m, 0))			// put header into the new file
		success = false;
	for (int k = 0; k < numBuckets; k++)	// add buckets into hash table
	{
		BinaryFile::Offset bucketOffset = m_hashTable.fileLength();
		BinaryFile::Offset initialOffset = 0; // initialize to nullptr
		if (!m_hashTable.write(initialOffset, bucketOffset))
			success = false;
	}
	return success; // return success of method
}

bool DiskMultiMap::openExisting(const std::string & filename)
{
	m_hashTable.close(); // close current data file to save data
	return openExisting(filename); // return success of method
}

void DiskMultiMap::close()
{
	m_hashTable.close(); // closes the data file
}

BinaryFile::Offset DiskMultiMap::acquireNode()
{
	Header header;
	m_hashTable.read(header, 0);		// read header into new header struct
	if (header.freeList == 0)			// if nullptr
	{
		int fileSize = m_hashTable.fileLength(); // get length of file
		BinaryFile::Offset newNodeAddress = fileSize; // format into offset
		return newNodeAddress; // return that as address of new node
	}
	BinaryFile::Offset newNodeAddress = header.freeList; // get address from freeList
	Association emptySpace;
	m_hashTable.read(emptySpace, header.freeList); // go into the empty Node space
	header.freeList = emptySpace.next; // place next node as top of list
	m_hashTable.write(header, 0); // rewrite header
	return newNodeAddress; // return address of the available space
}

bool DiskMultiMap::insert(const std::string& key, const std::string& value,
	const std::string& context)
{
	// checking if strings are too long
	if (key.size() > MAX_DATA_LENGTH || value.size() > MAX_DATA_LENGTH || context.size() > MAX_DATA_LENGTH)
		return false;

	BinaryFile::Offset nodeAddress = acquireNode(); // get address of new association
	Association newAssociation;	// create new node and input information
	strcpy(newAssociation.key, key.c_str());
	strcpy(newAssociation.value, value.c_str());
	strcpy(newAssociation.context, context.c_str());
	BinaryFile::Offset bucketAddress = stringHashFunction(key) + 8; // find bucket
	newAssociation.next = bucketAddress;			// point to top bucket on list
	m_hashTable.write(nodeAddress, bucketAddress);	// point to new association
	m_hashTable.write(newAssociation, nodeAddress); // write association in hash table
	return true;
}

DiskMultiMap::Iterator DiskMultiMap::search(const std::string & key)
{
	BinaryFile::Offset address = stringHashFunction(key) + 8; // get offset of bucket
	Association a;								// create a new association struct
	m_hashTable.read(a.next, address); // put address within bucket into a.next
	while (a.next != 0)
	{
		m_hashTable.read(a, a.next);			// read top association into a
		if (strcmp(key.c_str(), a.key) == 0)	// if you find matching association
		{
			Iterator it(true, address);			// construct new iterator
			return it;							// return it
		}
	}
	Iterator it;								// else construct invalid iterator
	return it;									// return it
}

int DiskMultiMap::erase(const std::string& key, const std::string& value, const std::string& context)
{
	// TODO: FINISH THIS IMPLEMENTATION
	int nodesDeleted = 0; // initialize counter for # association removed
	BinaryFile::Offset curr = 0; // initialize to nullptr
	BinaryFile::Offset prev = 0; // initialize to nullptr
	BinaryFile::Offset bucketNumber = stringHashFunction(key) + 8; // get bucket #
	Association a; // create new association structure
	m_hashTable.read(a.next, bucketNumber); // put address of first node in a.next
	if (a.next != 0) // if not nullptr
		curr = a.next;
	while (a.next != 0) // while not nullptr
	{
		m_hashTable.read(a, a.next); // read an association into a
		if (strcmp(a.key, key.c_str()) == 0 && strcmp(a.value, value.c_str()) == 0 && strcmp(a.context, context.c_str()) == 0)
		{
			BinaryFile::Offset killMe = curr;
			curr = a.next;
			if (prev == 0) // if pointing to top of the list
				m_hashTable.write(curr, bucketNumber); // point to new top of list
			else // prev is pointing to the last node
			{

			}
		}
		else
		{
			prev = curr; // assign prev offset to the curr offset
			curr = a.next; // move curr onto next node's address
		}
	}
	// if you found matching association,
	//		unhook node from the list
	//		add node to the list of free space
	//		increment counter
	return nodesDeleted; // return counter
}

DiskMultiMap::Iterator::Iterator()
{
	m_valid = false; // default constructor must create iterator in invalid state
}

DiskMultiMap::Iterator::Iterator(bool valid, BinaryFile::Offset nodeAddress)
{
	m_valid = valid;
	m_address = nodeAddress;
}

bool DiskMultiMap::Iterator::isValid() const
{
	return m_valid;
}

DiskMultiMap::Iterator& DiskMultiMap::Iterator::operator++()
{
	// TODO: FINISH IMPLEMENTING PREFIX INCREMENT OPERATOR
	if (!m_valid) // if iterator not valid, must return
		return *this;
	// if there's a next association
	//		change this iterator's offset to that next association
	//		return this iterator
	// if there isn't a next association
	//		change m_valid field to false
	//		return this iterator
}

MultiMapTuple DiskMultiMap::Iterator::operator*()
{
	// TODO: FINISH IMPLEMENTING THIS FUNCTION
	MultiMapTuple m;
	if (!m_valid) // if iterator not valid, return tuple with empty strings
		return m;
	// Note: There's a note about caching in the spec under this function
}