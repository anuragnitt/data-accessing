#include <iostream>
#include <string>
#include <fstream>
#include <chrono>
#include "bloomfilter.hpp"
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

uint64_t size_by_fp_prob(const uint64_t num_keys, double fp_prob) {
    if (fp_prob <= 0 or fp_prob > 1) {
        throw invalid_argument("invalid probability");
    }

    return ceil(-(num_keys / log(2)) * (log2(fp_prob)));
}

void populate_filter(BloomFilter<string>& bloom, const string& filename) {
    fstream file(filename.c_str(), ios_base::in);
    if (not file.good()) {
        file.close();
        throw fstream::failure("failed to open file");
    }

    string line;
    while (getline(file, line)) {
        bloom.insert(line);
    }

    file.close();
}

void benchmark(BloomFilter<string>& bloom, const string& filename) {
    using namespace std::chrono;

    std::cout << "\npopulating the filter ... ";
    std::chrono::_V2::system_clock::time_point start = high_resolution_clock::now();

    populate_filter(bloom, filename);

    std::chrono::_V2::system_clock::time_point stop = high_resolution_clock::now();
    std::cout << "done\n";

    double time = duration_cast<nanoseconds>(stop - start).count();
    time /= bloom.num_keys();

    std::cout << "\nnumber of keys: " << bloom.num_keys();
    std::cout << "\nsize of filter (bytes): " << bloom.size_in_bytes();
    std::cout << "\naverage size per key in bytes: " << (double)bloom.size_in_bytes() / bloom.num_keys();
    std::cout << "\nspace occupancy: " << 100 * bloom.occupancy_ratio() << " %";
    std::cout << "\naverage key insertion time: " << time << " ns";
    std::cout << "\nfalse positive probabilty: " << 100 * bloom.fp_prob() << " %\n";

    string input;
    while (true) {
        std::cout << "\nlookup password: ";
        std::cin.sync();
        getline(std::cin, input);

        if (input == "exit") {
            break;
        }

        start = high_resolution_clock::now();
        bool result = bloom.lookup(input);
        stop = high_resolution_clock::now();
        time = duration_cast<nanoseconds>(stop - start).count();

        if (result) {
            std::cout << "password is common";
        }
        else {
            std::cout << "password is unique";
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
        double fp_prob;
        cout << "custom false positive probability: ";
        cin >> fp_prob;

        cout << "\nreading file ...\n";
        uint64_t num_keys = count_lines(filename);
        uint64_t size = size_by_fp_prob(num_keys, fp_prob);

        BloomFilter<string> bloom(num_keys, size);
        benchmark(bloom, filename);
        return 0;
    }

    catch (const exception& exc) {
        cerr << exc.what() << endl;
        return 1;
    }
}
