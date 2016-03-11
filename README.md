# Cyber-Spider
## Description
This was my last CS 32 project of Winter 2016. The idea of this project revolves around creating an <b>attack detection system</b>
that would be able to crawl through millions or billions of computer log data and detect malicious entities. The program
also has the ability to discover new malicious entities that are associated with an already known set of 
malicious entities. The programmer would then have the option to purge
all associations within the log data that contain known malicious entities, emulating a sort of virus or 
attack defense program. This project tested our knowledge of binary search trees, hash tables, the C++ STL, and the time complexity of certain algorithms.

## Design
Since the program is designed to store a huge amount of telemetry log data, the project utilizes a disk based open hash table that satifies a maximum load of 0.75 to minimize the number of collisions. The open hash table is designed to hold nodes, hashed into buckets by their key values. Each node contains a key field, a value field, and a context field, which represents the three pieces of valid telemetry data. The implementation of the disk based open hash table is also capable of re=using space within the disk file that was occupied by removed nodes.
<br><br>
Crawling through huge amounts of data would also be super inefficient, which is why the implementations of certain functions within the project had to be under a certain time complexity. We chose to use a hash table for this project because accessing elements within the hash table have a time complexity of O(1), which makes accessing elements very efficient and ideal for our program.
<br><br>
Lastly, if an entity such as <i>google.com</i> was discovered as a malicious entity, the program is designed to not recognize it as a malicious entity based on its prevalence/popularity within the log data.

## Directory
The main source code contained in this project can be found within the CS 32 Project 4 folder.
<br><br>
<b>BinaryFile.h</b> was a file provided for us. It allowed us to create and write into disk files so that we could make our multi map.
<br><br>
<b>DiskMultiMap.h and DiskMultiMap.cpp</b> contain the design and implementaion of of the disk based open hash table capable of storing telemetry log data. It has the ability to insert new nodes, erase nodes, reuse disk space, and comes with an iterator to help traverse the hash table.
<br><br>
<b>IntelWeb.h and IntelWeb.cpp</b> contain the code for a class that can be used to process large amounts of telemetry data. Utilizing two DiskMultiMaps, the Intel Web class can "ingest" (insert) new telemetry log data from external text files, "crawl" through the stored telemetry log data, and "purge" recognized and newly discovered malicious entities from the log data.
<br><br>
<b>MultiMapTuple.h and InteractionTuple.h</b> contains the code for two simple structs that helped make the implementation easier.
<br><br>
Don't worry about the <b>main.cpp</b> file or any other <b>.dat</b> or <b>.txt</b> files that might also be contained within the same folder. The presence of thos files were purely for testing purposes.
