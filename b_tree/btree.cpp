#include <iostream>
#include <utility>
#include "btree.hpp"
using namespace std;

bool cmp(int a, int b) {
	return a < b;
}

void print(int x) {
	std::cout << x;
}

int main(void) {
	BTree<int> bt(5, cmp, print);
	BTree<int> bt2 = bt;
	BTree<int> bt3(6, cmp, print);
	bt = bt3;
	return 0;
}
