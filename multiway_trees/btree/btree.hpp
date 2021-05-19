#pragma once

#include <utility>
#include <sstream>

#ifndef _BTREE_NOT_MODIFIED
	#define _BTREE_NOT_MODIFIED 0
#endif

#ifndef _BTREE_MODIFIED_NOT_ROOT
	#define _BTREE_MODIFIED_NOT_ROOT 1
#endif

#ifndef _BTREE_NEW_ROOT
	#define _BTREE_NEW_ROOT 2
#endif

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
typedef unsigned int size_t;

template <typename _Tp> class BTree;

template <typename _Tp> std::ostream& operator<<(std::ostream&, const BTree<_Tp>&);

template <typename _Tp>
class BNode {
	private:
		BNode** child;
		_Tp* key;
		uint32_t size;
		bool leaf;

		friend class BTree<_Tp>;

		void deepClean(void) noexcept;

	public:
		explicit BNode(const uint32_t);

		~BNode(void);
};

template <typename _Tp>
class BTree {
	private:
		BNode<_Tp>* root;
		const uint32_t minDegree;

		bool (*lessThan)(const _Tp&, const _Tp&) noexcept;
		void (*printKey)(const _Tp&) noexcept;

		uint32_t keyCount;
		uint32_t heightCount;

		void copyNode(BNode<_Tp>*&, const BNode<_Tp>*) noexcept;

		void freeNode(BNode<_Tp>*);

		uint32_t findIndex(const BNode<_Tp>*, const _Tp) const noexcept;

		uint32_t nodeInsert(BNode<_Tp>*, const _Tp) noexcept;

		_Tp nodeDelete(BNode<_Tp>*, uint32_t) noexcept;

		void splitChild(BNode<_Tp>*, const uint32_t) noexcept;

		uint8_t mergeChildren(BNode<_Tp>*, const uint32_t) noexcept;

		uint8_t fixChildSize(BNode<_Tp>*, const uint32_t) noexcept;

		void printNode(std::ostream&, const BNode<_Tp>*, const uint32_t) const noexcept;

	public:
		explicit BTree(const uint32_t, bool (*)(const _Tp&, const _Tp&), void (*)(const _Tp&) = nullptr);

		~BTree(void);

		BTree(const BTree&);

		BTree& operator=(const BTree&);

		constexpr uint32_t num_keys(void) const noexcept;

		constexpr uint32_t height(void) const noexcept;

		constexpr uint64_t size_in_bytes(void) const noexcept;

		void insert(const _Tp) noexcept;

		_Tp remove(const _Tp);

		std::pair<const BNode<_Tp>*, uint32_t> search(const _Tp) const noexcept;

		_Tp searchKey(const _Tp) const;

		friend std::ostream& operator<< <>(std::ostream&, const BTree<_Tp>&);
};

template <typename _Tp>
BNode<_Tp>::BNode(const uint32_t minDegree)
	: size(0), leaf(true) {
	key = new _Tp[2 * minDegree - 1];
	child = new BNode<_Tp>*[2 * minDegree];

	for (uint32_t i = 0; i < 2 * minDegree; i++) {
		child[i] = nullptr;
	}
}

template <typename _Tp>
BNode<_Tp>::~BNode(void) {
	delete[] child;
	delete[] key;
}

template <typename _Tp>
void BNode<_Tp>::deepClean(void) noexcept {
	if (not leaf) {
		for (uint32_t i = 0; i <= size; i++) {
			child[i]->deepClean();
		}
	}

	delete[] child;
	delete[] key;
}

template <typename _Tp>
BTree<_Tp>::BTree(const uint32_t deg, bool (*compare)(const _Tp&, const _Tp&), void (*printK)(const _Tp&))
	: minDegree(deg), lessThan(compare), printKey(printK), keyCount(0), heightCount(0) {
	root = new BNode<_Tp>(minDegree);
}

template <typename _Tp>
BTree<_Tp>::~BTree(void) {
	freeNode(root);
}

template <typename _Tp>
BTree<_Tp>::BTree(const BTree& btree)
	: minDegree(btree.minDegree), lessThan(btree.lessThan), printKey(btree.printKey),
	keyCount(btree.keyCount), heightCount(btree.heightCount) {
	copyNode(root, btree.root);
}

template <typename _Tp>
BTree<_Tp>& BTree<_Tp>::operator=(const BTree& btree) {
	if (minDegree != btree.minDegree)
		throw std::bad_alloc();

	keyCount = btree.keyCount;
	heightCount = btree.heightCount;
	freeNode(root);
	copyNode(root, btree.root);

	return *this;
}

template <typename _Tp>
void BTree<_Tp>::copyNode(BNode<_Tp>*& dest, const BNode<_Tp>* src) noexcept {
	dest = new BNode<_Tp>(minDegree);
	dest->size = src->size;
	dest->leaf = src->leaf;

	for (uint32_t i = 0; i < src->size; i++) {
		dest->key[i] = src->key[i];
	}

	if (not src->leaf) {
		for (uint32_t i = 0; i <= src->size; i++) {
			copyNode(dest->child[i], src->child[i]);
		}
	}
}

template <typename _Tp>
void BTree<_Tp>::freeNode(BNode<_Tp>* node) {
	if (node == nullptr)
		throw std::runtime_error("nullptr bad deallocation");

	if (not node->leaf) {
		for (uint32_t i = 0; i <= node->size; i++) {
			freeNode(node->child[i]);
		}
	}

	delete node;
}

template <typename _Tp>
uint32_t BTree<_Tp>::findIndex(const BNode<_Tp>* node, const _Tp key) const noexcept {
	uint32_t i = 0;
	while (i < node->size and lessThan(node->key[i], key)) {
		i++;
	}
	
	return i;
}

template <typename _Tp>
uint32_t BTree<_Tp>::nodeInsert(BNode<_Tp>* node, const _Tp key) noexcept {
	uint32_t index;

	for (index = node->size; index > 0 and lessThan(key, node->key[index - 1]); index--) {
		node->key[index] = node->key[index - 1];
		node->child[index + 1] = node->child[index];
	}

	node->child[index + 1] = node->child[index];
	node->key[index] = key;
	node->size++;

	return index;
}

template <typename _Tp>
_Tp BTree<_Tp>::nodeDelete(BNode<_Tp>* node, uint32_t index) noexcept {
	_Tp toReturn = node->key[index];
	node->size--;

	while (index < node->size) {
		node->key[index] = node->key[index + 1];
		node->child[index + 1] = node->child[index + 2];
		index++;
	}

	return toReturn;
}

template <typename _Tp>
void BTree<_Tp>::splitChild(BNode<_Tp>* par, const uint32_t index) noexcept {
	BNode<_Tp>* toSplit = par->child[index];
	BNode<_Tp>* newNode = new BNode<_Tp>(minDegree);
	newNode->leaf = toSplit->leaf;
	newNode->size = minDegree - 1;

	for (uint32_t j = 0; j < minDegree - 1; j++) {
		newNode->key[j] = toSplit->key[j + minDegree];
	}

	if (not toSplit->leaf) {
		for (uint32_t j = 0; j < minDegree; j++)
			newNode->child[j] = toSplit->child[j + minDegree];
	}
	toSplit->size = minDegree - 1;

	nodeInsert(par, toSplit->key[minDegree - 1]);
	par->child[index + 1] = newNode;
}

template <typename _Tp>
uint8_t BTree<_Tp>::mergeChildren(BNode<_Tp>* par, const uint32_t index) noexcept {
	BNode<_Tp>* leftChild = par->child[index];
	BNode<_Tp>* rightChild = par->child[index + 1];

	leftChild->key[leftChild->size] = nodeDelete(par, index);
	uint32_t j = ++ leftChild->size;

	for (uint32_t k = 0; k < rightChild->size; k++) {
		leftChild->key[j + k] = rightChild->key[k];
		leftChild->child[j + k] = rightChild->child[k];
	}
	leftChild->size += rightChild->size;
	leftChild->child[leftChild->size] = rightChild->child[rightChild->size];

	delete rightChild;

	if (par->size == 0) {
		root = leftChild;
		delete par;
		heightCount--;
		return _BTREE_NEW_ROOT;
	}

	return _BTREE_MODIFIED_NOT_ROOT;
}

template <typename _Tp>
uint8_t BTree<_Tp>::fixChildSize(BNode<_Tp>* par, const uint32_t index) noexcept {
	BNode<_Tp>* child = par->child[index];

	if (child->size < minDegree) {
		if (index > 0 and par->child[index - 1]->size >= minDegree) {
			BNode<_Tp>* leftSibling = par->child[index - 1];

			for (uint32_t i = nodeInsert(child, par->key[index - 1]); i > 0; i--) {
				child->child[i] = child->child[i - 1];
			}
			
			child->child[0] = leftSibling->child[leftSibling->size];
			par->key[index - 1] = nodeDelete(leftSibling, leftSibling->size - 1);
		}

		else if (index < par->size and par->child[index + 1]->size >= minDegree) {
			BNode<_Tp>* rightSibling = par->child[index + 1];
			nodeInsert(child, par->key[index]);
			child->child[child->size] = rightSibling->child[0];
			rightSibling->child[0] = rightSibling->child[1];
			par->key[index] = nodeDelete(rightSibling, 0);
		}

		else if (index > 0) {
			return mergeChildren(par, index - 1);
		}
		
		else {
			return mergeChildren(par, index);
		}
		
		return _BTREE_MODIFIED_NOT_ROOT;
	}

	return _BTREE_NOT_MODIFIED;
}

template <typename _Tp>
void BTree<_Tp>::printNode(std::ostream& out, const BNode<_Tp>* node, const uint32_t indent) const noexcept {
	for (uint32_t i = 0; i < indent; i++) {
		out << "\t";
	}

	for (uint32_t i = 0; i < node->size; i++) {
		printKey(node->key[i]);
		out << " ";
	}
	out << std::endl;

	if (not node->leaf) {
		for (uint32_t i = 0; i <= node->size; i++)
			printNode(out, node->child[i], indent + 1);
	}
}

template <typename _Tp>
constexpr uint32_t BTree<_Tp>::num_keys(void) const noexcept {
	return keyCount;
}

template <typename _Tp>
constexpr uint32_t BTree<_Tp>::height(void) const noexcept {
	return heightCount;
}

template <typename _Tp>
constexpr uint64_t BTree<_Tp>::size_in_bytes(void) const noexcept {
	return keyCount * sizeof(_Tp);
}

template <typename _Tp>
void BTree<_Tp>::insert(const _Tp key) noexcept {
	if (root->size == 2 * minDegree - 1) {
		BNode<_Tp>* newRoot = new BNode<_Tp>(minDegree);
		newRoot->leaf = false;
		newRoot->child[0] = root;
		root = newRoot;
		splitChild(newRoot, 0);
		heightCount++;
	}

	BNode<_Tp>* curr = root;
	while (not curr->leaf) {
		uint32_t index = curr->size - 1;
		while (index >= 0 and lessThan(key, curr->key[index])) {
			index--;
		}
		index++;

		if (curr->child[index]->size == 2 * minDegree - 1) {
			splitChild(curr, index);
			if (lessThan(curr->key[index], key)) {
				index++;
			}
		}

		curr = curr->child[index];
	}

	nodeInsert(curr, key);
	keyCount++;
}

template <typename _Tp>
_Tp BTree<_Tp>::remove(const _Tp key) {
	BNode<_Tp>* curr = root;

	while (true) {
		uint32_t i = findIndex(curr, key);
		
		if (i < curr->size and not (lessThan(curr->key[i], key) or lessThan(key, curr->key[i]))) {
			_Tp toReturn = curr->key[i];
			if (curr->leaf) {
				nodeDelete(curr, i);
			}
			
			else {
				BNode<_Tp>* leftChild = curr->child[i];
				BNode<_Tp>* rightChild = curr->child[i + 1];

				if (leftChild->size >= minDegree) {
					while (not leftChild->leaf) {
						fixChildSize(leftChild, leftChild->size);
						leftChild = leftChild->child[leftChild->size];
					}
					curr->key[i] = nodeDelete(leftChild, leftChild->size - 1);
				}

				else if (rightChild->size >= minDegree) {
					while (not rightChild->leaf) {
						fixChildSize(rightChild, 0);
						rightChild = rightChild->child[0];
					}
					curr->key[i] = nodeDelete(rightChild, 0);
				}

				else {
					mergeChildren(curr, i);
					curr = leftChild;
					continue;
				}
			}

			return toReturn;
		}

		else {
			if (curr->leaf) {
				if (printKey == nullptr) {
					throw std::out_of_range("_BTREE_KEY_NOT_FOUND");
				}
		
				std::stringstream newbuf;
				std::streambuf* oldbuf = std::cout.rdbuf(newbuf.rdbuf());
				printKey(key);
				std::cout.rdbuf(oldbuf);
				throw std::out_of_range("_BTREE_KEY_NOT_FOUND: " + newbuf.str());
			}

			if (fixChildSize(curr, i) == _BTREE_NEW_ROOT) {
				curr = root;
			}
			
			else {
				curr = curr->child[findIndex(curr, key)];
			}
		}
	}

	keyCount--;
}

template <typename _Tp>
std::pair<const BNode<_Tp>*, uint32_t> BTree<_Tp>::search(const _Tp key) const noexcept {
	BNode<_Tp>* curr = root;
	
	while (true) {
		uint32_t i = findIndex(curr, key);

		if (i < curr->size and not (lessThan(key, curr->key[i]) or lessThan(curr->key[i], key))) {
			return std::pair<const BNode<_Tp>*, uint32_t>(curr, i);
		}
		
		else if (curr->leaf) {
			return std::pair<const BNode<_Tp>*, uint32_t>(nullptr, 0);
		}

		else {
			curr = curr->child[i];
		}
	}
}

template <typename _Tp>
_Tp BTree<_Tp>::searchKey(const _Tp key) const {
	std::pair<const BNode<_Tp>*, uint32_t> node = search(key);
	if (node.first == nullptr) {
		if (printKey == nullptr) {
			throw std::out_of_range("_BTREE_KEY_NOT_FOUND");
		}

		std::stringstream newbuf;
		std::streambuf* oldbuf = std::cout.rdbuf(newbuf.rdbuf());
		printKey(key);
		std::cout.rdbuf(oldbuf);
		throw std::out_of_range("_BTREE_KEY_NOT_FOUND: " + newbuf.str());
	}
	
	return node.first->key[node.second];
}

template <typename _Tp>
std::ostream& operator<<(std::ostream& out, const BTree<_Tp>& btree) {
	if (btree.printKey != nullptr and btree.root->size > 0) {
		out << std::endl;
		btree.printNode(out, btree.root, 0);
		out << std::endl;
	}

	return out;
}
