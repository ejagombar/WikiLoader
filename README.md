# WikiMapper
Parse Wikipedia XML data dumps to create an interactive graph

Currently, I am focusing on developing a method to quickly parse the ~100GB Wikipedia XML dump file.

## Aims
- [x] Parse the XML file, extracting page names and links
- [x] Analyse performance using GProf
- [x] Parallelise the parser.
- [x] Import data into Neo4j using Neo4j-Admin-Import
- [ ] Setup a third party visual Neo4j graph database explorer
- [ ] Develop a custom graph storage library
- [ ] Develop a custom viewer to visualise the data.

---

# Progress Log
This log will be used to track what I am doing, different methods that I try and used to save links and other resources that I use. 
## Obtaining data from Wikipedia
Wikipedia provides lots of different data formats to download data. However the site is very confusing to use. I eventually found a way to downlaod just a current snapshot of all english pages which takes up around 16GB

1) Downloaded the XML dump files from https://dumps.wikimedia.org/enwiki/20240401/ (pages articles multistream)
2) Verified files against checksum to ensure nothing is corrupt 
3) Uncompress the bz2 file which took around half an hour. It expaneded to a size of 100GB
3) Need to find a way to extract information from the giant xml file.

## Selecting a suitable XML parser

- I initally selected [PUGIXML](https://pugixml.org/) as the XML parser of choice due to it being very small, simple, and well documented. However, after doing further research, I found that using a DOM based parser would not be suitable due to the size of the files. and instead a SAX based parser is required.
- Instead, I will use libxml2 which is a GNOME project. [Libxml++](https://libxmlplusplus.github.io/libxmlplusplus/) is a C++ wrapper for libxml2 and this will be used as I am going to develop this project in C++. 

## Setting up a Package Manager and Build System
- After attempting to build Libxml++ and implement it into my project with CMake and getting nowhere, I decided to try to use a package manager.
- The Libxml++ documentation website returns a 404 error however the documentation can be built from the release tar.
- A secondary version was found [here](https://libxmlplusplus.github.io/libxmlplusplus/)

### Conan
- I initially selected Conan as the package manager for this project and followed [this tutorial](https://docs.conan.io/2/tutorial/consuming_packages/build_simple_cmake_project.html) to set it up. The package page for libxmlpp on Conan can be found [here](https://conan.io/center/recipes/libxmlpp?version=5.2.0) and the source code can be found [here](https://github.com/libxmlplusplus/libxmlplusplus?tab=readme-ov-file).
- I am unable to get libxml++ to link to the project. However, it does not seem to be an issue of Conan as that is able to find and download the package properly.

### Vcpkg
- An alternative method is to use Vcpkg instead of Conan so I removed Conan from the project and installed Vcpkg
- [This](https://learn.microsoft.com/en-gb/vcpkg/get_started/get-started?pivots=shell-bash) tutorial was used to set up [Vcpkg](https://vcpkg.io/en/package/libxmlpp)
- I am still getting issues. [Could not find package configuration file](https://stackoverflow.com/questions/65045150/how-to-fix-could-not-find-a-package-configuration-file-error-in-cmake) is one of them and it may be an issue with the Libxmlpp library?

### PkgConfig and CMake
- It turns out that Libxml++ does not have a CMake configuration by design, as it is instead handled by PkgConfig which requires a special configuration in the CMakeLists.txt file, as I discovered [here](https://stackoverflow.com/questions/63749077/c-cmake-cant-find-libxml)
- [This](https://stackoverflow.com/questions/29191855/what-is-the-proper-way-to-use-pkg-config-from-cmake) guide shows how to properly configure CMake with PkgConfig. Now that this is done, Libxml++ successfully links and builds in the project. An example from the libxml++ Github was tested to confirm this.
- Next, the documentation needs to be built, and the Clang language server needs to be configured so that it can see the libraries included by CMake as currently it shows errors in my editor and no autocomplete.

## Setting up ClangD Language Server
In order for ClangD to recognise the libraries included with CMake, a compile_commands.json file must be generated. This is done by adding a line into the CMake configuration which generates it in the build folder. It can then by symlinked into the root directoy for it to be detected by ClangD.

## Actually building it

### SAX Vs TextParser
Libxml2 supports both SAX and a TextParser parser. Both of these options are suitable for large files as they do not use a tree structure that requires large amounts of memory. After playing around with both options, I decided to use the SAX parser as this is most comonly used. However, I would later like to implement a TextParser to comapre the performance.

### LibXML++ SAX Format
LibXML++ presents the SAX as a class which can be overloaded to add functionality to the parser. It provides functions that are called on various events that occur when the text is parsed, such as on_start_element() and on_character(). Using these functions, the title and contents of the web pages can be extracted.

### REGEX and Faster Alternatives
Once the contents of the webpages can be extracted, the links in these pages need to be collated. This could be done with a REGEX library such as the C++ standard one. However, this is slow, which is a big problem due to the raw amount of data that needs to be processed. After researching alternatives, I decided to use RE2 which is a much faster alternative.

### Neo4j and Docker
For the first tests, I have decided to use Neo4j, a graph database to store the wikipedia data. A good article can be found [here](https://medium.com/@matthewghannoum/simple-graph-database-setup-with-neo4j-and-docker-compose-061253593b5a) which shows how to install Neo4j using docker.
After doing this, I decided to follow some neo4j [academy tutorials](https://graphacademy.neo4j.com/) to learn some Cypher (the language used for neo4j) before I did anything further.
I followed the [bitesize tutorials](https://github.com/cj2001/bite_sized_data_science?tab=readme-ov-file) to learn how to use [neo4j-admin-import](https://neo4j.com/docs/operations-manual/current/tutorial/neo4j-admin-import/), the tool that is used to import massive CSV files into Neo4j.
Although I prevously set up a container using a docker-compose.yaml file, I will use the commands below to do it all directly for now as it is simpler and I do not need the additional functinality for these tests.

Run the neo4j-admin command, linking the folders at these locations to use as volumes for the container
```
sudo docker run -v $HOME/graph_data/data:/data -v $HOME/graph_data/gameofthrones:/var/lib/neo4j/import neo4j:latest neo4j-admin database import full --nodes=/var/lib/neo4j/import/got-s1-nodes.csv --relationships=/var/lib/neo4j/import/got-s1-edges.csv
```
Run the container, port forwarding the db and mounting the data
```
sudo docker run -p7474:7474 -p7687:7687 -v $HOME/graph_data/data:/data -v $HOME/graph_data/gameofthrones:/var/lib/neo4j/import --env NEO4J_AUTH=neo4j/test1234 neo4j:latest
```
This is the command I used. THere are many issues as my csv file currently has lots of duplicates. It also has broken links but this is expected as I am not importing all items
```
sudo docker run -v $HOME/graph_data/data:/data -v $HOME/graph_data/gameofthrones:/var/lib/neo4j/import neo4j:latest neo4j-admin database import full --nodes=import/nodes.csv --relationships=import/links.csv --delimiter="U+007C" --overwrite-destination --verbose --skip-duplicate-nodes=true --bad-tolerance=999999999 --skip-bad-relationships
```
This works and I am able to view the graph and look at the relationship between nodes. However, since this viewer is browser based, the performance is limited. Only up to 300 nodes can be viewed at one time using the online Neo4j viewer.

There are also some issues in the parser. Some page titles are being cut off, for example, there is a page called "é the Giant" which should be called "André the Giant". After some debugging it was discovered, that on_character function can return before the full string has been returned. To remedy this, at the start of each new page, a title varaible is created and each on_character function that contains "Title" data appends the result to this variable. This ensures that any title strings that are split, are recombined.

### GProf Analysis

- In order to profile the code with Gprof, the flags below must be added to the CMAKE file.

```
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")
SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -pg")
```

- After this, the program must be executed and left to finish running.

- Then, the GProf can be called, passing in the executable file.

- Here are some of the results:

```
Flat profile:

Each sample counts as 0.01 seconds.
  %   cumulative   self              self     total           
 time   seconds   seconds    calls  ns/call  ns/call  name    
  4.69      0.15     0.15                             re2::PODArray<int>::size() const
  4.37      0.28     0.14                             std::unique_ptr<int [], re2::PODArray<int>::Deleter>::get() const
  3.07      0.38     0.10                             std::unique_ptr<int [], re2::PODArray<int>::Deleter>::operator[](unsigned long) const
  2.59      0.46     0.08                             re2::SparseSetT<void>::contains(int) const
  2.59      0.54     0.08                             std::_Head_base<0ul, int*, false>::_M_head(std::_Head_base<0ul, int*, false> const&)
  2.59      0.62     0.08                             std::_Tuple_impl<0ul, int*, re2::PODArray<int>::Deleter>::_M_head(std::_Tuple_impl<0ul, int*, re2::PODArray<int>::Deleter> const&)
  2.43      0.69     0.08                             re2::SparseSetT<void>::max_size() const
  2.43      0.77     0.08                             std::tuple_element<0ul, std::tuple<int*, re2::PODArray<int>::Deleter> >::type const& std::get<0ul, int*, re2::PODArray<int>::Deleter>(std::tuple<int*, re2::PODArray<int>::Deleter> const&)
  2.27      0.84     0.07                             std::__uniq_ptr_impl<int, re2::PODArray<int>::Deleter>::_M_ptr() const
  2.27      0.91     0.07                             std::_Head_base<1ul, re2::PODArray<int>::Deleter, false>::_M_head(std::_Head_base<1ul, re2::PODArray<int>::Deleter, false> const&)
  1.94      0.97     0.06                             re2::DFA::AddToQueue(re2::DFA::Workq*, int, unsigned int)
  1.94      1.03     0.06                             std::_Tuple_impl<1ul, re2::PODArray<int>::Deleter>::_M_head(std::_Tuple_impl<1ul, re2::PODArray<int>::Deleter> const&)
  1.78      1.08     0.06                             re2::PODArray<int>::operator[](int) const
  1.62      1.13     0.05                             re2::Prog::Inst::opcode()
```

- Most of the time is spent on the RE2 function calls. This makes sense as the RE2 has to process a lot of text for each page. This is hard to increase efficiency so the next steps will be to include multithreading so that multiple pages can be processed at once.


### Benchmarking on Server
nohup ~/WikiMapper/build/WikiMapper ~/enwiki-20240401-pages-articles-multistream.xml &
disown
exit

### (Sidequest) Writing data directly to Neo4j using HTTP
I am struggling to get the neo4j-admin tool to work at the moment so I thought I would test using the HTTP interface so I could simply write data to the database directly. I am expecting this method to be extremely slow but it would be nice to get some things to work so I can see some of the data in Neo4j.
I used the curl library to perform http requests. This worked, and I was able to insert data however it took over two hours to insert around 160000 records, much less than the 6 million so this method is not really viable.

### Parallelising the Parser
I did consider attempting to have each thread read from the file, process the data, and append it to the output file. However, one thread reading from the file would block other threads from doing so, reducing the performance.
A better way to acheive multithreading is outlined below

- One thread will be used to split the data into chunks and add them to a queue.
- Other threads will then take data from this queue and process it and return it to a processed buffer.
- A last thread will take the processed data and write it to the output files.

This has now been fully implemented. Currently, the first thread splits the file into sections of 400 pages which are then added to the queue. There is also a maximum length that the queue can be. These are currently both arbitary numbers.

### Importing the data into Neo4j
For some reason, there are lots of duplicate pages in the XML dump. This causes errors when importing it into neo4j using Neo4j-Admin-Import. To fix this the duplicate nodes must be removed.

- Remove the csv header line `sed -i '1d' ./nodes.csv`
- Remove duplicate lines `sort nodes.csv | uniq > nodesSorted.csv`
- Reinsert the csv header line `sed -i '1i pageName:ID' ./nodesSorted.csv`

Unfortunatley there are still some issues with the linking of pages. Wikipedia has a number of rules that are used to link pages with similar names. For example, [[Suez_Canal]] will still link to the page titled Suez Canal. This also works for other characters such as `&` `&amp;` and `and` all being treated the same. A few find and replace rules have been implemented to fix this, however, it has not been verified that all rules have been met.

On top of this, there are lots of links to pages that do not exist. This issue is impossible to fix when importing the data into the database using Neo4j-Adim-Import so instead, the `--skip-bad-relationships` flag has been set and `--bad-tolerance` set to 100,000,000

The command below was used to insert the data into the Neo4j database. 
```
sudo docker run -v $HOME/graph_data/data:/data -v $HOME/graph_data/wiki:/var/lib/neo4j/import neo4j:latest neo4j-admin database import full --nodes=/var/lib/neo4j/import/nodesSorted.csv --relationships=/var/lib/neo4j/import/links.csv --overwrite-destination --verbose --skip-bad-relationships --bad-tolerance=100000000 --multiline-fields
```

This resultsed in an output as shown below, with 16,625,742 nodes and 302,342,403 relationships

```
IMPORT DONE in 2m 50s 583ms.
Imported:
  16625742 nodes
  302342403 relationships
  16625742 properties
```

# Benchmarks and Testing

### Test 1: 36 hrs 49 mins
- Conducted on my Proxmox Server, inside an LCX container
- To see the code used, checkout at the git tag `benchmarkTest1`

```
---------Info---------
Page Number: 23603262
Progress: 100.000%
Time Left: 0 hrs 0 mins 0 secs
Time Taken: 36 hrs 49 mins 15 secs
Finished processing document.
Page Count: 23603280
```

### Test 2: 10 hrs 48 mins
- Conducted on my Proxmox Server, inside an LCX container again
- To see the code used, checkout at the git tag `queuelimits`
- On top of making this code parallelised, some debugging flags that were mistakenly left in the Cmake file were removed.
- The Proxmox machine that this was run on only has 4 cores so a greater will be seen on a more modern CPU.

```
---------Info---------
Page Number: 23603262
Progress: 100.000%
Time Left: 0 hrs 0 mins 0 secs
Time Taken: 10 hrs 48 mins 48 secs
Finished processing document.
Page Count: 23603280
```
