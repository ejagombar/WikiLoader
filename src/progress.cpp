#include "progress.hpp"
#include <ctime>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <thread>

Progress::Progress() {
    startTime = std::chrono::system_clock::now();
    std::thread(&Progress::displayProgress, this).detach();
}

void Progress::increment() {
    processedPageCount++;
    updateProcessingRate();
}

void Progress::setFileProgress(double percentage) {
    if (percentage >= 0.0 && percentage <= 100.0) {
        fileProgressPercentage = percentage;
    }
}

void Progress::finish() { runOutputThread = false; }

void Progress::updateProcessingRate() {
    static std::mutex rateMutex;
    std::lock_guard<std::mutex> lock(rateMutex);

    auto now = std::chrono::system_clock::now();
    int currentCount = processedPageCount.load();

    rateHistory.push_back({now, currentCount});

    // Keep only last 30 seconds of data for a rolling average
    auto cutoff = now - std::chrono::seconds(30);
    while (!rateHistory.empty() && rateHistory.front().first < cutoff) {
        rateHistory.pop_front();
    }

    // Calculate rate if we have at least 2 data points
    if (rateHistory.size() >= 2) {
        auto oldest = rateHistory.front();
        auto newest = rateHistory.back();

        auto timeDiff = std::chrono::duration_cast<std::chrono::milliseconds>(newest.first - oldest.first).count();
        int countDiff = newest.second - oldest.second;

        if (timeDiff > 0) {
            currentRate = (static_cast<double>(countDiff) / timeDiff) * 1000.0; // pages per second
        }
    }
}

std::string Progress::formatDuration(std::chrono::seconds duration) {
    auto hours = std::chrono::duration_cast<std::chrono::hours>(duration);
    auto minutes = std::chrono::duration_cast<std::chrono::minutes>(duration % std::chrono::hours(1));
    auto seconds = duration % std::chrono::minutes(1);

    std::ostringstream oss;
    if (hours.count() > 0) {
        oss << hours.count() << "h " << minutes.count() << "m " << seconds.count() << "s";
    } else if (minutes.count() > 0) {
        oss << minutes.count() << "m " << seconds.count() << "s";
    } else {
        oss << seconds.count() << "s";
    }
    return oss.str();
}

std::string Progress::formatRate(double rate) {
    std::ostringstream oss;
    if (rate >= 1000.0) {
        oss << std::fixed << std::setprecision(1) << (rate / 1000.0) << "k pages/s";
    } else if (rate >= 1.0) {
        oss << std::fixed << std::setprecision(1) << rate << " pages/s";
    } else if (rate > 0.0) {
        oss << std::fixed << std::setprecision(1) << (rate * 60.0) << " pages/min";
    } else {
        oss << "calculating...";
    }
    return oss.str();
}

void Progress::displayProgress() {
    std::cout << "┌──────────────────────────────────────┐\n";
    std::cout << "│         Wikipedia XML Parser         │\n";
    std::cout << "├──────────────────────────────────────┤\n";
    std::cout << "│                                      │\n";
    std::cout << "│                                      │\n";
    std::cout << "│                                      │\n";
    std::cout << "│                                      │\n";
    std::cout << "│                                      │\n";
    std::cout << "└──────────────────────────────────────┘\n";

    int width = 19;

    std::cout << "\r" << cursup;

    while (runOutputThread) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        int count = processedPageCount.load();
        auto start = startTime.load();
        auto now = std::chrono::system_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start);
        double rate = currentRate.load();
        double fileProgress = fileProgressPercentage.load();

        std::cout << "\r" << cursup << cursup << cursup << cursup << cursup;

        std::cout << "│ Pages processed: " << std::setw(width) << std::left << std::to_string(count) << " │\n";
        std::cout << "│ Processing rate: " << std::setw(width) << std::left << formatRate(rate) << " │\n";
        std::cout << "│ Elapsed time:    " << std::setw(width) << std::left << formatDuration(elapsed) << " │\n";

        // Show file progress if available
        if (fileProgress >= 0.0) {
            std::ostringstream progressStr;
            progressStr << std::fixed << std::setprecision(1) << fileProgress << "%";
            std::cout << "│ File progress:   " << std::setw(width) << std::left << progressStr.str() << " │\n";

            // Estimate remaining time based on file progress
            if (fileProgress > 0.1 && fileProgress < 99.9) {
                double estimatedTotal = elapsed.count() / (fileProgress / 100.0);
                auto remaining = std::chrono::seconds(static_cast<long>(estimatedTotal - elapsed.count()));
                std::cout << "│ Est. remaining:  " << std::setw(width) << std::left << formatDuration(remaining)
                          << " │\n";
            } else {
                std::cout << "│ Est. remaining:  " << std::setw(width) << std::left << "calculating..." << " │\n";
            }
        } else {
            std::cout << "│ File progress:   " << std::setw(width) << std::left << "unknown" << " │\n";
            std::cout << "│ Est. remaining:  " << std::setw(width) << std::left << "unknown" << " │\n";
        }

        std::cout << std::flush;
    }

    std::cout << "\r" << cursup << cursup << cursup << cursup << cursup;
    std::cout << "│ Status:          " << std::setw(width) << std::left << "COMPLETED!" << " │\n";
    std::cout << "│ Total processed: " << std::setw(width) << std::left << std::to_string(processedPageCount.load())
              << " │\n";
    auto finalElapsed =
        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - startTime.load());
    std::cout << "│ Total time:      " << std::setw(width) << std::left << formatDuration(finalElapsed) << " │\n";
    std::cout << "│ Final rate:      " << std::setw(width) << std::left << formatRate(currentRate.load()) << " │\n";
    std::cout << "│                                             │\n";
    std::cout << "└─────────────────────────────────────────────┘\n";
    std::cout << "Processing complete! \n" << std::endl;
}
