
# WikiLoader

A high-performance, multi-threaded Wikipedia XML dump parser that extracts pages and inter-page links, outputting them in CSV format for Neo4j graph databases.

[![C++](https://img.shields.io/badge/C%2B%2B-17%2B-blue.svg)](https://isocpp.org/)
[![CMake](https://img.shields.io/badge/CMake-3.21%2B-green.svg)](https://cmake.org/)
[![vcpkg](https://img.shields.io/badge/vcpkg-latest-orange.svg)](https://vcpkg.io/)
[![License](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

## Overview

WikiLoader processes 100GB+ Wikipedia XML dumps, converting them to two CSV files. These files can be processed and imported into Neo4j using the [neo4j-admin-import](https://neo4j.com/docs/operations-manual/current/tutorial/neo4j-admin-import/) tool. Although this project can be compiled and run on any platform, this project was built and tested using Linux. The instructions below contain some additional Linux commands that were used to process the csv files before importing them into Neo4j.

## Installation

### Prerequisites

**Install vcpkg**:
```bash
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh     # Linux/macOS
# ./bootstrap-vcpkg.bat  # Windows
```

**Install CMake**:
```bash
# Ubuntu/Debian
sudo apt update && sudo apt install cmake

# Or download from https://cmake.org/download/
```

### Build Instructions

```bash
# 1. Clone and enter directory
git clone https://github.com/ejagombar/WikiLoader.git && cd wikiloader

# 2. Create a build directory
mkdir build

# 3. Build with vcpkg (automatically installs dependencies)
cmake -DCMAKE_BUILD_TYPE=Release --preset=default

# 4. The executable will be in build/ directory
./build/WikiLoader file.xml
```

## Usage

### Download Wikipedia Dump

The Wikipedia XML dumps can be found [here](https://dumps.wikimedia.org/backup-index.html). Scroll to the bottom of the page to select a particular language. Ensure to use the `pages-articles-multisteam` varient. The English varient is roughly 23GB when compressed.

Linux instructions for downloading and decompressing the English Wikipedia dump.

```bash
# English Wikipedia (latest articles, ~20GB compressed)
wget https://dumps.wikimedia.org/enwiki/latest/enwiki-latest-pages-articles.xml.bz2

# Decompress file
bunzip2 enwiki-latest-pages-articles.xml.bz2
# -or-
lbzip2 enwiki-latest-pages-articles.xml.bz2
# lbzip2 is a faster, multithreaded alternative
```

### Importing the data into Neo4j
For some reason, there are lots of duplicate pages in the XML dump. This causes errors when importing it into neo4j using Neo4j-Admin-Import. To fix this the duplicate nodes must be removed. The commands below show how to filter the duplicates on Linux.

```bash
# Remove the csv header line 
sed -i '1d' ./nodes.csv

# Remove duplicate lines
sort nodes.csv | uniq > nodesSorted.csv

# Reinsert the csv header line
sed -i '1i pageName:ID,title,:LABEL' ./nodesSorted.csv
```
> [!WARNING] 
Either due to issues with the XML dump or the code, there are lots of links to pages that do not exist. When using the Neo4j-Adim-Import tool, set the the `--skip-bad-relationships` flag and `--bad-tolerance` flag to 100,000,000.

**For detailed instructions on how to import these files into Neo4j, follow the steps [here](https://neo4j.com/docs/operations-manual/2025.05/tutorial/neo4j-admin-import/). An example command has ben provided below.**

```
sudo docker run -v $HOME/graph_data/data:/data -v $HOME/graph_data/wiki:/var/lib/neo4j/import neo4j:latest neo4j-admin database import full --nodes=/var/lib/neo4j/import/nodesSorted.csv --relationships=/var/lib/neo4j/import/links.csv --overwrite-destination --verbose --skip-bad-relationships --bad-tolerance=100000000 --multiline-fields
```
Finished! 🎉 By following these steps you should have been able to convert a Wikipedia XML dump into a Neo4j database.

## Tool Output Information

The tool generates two CSV for graph database import which are explained below.

### `nodes.csv` - Page Information
```csv
pageName:ID,title,:LABEL
computer_accessibility,"Computer accessibility",PAGE
anarchism,"Anarchism",PAGE
history_of_afghanistan,"History of Afghanistan",REDIRECT
albedo,"Albedo",PAGE
```

**Columns:**
- `pageName:ID`: Normalized page identifier (lowercase, spaces→underscores)
- `title`: Original page title with proper capitalization
- `:LABEL`: Page type (`PAGE` for articles, `REDIRECT` for redirects)

### `links.csv` - Inter-page Links
```csv
:START_ID,:END_ID,:TYPE
anarchism,political_philosophy,LINK
anarchism,authority,LINK
computer_accessibility,assistive_technology,LINK
albedo,solar_radiation,LINK
```

**Columns:**
- `:START_ID`: Source page identifier
- `:END_ID`: Target page identifier  
- `:TYPE`: Link type (always `LINK`)

## Architecture

The application uses a producer-consumer pattern with thread-safe queues.

![g52](https://github.com/user-attachments/assets/46491544-9b49-490f-9b08-098ef8c5fb87)

**Key Components:**
- **XML Reader**: Streams XML and batches pages for processing
- **MySaxParser**: SAX-based parser for memory efficiency  
- **TSQueue**: Thread-safe queue with graceful shutdown support
- **Progress**: Dynamic progress tracking with ETA calculation
- **CSV Writer**: Concurrent output writing with buffering

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Support

For questions, bug reports, or feature requests, please [open an issue](https://github.com/ejagombar/WikiLoader/issues) on GitHub.

For performance questions or optimization help, include:
- System specifications (CPU, RAM, storage type)
- Input file size and Wikipedia language
- Current processing times and bottlenecks observed

