#include <climits>
#include <deque>
#include <random>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <list>

std::vector<long> data;

void runtest(void)
{
    std::random_device rd;
    std::mt19937_64 mt(rd());
    std::uniform_int_distribution<unsigned long> dis(0, ULONG_MAX-1);

    long n = (1L << 26);
    data.resize(n);
    for (long i = 0; i < n; i++)
        data[i] = dis(mt);

    std::sort(data.begin(), data.end(), [](long a, long b) { return a < b; } );
}

int main(void)
{
    runtest();
    return 0;
}
