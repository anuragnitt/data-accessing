#include <iostream>
#include <string>
#include <chrono>
#include <fstream>
#include "murmurhash3.hpp"
#include "rabinfingerprint.hpp"
#include "cuckoofilter.hpp"
using namespace std;

uint64_t count_lines(const string& filename) {
    fstream file(filename.c_str(), ios_base::in);
    if (not file.good()) {
        file.close();
        throw fstream::failure("failed to open file");
    }

    string line;
    uint64_t count = 0;
    while (getline(file, line)) {
        count++;
    }

    file.close();
    return count;
}

void populate_filter(CuckooFilterLL<string, MurMurHash3, RabinFingerprint>& cuckoo, const string& filename, uint64_t limit) {
    fstream file(filename.c_str(), ios_base::in);
    if (not file.good()) {
        file.close();
        throw fstream::failure("failed to open file");
    }

    string line;
    while (getline(file, line) and limit--) {
        try {
            cuckoo.insert(line);
        }
        catch (const out_of_range& exc) {
            continue;
        }
        catch (const overflow_error& exc) {
            continue;
        }
    }

    file.close();
}

void populate_filter(CuckooFilterHL<string, MurMurHash3, RabinFingerprint>& cuckoo, const string& filename, uint64_t limit) {
    fstream file(filename.c_str(), ios_base::in);
    if (not file.good()) {
        file.close();
        throw fstream::failure("failed to open file");
    }

    string line;
    while (getline(file, line) and limit--) {
        try {
            cuckoo.insert(line);
        }
        catch (const overflow_error& exc) {
            continue;
        }
    }

    file.close();
}

void benchmark(CuckooFilterLL<string, MurMurHash3, RabinFingerprint>& cuckoo, const string& filename, uint64_t limit) {
    using namespace std::chrono;

    std::cout << "\npopulating the filter ... ";
    std::chrono::_V2::system_clock::time_point start = high_resolution_clock::now();

    populate_filter(cuckoo, filename, limit);

    std::chrono::_V2::system_clock::time_point stop = high_resolution_clock::now();
    std::cout << "done\n";

    double time = duration_cast<nanoseconds>(stop - start).count();
    time /= cuckoo.num_keys();

    std::cout << "\nnumber of keys\t\t\t: " << cuckoo.num_keys();
    std::cout << "\nsize of filter (bytes)\t\t: " << cuckoo.size_in_bytes();
    std::cout << "\naverage size per key (bytes)\t: " << ((double) (cuckoo.size_in_bytes()) / 2) / cuckoo.num_keys();
    std::cout << "\nload factor\t\t\t: " << 100 * cuckoo.load_factor() << " %";
    std::cout << "\naverage key insertion time\t: " << time << " ns\n";

    std::cout << "\n1. lookup\n2. deletion\n";

    string input; size_t action;
    while (true) {
        std::cout << "\naction: ";
        std::cin.sync();
        std::cin >> action;

        std::cout << "password: ";
        std::cin.sync();
        getline(std::cin, input);

        if (input == "exit") {
            break;
        }

        start = high_resolution_clock::now();
        bool result;

        if (action == 1) {
            result = cuckoo.lookup(input);
        }

        else if (action == 2) {
            result = cuckoo.remove(input);
        }

        else {
            std::cout << std::endl;
            std::cin.clear();
            break;
        }

        stop = high_resolution_clock::now();
        time = duration_cast<nanoseconds>(stop - start).count();

        if (result and action == 1) {
            std::cout << "password is common";
        }

        else if (result and action == 2) {
            std::cout << "password deleted";
        }

        else if (not result and action == 1) {
            std::cout << "password is unique";
        }

        else if (not result and action == 2) {
            std::cout << "password doesn't exist";
        }

        std::cout << " (operation time: " << time << " nanoseconds)\n";
        input.clear();
    }
}

void benchmark(CuckooFilterHL<string, MurMurHash3, RabinFingerprint>& cuckoo, const string& filename, uint64_t limit) {
    using namespace std::chrono;

    std::cout << "\npopulating the filter ... ";
    std::chrono::_V2::system_clock::time_point start = high_resolution_clock::now();

    populate_filter(cuckoo, filename, limit);

    std::chrono::_V2::system_clock::time_point stop = high_resolution_clock::now();
    std::cout << "done\n";

    double time = duration_cast<nanoseconds>(stop - start).count();
    time /= cuckoo.num_keys();

    std::cout << "\nnumber of keys\t\t\t: " << cuckoo.num_keys();
    std::cout << "\nsize of filter (bytes)\t\t: " << cuckoo.size_in_bytes();
    std::cout << "\naverage size per key (bytes)\t: " << ((double) (cuckoo.size_in_bytes()) / 2) / cuckoo.num_keys();
    std::cout << "\nload factor\t\t\t: " << 100 * cuckoo.load_factor() << " %";
    std::cout << "\naverage key insertion time\t: " << time << " ns\n";

    std::cout << "\n1. lookup\n2. deletion\n";

    string input; size_t action;
    while (true) {
        std::cout << "\naction: ";
        std::cin.sync();
        std::cin >> action;

        std::cout << "password: ";
        std::cin.sync();
        getline(std::cin, input);

        if (input == "exit") {
            break;
        }

        start = high_resolution_clock::now();
        bool result;

        if (action == 1) {
            result = cuckoo.lookup(input);
        }

        else if (action == 2) {
            result = cuckoo.remove(input);
        }

        else {
            std::cout << std::endl;
            std::cin.clear();
            break;
        }

        stop = high_resolution_clock::now();
        time = duration_cast<nanoseconds>(stop - start).count();

        if (result and action == 1) {
            std::cout << "password is common";
        }

        else if (result and action == 2) {
            std::cout << "password deleted";
        }

        else if (not result and action == 1) {
            std::cout << "password is unique";
        }

        else if (not result and action == 2) {
            std::cout << "password doesn't exist";
        }

        std::cout << " (operation time: " << time << " nanoseconds)\n";
        input.clear();
    }
}

int main(void) {
    string filename;
    cout << "enter dictionary path: ";
    cin >> filename;

    try {
        cout << "\nreading file ...\n";
        uint64_t num_keys = count_lines(filename);
        
        double load_factor;
        uint64_t ll_limit, hl_limit;

        cout << "\nupper limit on keys (low load cuckoo filter): ";
        cin >> ll_limit;

        cout << "upper limit on keys (high load cuckoo filter): ";
        cin >> hl_limit;

        cout << "custom load factor (low load cuckoo filter): ";
        cin >> load_factor;

        CuckooFilterLL<string, MurMurHash3, RabinFingerprint> cuckoo_ll(ll_limit, 500, load_factor);
        CuckooFilterHL<string, MurMurHash3, RabinFingerprint> cuckoo_hl(hl_limit, 500, 2);

        cout << "\n------------------ LOW LOAD CUCKOO FILTER ------------------\n";
        benchmark(cuckoo_ll, filename, ll_limit);
        cout << "\n------------------ HIGH LOAD CUCKOO FILTER ------------------\n";
        benchmark(cuckoo_hl, filename, hl_limit);
        return 0;
    }

    catch (const exception& exc) {
        cerr << exc.what() << endl;
        return 1;
    }
}

