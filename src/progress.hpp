#include <atomic>
#include <chrono>
#include <ctime>
#include <deque>
#include <string>

#define cursup "\033[A"

class Progress {
  public:
    Progress();
    ~Progress() { runOutputThread = false; };

    void displayProgress();
    void increment();
    void setFileProgress(double percentage);
    void finish();

  private:
    std::atomic<int> processedPageCount = 0;
    std::atomic<bool> runOutputThread = true;
    std::atomic<double> fileProgressPercentage = -1.0; // -1 is unknown
    std::atomic<std::chrono::time_point<std::chrono::system_clock>> startTime;

    std::deque<std::pair<std::chrono::time_point<std::chrono::system_clock>, int>> rateHistory;
    std::atomic<double> currentRate = 0.0; // pages per second

    void updateProcessingRate();
    std::string formatDuration(std::chrono::seconds duration);
    std::string formatRate(double rate);
};
