#include "DiskMultiMap.h"
#include "BinaryFile.h"
#include "IntelWeb.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <stack>
#include <algorithm>
using namespace std;

bool operator<(const InteractionTuple& tuple1, const InteractionTuple& tuple2)
{
	if (tuple1.context < tuple2.context)
		return true;
	else if (tuple1.context > tuple2.context)
		return false;
	else if (tuple1.from < tuple2.from)
		return true;
	else if (tuple1.from > tuple2.from)
		return false;
	else if (tuple1.to < tuple2.to)
		return true;
	else if (tuple1.to > tuple2.to)
		return false;
	else
		return false;
}

IntelWeb::IntelWeb()
{}

IntelWeb::~IntelWeb()
{
	close();
}

bool IntelWeb::createNew(const std::string& filePrefix, unsigned int maxDataItems)
{
	close(); // close any existing open files first

	bool success = true; // return this
	// create file names for each hashtable of data
	string name1 = filePrefix + "interaction.dat";
	string name2 = filePrefix + "reverseinteraction.dat";

	// generate # of buckets based on load # maxDataItems and ROUND UP at least 1
	int goodNumberOfBuckets = maxDataItems / 0.75 + 5;
	// create new DMM's in all 2 BinaryFiles with specified names and # of buckets
	if (!m_interactionTable.createNew(name1, goodNumberOfBuckets))
		success = false;
	if (!m_reverseInteractionTable.createNew(name2, goodNumberOfBuckets))
		success = false;

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
	string name1 = filePrefix + "interaction.dat";
	string name2 = filePrefix + "reverseinteraction.dat";
	if (!m_interactionTable.openExisting(name1))
		success = false;
	if (!m_reverseInteractionTable.openExisting(name2))
		success = false;

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
		return false;

	string line; // string to hold in read lines
	while (getline(inf, line)) // while you can read in a line from the file
	{
		istringstream iss(line); // make a string stream out of the line
		string context;				// string to hold context of each line
		string from;				// string to hold "from" from each line
		string to;					// string to hold "to" field from each line
		if (!(iss >> context >> from >> to)) // each line has to fit this template
			continue;

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
	stack<string> toBeProcessed;		// waiting list for entities to analyze
	unordered_set<string> uniqueEntities; // set of unique bad entities
	set<InteractionTuple> interactionTupleSet; // set for unique interactions
	unordered_set<string> processedStrings; // set of stuff you've already looked at
	for (vector<string>::const_iterator it = indicators.begin(); it != indicators.end(); it++)
		toBeProcessed.push(*it); // push all the indicators onto the queue
	while (!toBeProcessed.empty()) // while there's still stuff to look at
	{
		string curr = toBeProcessed.top(); // get the top item from the stack
		toBeProcessed.pop();		// pop the item off of the stack
		if (processedStrings.find(curr) == processedStrings.end())
			continue;
		processedStrings.insert(curr); // insert curr into set of looked at strings
		DiskMultiMap::Iterator inOrder = m_interactionTable.search(curr);
		DiskMultiMap::Iterator reverseOrder = m_reverseInteractionTable.search(curr);
		int prevalenceOfCurr = 0;		// initialize a prevalence counter
		// calculate prevalence
		while (inOrder.isValid()) // while the inOrder iterator is still valid
		{
			prevalenceOfCurr++;
			++inOrder;
		}
		while (reverseOrder.isValid())
		{
			prevalenceOfCurr++;
			++inOrder;
		}
		if (prevalenceOfCurr > 0 && prevalenceOfCurr < minPrevalenceToBeGood) // if within prevalence range
		{
			uniqueEntities.insert(curr); // insert the entity into the set of entities
										 // push all their keys onto the stack
			DiskMultiMap::Iterator addInOrder = m_interactionTable.search(curr);
			DiskMultiMap::Iterator addReverse = m_reverseInteractionTable.search(curr);
			while (addInOrder.isValid())
			{
				MultiMapTuple newMultiMapTuple = *addInOrder;
				InteractionTuple newInteraction;
				newInteraction.context = newMultiMapTuple.context;
				newInteraction.from = newMultiMapTuple.key;
				newInteraction.to = newMultiMapTuple.value;
				toBeProcessed.push(newInteraction.to);
				interactionTupleSet.insert(newInteraction);
				++addInOrder;
			}
			while (addReverse.isValid())
			{
				MultiMapTuple newMultiMapTuple = *addReverse;
				InteractionTuple newInteraction;
				newInteraction.context = newMultiMapTuple.context;
				newInteraction.from = newMultiMapTuple.key;
				newInteraction.to = newMultiMapTuple.value;
				toBeProcessed.push(newInteraction.to);
				interactionTupleSet.insert(newInteraction);
				++addReverse;
			}
		}
		else
			continue;
	}
	// put all items in badEntities set into a vector, sort, and swap that with the passed in vector
	vector<string> finalEntitiesVector;
	for (unordered_set<string>::iterator setIterator = uniqueEntities.begin(); setIterator != uniqueEntities.end(); setIterator++)
		finalEntitiesVector.push_back(*setIterator);
	std::sort(finalEntitiesVector.begin(), finalEntitiesVector.end()); // sort vector
	badEntitiesFound = finalEntitiesVector;
	// put all the iteractions into a vector and swap that with the passed in vector
	vector<InteractionTuple> finalTupleVector;
	for (set<InteractionTuple>::iterator setIterator = interactionTupleSet.begin(); setIterator != interactionTupleSet.end(); setIterator++)
		finalTupleVector.push_back(*setIterator);
	interactions = finalTupleVector;
	return badEntitiesFound.size();
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
