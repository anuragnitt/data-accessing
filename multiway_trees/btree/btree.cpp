#include <iostream>
#include <utility>
#include "btree.hpp"
using namespace std;

bool compare(int a, int b) {
	return a < b;
}

void printInt(int x) {
	std::cout << x;
}

int main(void) {
	try {
		BTree<int> bt(3, compare, printInt);

		for (int i = 0; i < 20; i++) {
			bt.insert(i+1);
		}

		BTree<int> bt2 = bt;
		bt.remove(15);
		cout << bt;
		cout << bt2;

		bt = bt2;
		cout << bt;
		return 0;
	}

	catch (const std::exception& exc) {
		cerr << exc.what() << std::endl;
		return 1;
	}
}
