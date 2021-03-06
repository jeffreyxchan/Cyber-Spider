#define _CRT_SECURE_NO_WARNINGS
#include "DiskMultiMap.h"
#include "BinaryFile.h"
#include "MultiMapTuple.h"
#include <string>
#include <cstring>
#include <functional>
using namespace std;

const int MAX_DATA_LENGTH = 120;

DiskMultiMap::DiskMultiMap()
{}

DiskMultiMap::~DiskMultiMap()
{
	close(); // close data file associated with DiskMultiMap
}

unsigned int DiskMultiMap::stringHashFunction(const std::string& hashMe)
{
	std::hash<std::string> str_hash; // creates a string hasher
	unsigned int hashValue = str_hash(hashMe); // hash the string
	Header h; // crate a header struct
	m_hashTable.read(h, 0); // read in header struct into h
	unsigned int nBuckets = h.nBuckets; // grab the number of buckets
	unsigned int bucketNumber = hashValue % nBuckets; // use modulo
	return 4*bucketNumber + 8; // return bucket number
}

bool DiskMultiMap::createNew(const std::string & filename, unsigned int numBuckets)
{
	close();								// close current data file to save data
	bool success = true;					// return this bool
	if (!m_hashTable.createNew(filename))	// create new data file
		success = false;
	Header m;								// create a new header struct
	m.nBuckets = numBuckets;				// assign number of buckets to header
	m.freeList = 0;							// initialize to head pointer
	if (!m_hashTable.write(m, 0))			// put header into the new file
		success = false;
	for (unsigned int k = 0; k < numBuckets; k++)	// add buckets into hash table
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
	BinaryFile::Offset bucketAddress = stringHashFunction(key); // get offset of bucket
	Association a;								// create a new association struct
	m_hashTable.read(a.next, bucketAddress); // put address within bucket into a.next
	while (a.next != 0)
	{
		BinaryFile::Offset curr = a.next;		// address of current node
		m_hashTable.read(a, a.next);			// read an association into a
		if (strcmp(key.c_str(), a.key) == 0)	// if you find matching association
		{
			Iterator it(true, curr, this);			// iterator with address of curr node
			return it;							// return it
		}
	}
	Iterator it;								// else construct invalid iterator
	return it;									// return it
}

int DiskMultiMap::erase(const std::string& key, const std::string& value, const std::string& context)
{
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
				m_hashTable.write(b, prev); // rewrite association into data file
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
	BinaryFile::Offset testingOffset;
	m_hashTable.read(testingOffset, 8);
	return nodesDeleted; // return counter
}

DiskMultiMap::Iterator::Iterator()
{
	m_valid = false; // default constructor must create iterator in invalid state
}

DiskMultiMap::Iterator::Iterator(bool valid, BinaryFile::Offset nodeAddress, DiskMultiMap* myDiskMultiMap)
{
	m_valid = valid;
	m_address = nodeAddress;
	m_ptr = myDiskMultiMap;
}

bool DiskMultiMap::Iterator::isValid() const
{
	return m_valid;
}

DiskMultiMap::Iterator& DiskMultiMap::Iterator::operator++()
{
	if (!m_valid)						// if iterator not valid, must return
		return *this;
	Association m;						// create a new association struct
	m_ptr->m_hashTable.read(m, m_address); // read current association into m
	string ultimateKey = m.key; // key that we must match
	while (m_valid)	// while iterator still valid
	{
		if (m.next != 0)
		{
			m_address = m.next; // move iterator over to such next association
			m_ptr->m_hashTable.read(m, m.next); // read in that association
		}
		else
			m_valid = false;
		if (m_valid && strcmp(m.key, ultimateKey.c_str()) == 0) // if you find matching key
			return *this;
	}
	return *this;						// return modified iterator
}

MultiMapTuple DiskMultiMap::Iterator::operator*()
{
	MultiMapTuple m;
	if (!m_valid) // if iterator not valid, return tuple with empty strings
		return m;
	Association hold; // create a new association struct
	m_ptr->m_hashTable.read(hold, m_address); // read curr association into m
	m.key = hold.key;			// put information into MultiMapTuple
	m.value = hold.value;
	m.context = hold.context;
	return m;
}