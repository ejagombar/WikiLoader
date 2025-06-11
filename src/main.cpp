#include "TSQueue.hpp"
#include "progress.hpp"
#include "saxparser.hpp"

#include <cstdlib>
#include <iostream>
#include <libxml++/libxml++.h>
#include <libxml++/parsers/textreader.h>
#include <stdexcept>
#include <sys/types.h>
#include <thread>

void pageProcessor(TSQueue<std::string> &qIn, TSQueue<std::vector<Page>> &qOut, std::atomic<bool> &keepAlive) {
    MySaxParser parser;

    while (keepAlive.load() || !qIn.empty()) {
        std::string chunk;

        if (!qIn.pop(chunk)) {
            break;
        }

        parser.parse_memory(chunk);
        qOut.push(parser.getPages());
        parser.clear();
    }
}

void csvWriter(TSQueue<std::vector<Page>> &qIn, std::ofstream &nodeFile, std::ofstream &linkFile,
               std::atomic<bool> &keepAlive) {
    std::string linkStr;
    while (keepAlive.load() || !qIn.empty()) {
        std::vector<Page> pages;

        if (!qIn.pop(pages)) {
            break;
        }

        for (const Page &page : pages) {
            if (page.title.size() == 0) {
                continue;
            }

            linkStr.clear();
            for (const auto &x : page.links) {
                linkStr = linkStr + "\"" + page.title + "\",\"" + x + "\",LINK\n";
            }

            linkFile << linkStr;
            if (page.redirect)
                nodeFile << "\"" + page.title + "\",\"" + page.titleCaps + "\",REDIRECT\n";
            else
                nodeFile << "\"" + page.title + "\",\"" + page.titleCaps + "\",PAGE\n";
        }

        nodeFile << std::flush;
        linkFile << std::flush;
    }
}

void xmlReader(TSQueue<std::string> &qIn, TSQueue<std::vector<Page>> &qOut, std::string &filepath,
               Progress *progress = nullptr) {
    const int maxInputQueueSize = 5;
    const int pagesPerQueueItem = 400;
    const int readThreadSleepTimeMs = 25;

    int pageCount = 0;
    int totalPagesProcessed = 0;
    int pagesSinceLastUpdate = 0;

    std::string output("<mediawiki>");
    xmlpp::TextReader reader(filepath);

    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    double fileSize = 0;
    double totalContentSize = 0;

    if (file.is_open()) {
        fileSize = static_cast<double>(file.tellg());
        file.close();
    }

    while (reader.read()) {
        if (reader.get_name() == "page") {
            std::string pageXml = reader.read_outer_xml();
            output += pageXml;
            pageCount++;

            totalPagesProcessed++;
            pagesSinceLastUpdate++;

            // Track total content processed
            totalContentSize += pageXml.size();

            if (progress) {
                progress->increment();
                progress->setFileProgress((totalContentSize / fileSize) * 100);
            }

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

    if (pageCount > 0) {
        output += "\n</mediawiki>";
        qIn.push(output);
    }

    if (progress) {
        progress->setFileProgress(100.0);
    }
}

void parseFileParallel(std::string filepath) {
    const int threadCount = 16; // TODO: Make this dynamic

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

    // Create progress tracker (no parameters needed now)
    Progress progress;

    CSVFileNodes << "pageName:ID,title,:LABEL" << std::endl;
    CSVFileLinks << ":START_ID,:END_ID,:TYPE" << std::endl;

    std::vector<std::thread> processorThreads;
    for (int i = 0; i < threadCount; ++i) {
        processorThreads.emplace_back(pageProcessor, std::ref(qIn), std::ref(qOut), std::ref(processKeepAlive));
    }

    std::thread writerThread(csvWriter, std::ref(qOut), std::ref(CSVFileNodes), std::ref(CSVFileLinks),
                             std::ref(writerKeepAlive));

    // Pass progress to xmlReader
    xmlReader(qIn, qOut, filepath, &progress);

    processKeepAlive = false;
    qIn.setFinished();

    for (auto &t : processorThreads) {
        t.join();
    }

    writerKeepAlive = false;
    qOut.setFinished();
    writerThread.join();

    // Signal completion
    progress.finish();

    CSVFileLinks.close();
    CSVFileNodes.close();
}

int main(int argc, char *argv[]) {
    try {
        std::string filepath;

        if (argc > 1) {
            filepath = argv[1];
        } else {
            throw std::invalid_argument("No Wikipedia XML dump file provided");
        }

        parseFileParallel(filepath);

    } catch (const std::exception &e) {
        std::cerr << "Exception caught: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
