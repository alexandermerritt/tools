#include <deque>
#include <random>
#include <fstream>
#include <iostream>
#include <list>

std::list<std::string> data;

const std::string input_file = "/usr/share/dict/words";

void load(void)
{
    std::ifstream ifs;
    ifs.open(input_file.c_str());
    if (!ifs.is_open()) {
        perror("ifs.open");
        exit(1);
    }
    std::string line;
    while (ifs >> line) {
        data.push_back(line);
    }
    ifs.close();
}

void runtest(void)
{
    load();
    const size_t n = data.size();
    // find() or count() will do a hash computation and lookup
    for (size_t i = 0; i < (n << 0); i++)
        for (auto &s : data)
            (void)s;
}

int main(void)
{
    runtest();
    return 0;
}
