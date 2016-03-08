#define _CRT_SECURE_NO_WARNINGS
#include "DiskMultiMap.h"
#include "BinaryFile.h"
#include "MultiMapTuple.h"
#include <string>
#include <cstring>
#include <functional>

const int MAX_DATA_LENGTH = 120;

DiskMultiMap::DiskMultiMap()
{
	m_header.nBuckets = 0; // initialize to 0 buckets
	m_header.freeList = 0; // initialize to nullptr (empty list of free space)
}

DiskMultiMap::~DiskMultiMap()
{
	m_hashTable.close(); // close data file associated with DiskMultiMap
}

unsigned int DiskMultiMap::stringHashFunction(const std::string& hashMe)
{
	std::hash<std::string> str_hash; // creates a string hasher
	unsigned int hashValue = str_hash(hashMe); // hash the string
	Header h; // crate a header struct
	m_hashTable.read(h, 0); // read in header struct into h
	unsigned int nBuckets = h.nBuckets; // grab the number of buckets
	unsigned int bucketNumber = hashValue % nBuckets; // use modulo
	return bucketNumber + 8;
}

bool DiskMultiMap::createNew(const std::string & filename, unsigned int numBuckets)
{
	// creates and open hash table in a binary disk file with filename
	// and specified number of empty buckets
	close();								// close current data file to save data
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
	close(); // close current data file to save data
	return m_hashTable.openExisting(filename); // return success of method
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
	BinaryFile::Offset bucketAddress = stringHashFunction(key); // find bucket
	m_hashTable.read(newAssociation.next, bucketAddress); // point to top node on list
	m_hashTable.write(nodeAddress, bucketAddress);	// point to new association
	m_hashTable.write(newAssociation, nodeAddress); // write association in hash table
	return true;
}

DiskMultiMap::Iterator DiskMultiMap::search(const std::string & key)
{
	BinaryFile::Offset address = stringHashFunction(key); // get offset of bucket
	Association a;								// create a new association struct
	m_hashTable.read(a.next, address); // put address within bucket into a.next
	while (a.next != 0)
	{
		BinaryFile::Offset curr = a.next;		// address of current node
		m_hashTable.read(a, a.next);			// read an association into a
		if (strcmp(key.c_str(), a.key) == 0)	// if you find matching association
		{
			Iterator it(true, curr);			// iterator with address of curr node
			return it;							// return it
		}
	}
	Iterator it;								// else construct invalid iterator
	return it;									// return it
}

int DiskMultiMap::erase(const std::string& key, const std::string& value, const std::string& context)
{
	// TODO: CHECK THIS IMPLEMENTATION OVER AND OVER
	int nodesDeleted = 0; // initialize counter for # association removed
	BinaryFile::Offset curr = 0; // initialize to nullptr
	BinaryFile::Offset prev = 0; // initialize to nullptr
	BinaryFile::Offset bucketNumber = stringHashFunction(key); // get bucket #
	Association a; // create new association structure
	m_hashTable.read(a.next, bucketNumber); // put address of first node in a.next
	if (a.next != 0)						// if not nullptr
		curr = a.next;						// point curr to node too
	while (a.next != 0) // while not nullptr
	{
		m_hashTable.read(a, a.next); // read an association into a
		if (strcmp(a.key, key.c_str()) == 0 && strcmp(a.value, value.c_str()) == 0 && strcmp(a.context, context.c_str()) == 0)
		{
			BinaryFile::Offset killMe = curr;
			curr = a.next;						// move curr over to next node
			if (prev == 0)						// if pointing to top of the list
				m_hashTable.write(curr, bucketNumber); // point to new top of list
			else // prev is pointing to the last node
			{
				Association b; // create a new association struct
				m_hashTable.read(b, prev); // read association into b
				b.next = curr; // change next field to curr
			}
			// add removed node to the list of free memory
			Header m; // create a new header struct
			m_hashTable.read(m, 0); // read header into m
			Association dead; // create new association structure
			m_hashTable.read(dead, killMe); // read in association at killMe
			dead.next = m.freeList;
			m.freeList = killMe;
			m_hashTable.write(m, 0); // write header into file
			m_hashTable.write(dead, killMe); // write dead node back into file
			nodesDeleted++; // increment counter for nodes deleted
		}
		else
		{
			prev = curr; // assign prev offset to the curr offset
			curr = a.next; // move curr onto next node's address
		}
	}
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
	Association m; // create a new association struct
	//m_hashTable.read(m, m_address); // read current association into m
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
	//Association m; // create a new association struct
	//m_hashTable.read(m, m_address);
	// Note: There's a note about caching in the spec under this function
}