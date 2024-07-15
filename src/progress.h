#include <atomic>
#include <chrono>
#include <ctime>

#define cursup "\033[A"

class Progress {
  public:
    Progress(int total);
    ~Progress() { runOutputThread = false; };

    void displayProgress();
    void increment();

  private:
    int total = 0;
    std::atomic<int> processedPageCount = 0;
    std::atomic<bool> runOutputThread = true;
    std::atomic<std::chrono::time_point<std::chrono::system_clock>> startTime;
};
