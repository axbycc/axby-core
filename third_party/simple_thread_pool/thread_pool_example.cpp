#include <chrono>
#include <iostream>
#include <thread>
#include "simple_thread_pool.h"

int main(int argc, char** argv) {
  std::cout << "This is the thread pool example" << std::endl;

  SimpleThreadPool thread_pool(10);  // ten threads

  for (int i = 0; i < 100; ++i) {
    std::cout << "Calling push" << std::endl;
    thread_pool.Push([i]() {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    });
  }

  while (true) {
    const int n = thread_pool.NumJobsQueued();
    std::cout << n << " jobs currently queued" << std::endl;
    if (n == 0) {
      break;
    }
  }

  std::cout << "All jobs done" << std::endl;
}
