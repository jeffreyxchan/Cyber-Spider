#include <iostream>
#include <string>
#include "DiskMultiMap.h"
#include "IntelWeb.h"
using namespace std;

int main()
{
	IntelWeb iw;
	iw.createNew("ingestTestOne", 8);
	iw.ingest("sampleData.txt");
}