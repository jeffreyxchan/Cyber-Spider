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
{
	m_header.nBuckets = 0; // initialize to 0 buckets
	m_header.freeList = 0; // initialize to nullptr (empty list of free space)
}

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
	// DEBUGGING COUT STATEMENT FOR TEST PURPOSES
	std::cout << "I have computed bucket number " << 4 * bucketNumber + 8 <<
		" for the key " << hashMe << std::endl;
	return 4*bucketNumber + 8;
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
	// DEBUGGING COUT STATEMENT FOR TEST PURPOSES
	Header testing; // CREATED A TESTING HEADER STRUCT
	m_hashTable.read(testing, 0); // READ HEADER INTO TESTING HEADER STRUCT
	std::cout << "nBuckets: " << testing.nBuckets << " freeList: " << testing.freeList << endl;
	for (int k = 0; k < numBuckets; k++)	// add buckets into hash table
	{
		BinaryFile::Offset bucketOffset = m_hashTable.fileLength();
		BinaryFile::Offset initialOffset = 0; // initialize to nullptr
		if (!m_hashTable.write(initialOffset, bucketOffset))
			success = false;
		else
			std::cout << "Wrote offset " << initialOffset << " into address " << bucketOffset << endl;
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
	// DEBUGGING COUT STATEMENT FOR TEST PURPOSES
	std::cout << "The Node address I acquired is " << nodeAddress << endl;
	Association newAssociation;	// create new node and input information
	strcpy(newAssociation.key, key.c_str());
	strcpy(newAssociation.value, value.c_str());
	strcpy(newAssociation.context, context.c_str());
	// DEBUGGING COUT STATEMENT FOR TEST PURPOSES
	std::cout << "The node I'm inserting looks like:" << endl;
	std::cout << "Key: " << newAssociation.key << " Value: " << newAssociation.value <<
		" Context: " << newAssociation.context << endl;
	BinaryFile::Offset bucketAddress = stringHashFunction(key); // find bucket
	m_hashTable.read(newAssociation.next, bucketAddress); // point to top node on list
	m_hashTable.write(nodeAddress, bucketAddress);	// point to new association
	m_hashTable.write(newAssociation, nodeAddress); // write association in hash table
	// DEBUGGING COUT STATEMENT FOR TEST PURPOSES
	BinaryFile::Offset testing;
	m_hashTable.read(testing, bucketAddress);
	std::cout << "Reading the file, the bucket at address " << bucketAddress <<
		" contains the offset " << testing << endl;
	return true;
}

DiskMultiMap::Iterator DiskMultiMap::search(const std::string & key)
{
	BinaryFile::Offset bucketAddress = stringHashFunction(key); // get offset of bucket
	std::cout << "Search Function: I will start at bucket # " << bucketAddress << endl;
	Association a;								// create a new association struct
	m_hashTable.read(a.next, bucketAddress); // put address within bucket into a.next
	std::cout << "a.next is now: " << a.next << endl;
	while (a.next != 0)
	{
		BinaryFile::Offset curr = a.next;		// address of current node
		std::cout << "curr is now: " << curr << endl;
		m_hashTable.read(a, a.next);			// read an association into a
		std::cout << "Read an association into a" << endl;
		std::cout << "The association looks like:" << endl;
		std::cout << "Key: " << a.key << " Value: " << a.value << " Context: " << a.context << endl;
		if (strcmp(key.c_str(), a.key) == 0)	// if you find matching association
		{
			std::cout << "Found a matching key!" << endl;
			std::cout << "Constructing an iterator with address " << curr << endl;
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
	// DEBUGGING COUT STATEMENT FOR TEST PURPOSES
	std::cout << "This association is connected to bucket # " << bucketNumber << endl;
	Association a; // create new association structure
	m_hashTable.read(a.next, bucketNumber); // put address of first node in a.next
	// DEBUGGING COUT STATEMENT FOR TEST PURPOSES
	std::cout << "a.next starts off as " << a.next << " , which should be the address of the first node."
		<< endl;
	if (a.next != 0)						// if not nullptr
		curr = a.next;						// point curr to node too
	// DEBUGGING COUT STATEMENT FOR TEST PURPOSES
	std::cout << "As a result, curr starts off as " << curr << endl;
	while (a.next != 0) // while not nullptr
	{
		std::cout << "Address of node I'm looking at is " << a.next << endl;
		m_hashTable.read(a, a.next); // read an association into a
		if (strcmp(a.key, key.c_str()) == 0 && strcmp(a.value, value.c_str()) == 0 && strcmp(a.context, context.c_str()) == 0)
		{
			std::cout << "Found matching association. Marking to kill now!" << endl;
			std::cout << "This association looks like:" << endl;
			std::cout << "Key: " << a.key << " Value: " << a.value << " Context: " <<
				a.context << endl;
			BinaryFile::Offset killMe = curr;
			// DEBUGGING COUT STATEMENT FOR TEST PURPOSES
			std::cout << "killMe = " << killMe << endl;
			curr = a.next;						// move curr over to next node
			std::cout << "curr is now: " << curr << endl;
			std::cout << "and prev is: " << prev << endl;
			if (prev == 0)						// if pointing to top of the list
				m_hashTable.write(curr, bucketNumber); // point to new top of list
			else // prev is pointing to the last node
			{
				std::cout << "Because prev is not zero and it's " << prev << " instead, I'm in here." << endl;
				Association b; // create a new association struct
				m_hashTable.read(b, prev); // read association into b
				std::cout << "The association b at prev looks like:" << endl;
				std::cout << "Key: " << b.key << " Value: " << b.value <<
					" Context: " << b.context << endl;
				b.next = curr; // change next field to curr
				std::cout << "b.next is now " << b.next << endl;
				m_hashTable.write(b, prev); // rewrite association into data file
				std::cout << "After changing b.next, I wrote node b back into hasnTable" << endl;
			}
			// add removed node to the list of free memory
			Header m; // create a new header struct
			m_hashTable.read(m, 0); // read header into m
			Association dead; // create new association structure
			m_hashTable.read(dead, killMe); // read in association at killMe
			std::cout << "Created association dead from the node at address killMe = " << killMe << endl;
			std::cout << "The Association dead looks like:" << endl;
			std::cout << "Key: " << dead.key << " Value: " << dead.value <<
				" Context: " << dead.context << endl;
			dead.next = m.freeList;
			// DEBUGGING COUT STATEMENT FOR TEST PURPOSES
			std::cout << "dead.next is " << dead.next << endl;
			m.freeList = killMe;
			// DEBUGGING COUT STATEMENT FOR TEST PURPOSES
			std::cout << "freeList is now " << m.freeList << endl;
			m_hashTable.write(m, 0); // write header into file
			m_hashTable.write(dead, killMe); // write dead node back into file
			nodesDeleted++; // increment counter for nodes deleted
			std::cout << "nodesDeleted is now: " << nodesDeleted << endl;
		}
		else
		{
			prev = curr; // assign prev offset to the curr offset
			curr = a.next; // move curr onto next node's address
			// DEBUGGING COUT STATEMENT FOR TEST PURPOSES
			std::cout << "Didn't fine matching node, so moving on in list. curr is now " << curr << endl;
		}
	}
	std::cout << "Since a.next = " << a.next << ". Finally hit a nullptr. The number of deleted nodes is: " << nodesDeleted << endl;
	BinaryFile::Offset testingOffset;
	m_hashTable.read(testingOffset, 8);
	std::cout << "Offset within bucket # 8 is now " << testingOffset << endl;
	return nodesDeleted; // return counter
}

DiskMultiMap::Iterator::Iterator()
{
	m_valid = false; // default constructor must create iterator in invalid state
	std::cout << "Default Iterator Constructor:" << endl;
	std::cout << "You have created a default iterator where m_valid = " << m_valid << endl;
}

DiskMultiMap::Iterator::Iterator(bool valid, BinaryFile::Offset nodeAddress, DiskMultiMap* myDiskMultiMap)
{
	m_valid = valid;
	m_address = nodeAddress;
	m_ptr = myDiskMultiMap;
	// COUT DEBUGGING STATEMENTS TO BE REMOVED
	std::cout << "Custom Iterator Constructor:" << endl;
	std::cout << "You have create a custom iterator that looks like:" << endl;
	std::cout << "m_valid: " << m_valid << " m_address: " << m_address << endl;
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
	std::cout << "operator++: Read an association into m." << endl;
	std::cout << "The association looks like:" << endl;
	std::cout << "Key: " << m.key << " Value: " << m.value << " Context: " << m.context << endl;
	if (m.next != 0)					// if there's a next association
		m_address = m.next;				// set address to that association
	else
		m_valid = false;				// else, set to invalid iterator
	std::cout << "The address of this iterator is now " << m_address << endl;
	return *this;						// return modified iterator
}

MultiMapTuple DiskMultiMap::Iterator::operator*()
{
	MultiMapTuple m;
	if (!m_valid) // if iterator not valid, return tuple with empty strings
		return m;
	Association hold; // create a new association struct
	m_ptr->m_hashTable.read(hold, m_address); // read curr association into m
	std::cout << "operator*: Read in association into hold." << endl;
	std::cout << "The association looks like:" << endl;
	std::cout << "Key: " << hold.key << " Value: " << hold.value << " Context: " << hold.context << endl;
	m.key = hold.key;			// put information into MultiMapTuple
	m.value = hold.value;
	m.context = hold.context;
	std::cout << "Information put into MMT is:" << endl;
	std::cout << "Key: " << m.key << " Value: " << m.value << " Context: " << m.context << endl;
	return m;
}