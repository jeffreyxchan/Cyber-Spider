#include "DiskMultiMap.h"
#include "BinaryFile.h"
#include "IntelWeb.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <queue>
using namespace std;

IntelWeb::IntelWeb()
{
	cout << "Successfully created IntelWeb object." << endl;
}

IntelWeb::~IntelWeb()
{
	close();
}

bool IntelWeb::createNew(const std::string& filePrefix, unsigned int maxDataItems)
{
	close(); // close any existing open files first
	cout << "Started by closing any open files." << endl;
	
	bool success = true; // return this
	// create file names for each hashtable of data
	string name1 = filePrefix + "interaction.dat";
	string name2 = filePrefix + "reverseinteraction.dat";
	cout << "While creating new, these are the file names I generated:" << endl;
	cout << "name1: " << name1 << " name2: " << name2 << endl;
	// generate # of buckets based on load # maxDataItems and ROUND UP at least 1
	int goodNumberOfBuckets = maxDataItems / 0.75 + 5;
	cout << "The goodNumberOfBuckets I generated is: " << goodNumberOfBuckets << endl;
	// create new DMM's in all 2 BinaryFiles with specified names and # of buckets
	if (!m_interactionTable.createNew(name1, goodNumberOfBuckets))
		success = false;
	if (!m_reverseInteractionTable.createNew(name2, goodNumberOfBuckets))
		success = false;
	if (success)
		cout << "Successfully created all new files!" << endl;
	// if anything fails, must close all data files and return false
	if (!success)
		close();
	return success; // return the return bool
}

bool IntelWeb::openExisting(const std::string & filePrefix)
{
	// close any current files
	close();
	cout << "openExisting: Starting by closing all files" << endl;

	bool success = true; // initialize return bool
	string name1 = filePrefix + "interaction.dat";
	string name2 = filePrefix + "reverseinteraction.dat";
	cout << "Gonna try to open: " << name1 << " & " << name2 << endl;
	if (!m_interactionTable.openExisting(name1))
		success = false;
	if (!m_reverseInteractionTable.openExisting(name2))
		success = false;
	if (success)
		cout << "Opening existing files was a success!" << endl;
	if (!success)
		close();
	return success;
}

void IntelWeb::close()
{
	m_interactionTable.close();
	m_reverseInteractionTable.close();
}

bool IntelWeb::ingest(const std::string & telemetryFile)
{
	ifstream inf(telemetryFile); // open the file that contains telemetry data
	if (!inf) // if not successful, relay message
	{
		cout << "Problem with opening such telemetryFile!" << endl;
		return false;
	}
	else
		cout << "Successfully opened: " << telemetryFile << endl;
	string line; // string to hold in read lines
	while (getline(inf, line)) // while you can read in a line from the file
	{
		istringstream iss(line); // make a string stream out of the line
		string context;				// string to hold context of each line
		string from;				// string to hold "from" from each line
		string to;					// string to hold "to" field from each line
		if (!(iss >> context >> from >> to)) // each line has to fit this template
		{
			cout << "Ignoring badly-formatted line of input: " << line << endl;
			continue;
		}
		cout << "The line I read looks like:" << endl;
		cout << "Context: " << context << " From: " << from << " To: " << to << endl;
		// push lines of data into member tables
		m_interactionTable.insert(from, to, context);
		m_reverseInteractionTable.insert(to, from, context);
	}
	return true;
}

unsigned int IntelWeb::crawl(const std::vector<std::string>& indicators, 
	unsigned int minPrevalenceToBeGood, std::vector<std::string>& badEntitiesFound, 
	std::vector<InteractionTuple>& interactions)
{
	unsigned int numberOfDiscoveredMaliciousEntities = 0; // initialize a counter
	queue<string> toBeProcessed; // initialize aa waiting list of entities to analyze
	unordered_set<string> uniqueEntities; // intialize a set of unique entities
	unordered_map<string, int> prevalenceMap; // intialize a prevalence map
	//unordered_set<InteractionTuple> uniqueInterations;
	for (vector<string>::const_iterator it = indicators.begin(); it != indicators.end(); it++)
	{
		toBeProcessed.push(*it); // push an entity onto the queue
		string originalString = *it; // save the first string just in case
		while (!toBeProcessed.empty())			// while the queue isnt empty
		{
			string findMe = toBeProcessed.front(); // extract front item
			toBeProcessed.pop();				// pop the item off
			DiskMultiMap::Iterator inOrder = m_interactionTable.search(findMe);
			while (inOrder.isValid()) // while iterator still valid
			{
				MultiMapTuple mmt = *inOrder; // get MultiMapTuple
				toBeProcessed.push(mmt.value); // push value onto queue
				uniqueEntities.insert(mmt.key); // push key onto unique set
				uniqueEntities.insert(mmt.value); // push value onto unique set
				unordered_map<string, int>::const_iterator found = prevalenceMap.find(mmt.key);
				if (found == prevalenceMap.end()) // if not found
					prevalenceMap.insert({ mmt.key, 1 }); // create a mapping
				else
					prevalenceMap[mmt.key]++; // increment the maping number
				found = prevalenceMap.find(mmt.value);
				if (found == prevalenceMap.end())
					prevalenceMap.insert({ mmt.value, 1 }); // create a mapping
				else
					prevalenceMap[mmt.value]++; // increment the mapping number
				//InteractionTuple tuple;
				//tuple.context = mmt.context;
				//tuple.from = mmt.key;
				//tuple.to = mmt.value;
				//uniqueInterations.insert(tuple); // insert tuple
				++inOrder; // increment iterator
			}
			DiskMultiMap::Iterator reverseOrder = m_reverseInteractionTable.search(findMe);
			while (reverseOrder.isValid())
			{
				MultiMapTuple mmt = *reverseOrder; // get MultiMapTuple
				toBeProcessed.push(mmt.value); // push value onto queue
				uniqueEntities.insert(mmt.key); // push key onto unique set
				uniqueEntities.insert(mmt.value); // push value onto unique set
				unordered_map<string, int>::const_iterator found = prevalenceMap.find(mmt.key);
				if (found == prevalenceMap.end()) // if not found
					prevalenceMap.insert({ mmt.key, 1 }); // create a mapping
				else
					prevalenceMap[mmt.key]++; // increment the maping number
				found = prevalenceMap.find(mmt.value);
				if (found == prevalenceMap.end())
					prevalenceMap.insert({ mmt.value, 1 }); // create a mapping
				else
					prevalenceMap[mmt.value]++; // increment the mapping number
				//InteractionTuple tuple;
				//tuple.context = mmt.context;
				//tuple.to = mmt.key;
				//tuple.from = mmt.value;
				//uniqueInterations.insert(tuple); // insert tuple
				++reverseOrder;
			}
		}
	}
	return numberOfDiscoveredMaliciousEntities;
}

bool IntelWeb::purge(const std::string & entity)
{
	int numberOfDataDeleted = 0;
	DiskMultiMap::Iterator it = m_interactionTable.search(entity);
	DiskMultiMap::Iterator it2 = m_reverseInteractionTable.search(entity);
	while (it.isValid()) // while the first iterator is still valid
	{
		MultiMapTuple m = *it; // read in the mmt
		++it; // increment the iterator before changing anything
		m_interactionTable.erase(m.key, m.value, m.context); // erase association
		// deals with corresponding association in other table
		m_reverseInteractionTable.erase(m.value, m.key, m.context);
		numberOfDataDeleted++; // increment # of data deleted
	}
	while (it2.isValid())
	{
		MultiMapTuple m = *it2; // read in the mmt
		++it2;	// increment the iterator before changing anything
		// erase association
		m_reverseInteractionTable.erase(m.key, m.value, m.context);
		// deals with the corresponding association in other table
		m_interactionTable.erase(m.value, m.key, m.context);
		numberOfDataDeleted++; // increment # of data deleted
	}
	return numberOfDataDeleted > 0; // return if any data was removed
}
