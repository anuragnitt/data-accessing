#include <iostream>
#include <utility>
#include <string>
#include <chrono>
#include <fstream>
#include "bplustree.hpp"
using namespace std;

void benchmark(BPTree<int>& tree) {
	using namespace std::chrono;

	std::cout << "\npopulating the tree ... ";
    std::chrono::_V2::system_clock::time_point start = high_resolution_clock::now();

    for (int i = -50000; i < 50000; i++) {
        tree.insert(i + 1);
    }

    std::chrono::_V2::system_clock::time_point stop = high_resolution_clock::now();
    std::cout << "done\n";

	double time = duration_cast<nanoseconds>(stop - start).count();
    time /= tree.num_keys();

	std::cout << "\nnumber of keys: " << tree.num_keys();
    std::cout << "\nsize of tree (bytes): " << tree.size_in_bytes();
    std::cout << "\naverage size per key (bytes): " << (double)tree.size_in_bytes() / tree.num_keys();
    std::cout << "\naverage key insertion time: " << time << " ns\n";

	int input;
    while (true) {
        std::cout << "\nlookup number: ";
        std::cin.sync();
        cin >> input;

        start = high_resolution_clock::now();

		try {
			tree.searchKey(input);
			cout << "number found";
		}
		catch (const out_of_range& exc) {
			cout << "number not found";
		}

        stop = high_resolution_clock::now();
        time = duration_cast<nanoseconds>(stop - start).count();

        std::cout << " (operation time: " << time << " nanoseconds)\n";
    }
}

bool compare(const int& a, const int& b) {
	return a < b;
}

void print(const int& x) {
	cout << x;
}

int main(void) {
    try {
		size_t deg;
		cout << "minimum degree of btree: ";
		cin >> deg;

        BPTree<int> tree(deg, compare, print);
        benchmark(tree);
        return 0;
    }

    catch (const exception& exc) {
        cerr << exc.what() << endl;
        return 1;
    }
}
