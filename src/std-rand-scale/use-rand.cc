// g++ -std=c++14 -Wall -Wextra use-rand.cc -o use-rand -lpthread
#include <random>
#include <chrono>
#include <thread>
#include <vector>
#include <iostream>
#include <atomic>

#include <assert.h>

using namespace std;

atomic<long> globSum;

void threadMain(void)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<long> dis(0, 99);

    long sum = 0L;
    for (long i = 0; i < (1<<25); i++) {
        sum += dis(gen);
    }
    globSum += sum; // so dis() isn't optimized out
}

void runTest(int nthreads)
{
    vector<thread*> threads;

    auto c1 = chrono::steady_clock::now();
    for (int i = 0; i < nthreads; i++) {
        threads.push_back(new thread(threadMain));
    }
    for (thread* p : threads) {
        p->join();
    }
    auto c2 = chrono::steady_clock::now();
    cout << nthreads
        << " " << chrono::duration<float>(c2-c1).count()
        << endl;
}

int main(void)
{
    cout << "nThreads time(sec)" << endl;
    for (int t = 0; t < 16; t++) {
        globSum.store(0L);
        runTest(t);
    }
    assert( globSum.fetch_and(0L) > 0L );
    return 0;
}
