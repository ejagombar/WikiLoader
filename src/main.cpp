#include "TSQueue.h"
#include "progress.h"
#include "saxparser.h"

#include <cstdlib>
#include <iostream>
#include <libxml++/libxml++.h>
#include <libxml++/parsers/textreader.h>
#include <stdexcept>
#include <thread>

void pageProcessor(TSQueue<std::string> &qIn, TSQueue<std::vector<Page>> &qOut, std::atomic<bool> &keepAlive) {
    MySaxParser parser;

    while (keepAlive.load() || !qIn.empty()) {
        parser.parse_memory(qIn.pop());
        qOut.push(parser.getPages());
        parser.clear();
    }
}

void csvWriter(TSQueue<std::vector<Page>> &qIn, std::ofstream &nodeFile, std::ofstream &linkFile,
               std::atomic<bool> &keepAlive) {
    std::string linkStr;
    while (keepAlive.load() || !qIn.empty()) {
        if (qIn.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
            continue;
        }

        for (Page page : qIn.pop()) {
            if (page.title.size() == 0) {
                continue;
            }

            linkStr.clear();
            for (std::string x : page.links) {
                linkStr = linkStr + "\"" + page.title + "\",\"" + x + "\",LINK\n";
            }

            linkFile << linkStr << std::flush;
            nodeFile << "\"" << page.title << "\"" << std::endl;
        }
    }
}

void xmlReader(TSQueue<std::string> &qIn, TSQueue<std::vector<Page>> &qOut, std::string &filepath) {
    const int maxInputQueueSize = 5;
    const int pagesPerQueueItem = 400;
    const int wikipediaPages = 23603280;
    const int readThreadSleepTimeMs = 25;

    Progress progress(wikipediaPages);
    int pageCount(0);
    std::string output("<mediawiki>");
    xmlpp::TextReader reader(filepath);

    while (reader.read()) {
        if (reader.get_name() == "page") {
            output += reader.read_outer_xml();

            pageCount++;
            progress.increment();

            if (pageCount >= pagesPerQueueItem) {
                output += "\n</mediawiki>";
                qIn.push(output);
                output = "<mediawiki>\n";
                pageCount = 0;
            }
        }

        while (qIn.size() > maxInputQueueSize) {
            std::this_thread::sleep_for(std::chrono::milliseconds(readThreadSleepTimeMs));
        }
    }
}

void parseFileParallel(std::string filepath) {
    const int threadCount = 16;

    const char *linksFileName = "links.csv";
    const char *nodesFileName = "nodes.csv";

    std::remove(linksFileName);
    std::remove(nodesFileName);

    std::ofstream CSVFileLinks(linksFileName);
    std::ofstream CSVFileNodes(nodesFileName);

    TSQueue<std::string> qIn;
    TSQueue<std::vector<Page>> qOut;

    std::atomic<bool> processKeepAlive = true;
    std::atomic<bool> writerKeepAlive = true;

    CSVFileNodes << "pageName:ID" << std::endl;
    CSVFileLinks << ":START_ID,:END_ID,:TYPE" << std::endl;

    std::vector<std::thread> processorThreads;
    for (int i = 0; i < threadCount; ++i) {
        processorThreads.emplace_back(pageProcessor, std::ref(qIn), std::ref(qOut), std::ref(processKeepAlive));
    }

    std::thread writerThread(csvWriter, std::ref(qOut), std::ref(CSVFileNodes), std::ref(CSVFileLinks),
                             std::ref(writerKeepAlive));

    xmlReader(qIn, qOut, filepath);

    processKeepAlive = false;
    for (auto &t : processorThreads) {
        t.join();
    }

    writerKeepAlive = false;
    writerThread.join();

    CSVFileLinks.close();
    CSVFileNodes.close();
}

int main(int argc, char *argv[]) {
    try {
        std::string filepath;

        if (argc > 1) {
            filepath = argv[1];
        } else {
            throw std::invalid_argument("No file provided");
        }

        parseFileParallel(filepath);

    } catch (const std::exception &e) {
        std::cerr << "Exception caught: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
