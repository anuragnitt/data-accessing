#include <iostream>
#include <string>
#include <chrono>
#include <fstream>
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

void populate_filter(CuckooFilterHL<string>& cuckoo, const string& filename) {
    fstream file(filename.c_str(), ios_base::in);
    if (not file.good()) {
        file.close();
        throw fstream::failure("failed to open file");
    }

    string line;
    uint64_t count = 0;
    while (getline(file, line)) {
        try {
            cuckoo.insert(line);
        }
        catch (const overflow_error& exc) {
            continue;
        }
    }

    file.close();
}

void populate_filter(CuckooFilterLL<string>& cuckoo, const string& filename) {
    fstream file(filename.c_str(), ios_base::in);
    if (not file.good()) {
        file.close();
        throw fstream::failure("failed to open file");
    }

    string line;
    uint64_t count = 0;
    while (getline(file, line)) {
        try {
            cuckoo.insert(line);
        }
        catch (const overflow_error& exc) {
            continue;
        }
    }

    file.close();
}

void benchmark(CuckooFilterHL<string>& cuckoo, const string& filename) {
    using namespace std::chrono;

    std::cout << "\npopulating the filter ... ";
    std::chrono::_V2::system_clock::time_point start = high_resolution_clock::now();

    
    populate_filter(cuckoo, filename);

    std::chrono::_V2::system_clock::time_point stop = high_resolution_clock::now();
    std::cout << "done\n";

    double time = duration_cast<nanoseconds>(stop - start).count();
    time /= cuckoo.num_keys();

    std::cout << "\nnumber of keys: " << cuckoo.num_keys();
    std::cout << "\nsize of filter (bytes): " << 4 * cuckoo.size_in_bytes();
    std::cout << "\naverage size per key in bytes: " << (4.0 * cuckoo.size_in_bytes()) / cuckoo.num_keys();
    std::cout << "\nload factor: " << 100 * cuckoo.load_factor() << " %";
    std::cout << "\naverage key insertion time: " << time << " ns\n";

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

void benchmark(CuckooFilterLL<string>& cuckoo, const string& filename) {
    using namespace std::chrono;

    std::cout << "\npopulating the filter ... ";
    std::chrono::_V2::system_clock::time_point start = high_resolution_clock::now();

    
    populate_filter(cuckoo, filename);

    std::chrono::_V2::system_clock::time_point stop = high_resolution_clock::now();
    std::cout << "done\n";

    double time = duration_cast<nanoseconds>(stop - start).count();
    time /= cuckoo.num_keys();

    std::cout << "\nnumber of keys: " << cuckoo.num_keys();
    std::cout << "\nsize of filter (bytes): " << 4 * cuckoo.size_in_bytes();
    std::cout << "\naverage size per key in bytes: " << (4.0 * cuckoo.size_in_bytes()) / cuckoo.num_keys();
    std::cout << "\nload factor: " << 100 * cuckoo.load_factor() << " %";
    std::cout << "\naverage key insertion time: " << time << " ns\n";

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
        double load_factor;
        cout << "custom load factor (for low load cuckoo filter): ";
        cin >> load_factor;

        cout << "\nreading file ...\n";
        uint64_t num_keys = count_lines(filename);

        CuckooFilterLL<string> cuckoo_ll(num_keys, 500, load_factor);
        CuckooFilterHL<string> cuckoo_hl(num_keys, 500, 2);

        cout << "\n------------------ LOW LOAD CUCKOO FILTER ------------------\n";
        benchmark(cuckoo_ll, filename);
        cout << "\n------------------ HIGH LOAD CUCKOO FILTER ------------------\n";
        benchmark(cuckoo_hl, filename);
        return 0;
    }

    catch (const exception& exc) {
        cerr << exc.what() << endl;
        return 1;
    }
}

