#include <stdlib.h>
#include <string.h>

#include <deque>
#include <fstream>
#include <iostream>
#include <random>
#include <unordered_map>
#include <unordered_set>

std::unordered_map<std::string, int> data;
std::deque<std::string> keys;

enum {
    MAX_KEY_LEN = 64,
    ALPHA_LEN   = 62,
};

const char alpha[] = "abcdefghijklmnopqrstuvwxyz" 
                     "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                     "0123456789";

void runtest(int nkeys)
{
    std::cout << "Init" << std::endl;

    std::random_device rd;
    std::mt19937_64 mt(rd());
    std::uniform_int_distribution<unsigned long> dis(0, nkeys-1);

    for (int i = 0; i < nkeys; i++) {
        std::string s(1 + (dis(mt) % MAX_KEY_LEN), 'x');
        for (size_t c = 0; c < s.size(); c++)
            s[c] = alpha[dis(mt) % ALPHA_LEN];
        keys.push_back(s);
        data[s] = 1337;
    }

    const size_t n = keys.size();

    int *idxs = new int[n];
    if (!idxs)
        exit(1);
    for (size_t i = 0; i < n; i++)
        idxs[i] = i;
    for (size_t i = 0; i < (n << 2); i++)
        std::swap(idxs[dis(mt)], idxs[dis(mt)]);

    std::cout << "Running" << std::endl;

    for (size_t i = 0; i < (n << 10); i++)
        volatile int d = data[ keys[ dis(mt) ] ];
        //volatile int d = data[ keys[ idxs[ i % n ] ] ];

    delete [] idxs;
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        std::cerr << "specify number of keys" << std::endl;
        return 1;
    }
    const int nkeys = strtol(argv[1], nullptr, 10);
    runtest(nkeys);
    return 0;
}
