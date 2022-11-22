#include "sls/Detector.h"
#include <atomic>
#include <cstdlib>
#include <chrono>
#include <fmt/core.h>
#include <fmt/color.h>
#include <iostream>
#include <random>
#include <semaphore>
#include <stdexcept>
#include <thread>

using namespace std::chrono_literals;

class SleepTimer {
    std::random_device rd;
    std::mt19937 e2{rd()};
    std::uniform_real_distribution<> dist{1, 3};
  public:
    void random_sleep() {
        auto t = std::chrono::duration<double>(dist(e2));
        std::this_thread::sleep_for(t);
    }
    void sleep(std::chrono::nanoseconds t) { std::this_thread::sleep_for(t); }
};


std::atomic<bool> acquire{false};
std::counting_semaphore<1> sem{0};
std::mutex m;
SleepTimer timer;

//Global to avoid issues around shm (TODO! should not be needed!!!)
//construction messes with the shm
sls::Detector det;

void acquire_task(std::stop_token stop_token) {
    sls::Detector d;
    int count{0};
    while (!stop_token.stop_requested()) {
        if (!acquire) {
            sem.acquire();
            if (stop_token.stop_requested()) {
                fmt::print("Stopped closing acq thread\n");
                return;
            }
        }
        fmt::print("{}:d.acquire() \n", count);
        d.acquire();
        fmt::print("{}:d.acquire() - done\n", count);
        ++count;
    }
}

void start() {
    acquire = true;
    sem.release();
}

void stop() {
    fmt::print("stop()        \n"); //space to overwrite % in acquire printout
    acquire = false;
    det.stopDetector();
    det.stopReceiver();
    

    // Exit if stop failed
    int stop_counter{0};
    bool check_status{true};
    while(check_status){
        check_status = false;
        for (const auto& s : det.getDetectorStatus()){
            if(s!=sls::defs::runStatus::IDLE){
                fmt::print(fmt::fg(fmt::color::red), "Detector status: {}, after stop!\n", sls::ToString(s));
                check_status = true; //one is not idle have a second look
                ++stop_counter;
                timer.sleep(100ms);
            }
        }
        if (stop_counter > 500){
            fmt::print("Could not stop detector - Exiting\n");
            exit(EXIT_FAILURE);
        }
    }

    fmt::print("stop() - done\n");
}

int main() {
    int max_iter = 10;

    std::jthread t(acquire_task);
    timer.sleep(500ms);
    for (int i = 0; i != max_iter; ++i) {
        start();
        timer.random_sleep();
        stop();
        timer.sleep(300ms); //wait a reasonable time before starting again
    }
    // To exit we need to request stop and then release the
    // semaphore since the acquire_task might be waiting
    t.request_stop();
    sem.release();
}
