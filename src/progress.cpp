#include "progress.h"
#include <ctime>
#include <iomanip>
#include <iostream>
#include <thread>

Progress::Progress(int total) {
    this->total = total;
    startTime = std::chrono::system_clock::now();
    std::thread(&Progress::displayProgress, this).detach();
}

void Progress::increment() { processedPageCount++; }

void Progress::displayProgress() {
    std::cout << "---------Info---------\n\n-------Loading--------\n\n" << std::endl;

    while (runOutputThread) {
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
        int count = processedPageCount.load();

        auto start = startTime.load();
        auto end = std::chrono::system_clock::now();

        std::chrono::duration<double> elapsed_seconds = end - start;

        auto remainingSeconds = ((elapsed_seconds / count) * total - elapsed_seconds).count();
        int hoursLeft = static_cast<int>(remainingSeconds) / 3600;
        int minutesLeft = (static_cast<int>(remainingSeconds) % 3600) / 60;
        int secondsLeft = static_cast<int>(remainingSeconds) % 60;

        int hoursTaken = static_cast<int>(elapsed_seconds.count()) / 3600;
        int minutesTaken = (static_cast<int>(elapsed_seconds.count()) % 3600) / 60;
        int secondsTaken = static_cast<int>(elapsed_seconds.count()) % 60;

        float percentageDone = (static_cast<float>(count) / total) * 100.0;

        std::cout << std::setprecision(3) << std::fixed << "\r" << cursup << cursup << cursup << cursup
                  << "Page Number: " << count << "            \nProgress: " << percentageDone
                  << "%           \nTime Left: " << hoursLeft << " hrs " << minutesLeft << " mins " << secondsLeft
                  << " secs         \nTime Taken: " << hoursTaken << " hrs " << minutesTaken << " mins " << secondsTaken
                  << " secs         \n"

                  << std::flush;
    }
    std::cout << "Done!\a" << std::endl;
}
