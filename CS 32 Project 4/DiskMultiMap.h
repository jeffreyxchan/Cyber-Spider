#ifndef DISKMULTIMAP_H_
#define DISKMULTIMAP_H_

#include <string>
#include "MultiMapTuple.h"

class DiskMultiMap
{
public:
	class Iterator
	{
	public:
		Iterator();
		// You may add additional constructors
		Iterator(bool valid, BinaryFile::Offset nodeAddress);
		bool isValid() const;
		Iterator& operator++();
		MultiMapTuple operator*();
	private:
		// can add more private stuff there
		bool m_valid;
		BinaryFile::Offset m_address;
	};

	DiskMultiMap();
	~DiskMultiMap();
	bool createNew(const std::string& filename, unsigned int numBuckets);
	bool openExisting(const std::string& filename);
	void close();
	bool insert(const std::string& key, const std::string& value, const std::string& context);
	Iterator search(const std::string& key);
	int erase(const std::string& key, const std::string& value, const std::string& context);

private:
	struct Header
	{
		// header struct total of 8 bytes long
		int nBuckets;
		BinaryFile::Offset freeList;
	};
	struct Association
	{
		char key[121];
		char value[121];
		char context[121];
		BinaryFile::Offset next;
	};

	BinaryFile::Offset acquireNode();
	unsigned int stringHashFunction(const std::string& hashMe);

	BinaryFile m_hashTable;
	Header m_header;
};

#endif // DISKMULTIMAP_H_