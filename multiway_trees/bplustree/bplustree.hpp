#pragma once

#include <utility>
#include <sstream>

#ifndef _BPLUSTREE_NOT_MODIFIED
	#define _BPLUSTREE_NOT_MODIFIED 0
#endif

#ifndef _BPLUSTREE_MODIFIED_NOT_ROOT
	#define _BPLUSTREE_MODIFIED_NOT_ROOT 1
#endif

#ifndef _BPLUSTREE_NEW_ROOT
	#define _BPLUSTREE_NEW_ROOT 2
#endif

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
typedef unsigned int size_t;

template <typename _Tp> class BPTree;

template <typename _Tp> std::ostream& operator<<(std::ostream&, const BPTree<_Tp>&);

template <typename _Tp>
class BPNode {
    private:
        BPNode** child;
        _Tp* key;
        uint32_t size;
        bool leaf;

        void deepClean(void) noexcept;

        friend class BPTree<_Tp>;

    public:
	    explicit BPNode(const uint32_t);

        ~BPNode(void);
};

template <typename _Tp>
class BPTree {
    private:
        BPNode<_Tp>* root;
        const uint32_t minDegree;

        bool (*lessThan)(const _Tp&, const _Tp&) noexcept;
        void (*printKey)(const _Tp&) noexcept;

        uint32_t keyCount;
        uint32_t heightCount;
        
        void copyNode(BPNode<_Tp>*, const BPNode<_Tp>*) noexcept;

        void freeNode(BPNode<_Tp>*);

        BPNode<_Tp>* findParent(BPNode<_Tp>*, const BPNode<_Tp>*) const noexcept;

        uint8_t insertInternal(const _Tp, BPNode<_Tp>*, BPNode<_Tp>*) noexcept;

        uint8_t removeInternal(_Tp, BPNode<_Tp>*, BPNode<_Tp>*) noexcept;

        void printNode(std::ostream&, const BPNode<_Tp>*, const uint32_t) const noexcept;

    public:
        explicit BPTree(const uint32_t, bool (*)(const _Tp&, const _Tp&), void (*)(const _Tp&) = nullptr);

        ~BPTree(void);

        BPTree(const BPTree&);

        BPTree& operator=(const BPTree&);

        constexpr uint32_t num_keys(void) const noexcept;

        constexpr uint32_t height(void) const noexcept;

        constexpr uint64_t size_in_bytes(void) const noexcept;

        void insert(const _Tp) noexcept;

        void remove(const _Tp);

        std::pair<const BPNode<_Tp>*, uint32_t> search(const _Tp) const noexcept;

        _Tp searchKey(const _Tp key) const;

        friend std::ostream& operator<< <>(std::ostream&, const BPTree<_Tp>&);
};

template <typename _Tp>
BPNode<_Tp>::BPNode(const uint32_t minDegree)
    : size(0), leaf(true) {
	key = new _Tp[2 * minDegree - 1];
	child = new BPNode<_Tp>*[2 * minDegree];

    for (uint32_t i = 0; i < 2 * minDegree; i++) {
        child[i] = nullptr;
    }
}

template <typename _Tp>
BPNode<_Tp>::~BPNode(void) {
    delete[] child;
    delete[] key;
}

template <typename _Tp>
void BPNode<_Tp>::deepClean(void) noexcept {
    if (not leaf) {
        for (uint32_t i = 0; i <= size; i++) {
            child[i]->deepClean();
        }
    }

    delete[] child;
    delete[] key;
}

template <typename _Tp>
BPTree<_Tp>::BPTree(const uint32_t deg, bool (*compare)(const _Tp&, const _Tp&), void (*printK)(const _Tp&))
    : minDegree(deg), lessThan(compare), printKey(printK), keyCount(0), heightCount(0) {
	root = nullptr;
}

template <typename _Tp>
BPTree<_Tp>::~BPTree(void) {
	freeNode(root);
}

template <typename _Tp>
BPTree<_Tp>::BPTree(const BPTree& bptree)
    : minDegree(bptree.minDegree), lessThan(bptree.lessThan), printKey(bptree.printKey),
    keyCount(bptree.keyCount), heightCount(bptree.heightCount) {
    copyNode(root, bptree.root);
}

template <typename _Tp>
BPTree<_Tp>& BPTree<_Tp>::operator=(const BPTree& bptree) {
    if (minDegree != bptree.minDegree) {
        throw std::bad_alloc();
    }

    keyCount = bptree.keyCount;
    heightCount = bptree.heightCount;
    freeNode(root);
    copyNode(root, bptree.root);

    return *this;
}

template <typename _Tp>
void BPTree<_Tp>::copyNode(BPNode<_Tp>* dest, const BPNode<_Tp>* src) noexcept {
    dest = new BPNode<_Tp>(minDegree);
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
void BPTree<_Tp>::freeNode(BPNode<_Tp>* node) {
	if (node == nullptr) {
        throw std::runtime_error("nullptr bad deallocation");
    }

	if(not node->leaf) {
		for(uint32_t i = 0; i < node->size + 1; i++) {
			freeNode(node->child[i]);
		}
	}

	delete node;
}

template <typename _Tp>
BPNode<_Tp>* BPTree<_Tp>::findParent(BPNode<_Tp>* curr, const BPNode<_Tp>* child) const noexcept {
	if (curr->leaf or (curr->child[0])->leaf) {
		return nullptr;
	}

    BPNode<_Tp>* par;
	for (uint32_t i = 0; i < curr->size+1; i++) {
		if(curr->child[i] == child) {
			par = curr;
			return par;
		}

		else {
			par = findParent(curr->child[i], child);
			if (par != nullptr) {
                return par;
            }
		}
	}

	return par;
}

template <typename _Tp>
uint8_t BPTree<_Tp>::insertInternal(const _Tp key, BPNode<_Tp>* curr, BPNode<_Tp>* child) noexcept {
	if (curr->size < 2 * minDegree - 1) {
		uint32_t i = 0;
		while (lessThan(curr->key[i], key) and i < curr->size) {
            i++;
        }

		for (uint32_t j = curr->size; j > i; j--) {
			curr->key[j] = curr->key[j - 1];
		}

		for (uint32_t j = curr->size + 1; j > i + 1; j--) {
			curr->child[j] = curr->child[j - 1];
		}

		curr->key[i] = key;
		curr->size++;
		curr->child[i + 1] = child;

        return _BPLUSTREE_MODIFIED_NOT_ROOT;
	}

	else {
		BPNode<_Tp>* newInternal = new BPNode<_Tp>(minDegree);

		_Tp virtualKey[2 * minDegree];
		BPNode<_Tp>* virtualChild[2 * minDegree + 1];

		for (uint32_t i = 0; i < 2 * minDegree - 1; i++) {
			virtualKey[i] = curr->key[i];
		}

		for (int i = 0; i < 2 * minDegree; i++) {
			virtualChild[i] = curr->child[i];
		}

		uint32_t i = 0, j;
		while (lessThan(virtualKey[i], key) and i < 2 * minDegree - 1) {
            i++;
        }

		for (uint32_t j = 2 * minDegree; j > i; j--) {
			virtualKey[j] = virtualKey[j - 1];
		}

		virtualKey[i] = key; 
		for(uint32_t j = 2 * minDegree + 1; j > i + 1; j--) {
			virtualChild[j] = virtualChild[j - 1];
		}

		virtualChild[i + 1] = child; 
		newInternal->leaf = false;

		curr->size = minDegree;
		newInternal->size = minDegree - 1;

		for(i = 0, j = curr->size + 1; i < newInternal->size; i++, j++) {
			newInternal->key[i] = virtualKey[j];
		}

		for(i = 0, j = curr->size + 1; i < newInternal->size + 1; i++, j++) {
			newInternal->child[i] = virtualChild[j];
		}


		if(curr == root)
		{
			BPNode<_Tp>* newRoot = new BPNode<_Tp>(minDegree);
			newRoot->leaf = false;
			newRoot->key[0] = curr->key[curr->size];
			newRoot->child[0] = curr;
			newRoot->child[1] = newInternal;
			newRoot->size += 1;
			root = newRoot;

            return _BPLUSTREE_NEW_ROOT;
		}

		else {
			return insertInternal(curr->key[curr->size], findParent(root, curr), newInternal);
		}
	}
}

template <typename _Tp>
uint8_t BPTree<_Tp>::removeInternal(const _Tp key, BPNode<_Tp>* curr, BPNode<_Tp>* child) noexcept {
	if (curr == root) {
		if (curr->size == 1) {
			if (curr->child[1] == child) {
				delete child;
				root = curr->child[0];
				delete curr;
			}

			else if (curr->child[0] == child) {
				delete child;
				root = curr->child[1];
				delete curr;
			}

            heightCount--;
            return _BPLUSTREE_NEW_ROOT;
		}
	}

	uint32_t index;
	for (index = 0; index < curr->size; index++) {
		if(index < curr->size and not (lessThan(curr->key[index], key) or lessThan(key, curr->key[index]))) {
			break;
		}
	}

	for (uint32_t i = index; i < curr->size; i++) {
		curr->key[i] = curr->key[i + 1];
	}

	for (index = 0; index < curr->size + 1; index++) {
		if(curr->child[index] == child) {
			break;
		}
	}

	for (uint32_t i = index; i < curr->size + 1; i++) {
		curr->child[i] = curr->child[i + 1];
	}

	curr->size--;
	if (curr->size >= minDegree - 1) {
		return _BPLUSTREE_MODIFIED_NOT_ROOT;
	}

	if (curr == root) {
        return _BPLUSTREE_NOT_MODIFIED;
    }

	BPNode<_Tp>* par = findParent(root, curr);
	uint32_t leftSibling, rightSibling;

	for (index = 0; index < par->size + 1; index++) {
		if (par->child[index] == curr) {
			leftSibling = index - 1;
			rightSibling = index + 1;
			break;
		}
	}

	if (leftSibling >= 0) {
		BPNode<_Tp>* leftBPNode = par->child[leftSibling];

		if (leftBPNode->size >= minDegree) {
			for (uint32_t i = curr->size; i > 0; i--) {
				curr->key[i] = curr->key[i - 1];
			}

			curr->key[0] = par->key[leftSibling];
			par->key[leftSibling] = leftBPNode->key[leftBPNode->size - 1];

			for (int i = curr->size + 1; i > 0; i--) {
				curr->child[i] = curr->child[i - 1];
			}

			curr->child[0] = leftBPNode->child[leftBPNode->size];
			curr->size++;
			leftBPNode->size--;

			return _BPLUSTREE_MODIFIED_NOT_ROOT;
		}
	}

	if (rightSibling <= par->size) {
		BPNode<_Tp>* rightBPNode = par->child[rightSibling];

		if (rightBPNode->size >= minDegree) {
			curr->key[curr->size] = par->key[index];
			par->key[index] = rightBPNode->key[0];

			for (uint32_t i = 0; i < rightBPNode->size - 1; i++) {
				rightBPNode->key[i] = rightBPNode->key[i + 1];
			}

			curr->child[curr->size + 1] = rightBPNode->child[0];
			for (uint32_t i = 0; i < rightBPNode->size; ++i) {
				rightBPNode->child[i] = rightBPNode->child[i + 1];
			}

			curr->size++;
			rightBPNode->size--;
			
            return _BPLUSTREE_MODIFIED_NOT_ROOT;
		}
	}

	if (leftSibling >= 0) {
		BPNode<_Tp>* leftBPNode = par->child[leftSibling];
		leftBPNode->key[leftBPNode->size] = par->key[leftSibling];

		for (uint32_t i = leftBPNode->size + 1, j = 0; j < curr->size; j++) {
			leftBPNode->key[i] = curr->key[j];
		}

		for (uint32_t i = leftBPNode->size + 1, j = 0; j < curr->size + 1; j++) {
			leftBPNode->child[i] = curr->child[j];
			curr->child[j] = nullptr;
		}

		leftBPNode->size += curr->size + 1;
		curr->size = 0;

		return removeInternal(par->key[leftSibling], par, curr);
	}

	else if (rightSibling <= par->size) {
		BPNode<_Tp>* rightBPNode = par->child[rightSibling];
		curr->key[curr->size] = par->key[rightSibling - 1];

		for (uint32_t i = curr->size+1, j = 0; j < rightBPNode->size; j++) {
			curr->key[i] = rightBPNode->key[j];
		}

		for (uint32_t i = curr->size+1, j = 0; j < rightBPNode->size + 1; j++) {
			curr->child[i] = rightBPNode->child[j];
			rightBPNode->child[j] = nullptr;
		}

		curr->size += rightBPNode->size + 1;
		rightBPNode->size = 0;

		removeInternal(par->key[rightSibling - 1], par, rightBPNode);
	}
}

template <typename _Tp>
void BPTree<_Tp>::printNode(std::ostream& out, const BPNode<_Tp>* node, const uint32_t indent) const noexcept {
    if (node == nullptr) {
        return;;
    }

    for (uint32_t i = 0; i < indent; i++) {
		out << "\t";
	}

	for (uint32_t i = 0; i < node->size; i++) {
		printKey(node->key[i]);
        out << " ";
	}
    out << std::endl;
		
	if (not node->leaf) {
		for (uint32_t i = 0; i <= node->size; i++) {
			printNode(out, node->child[i], indent + 1);
		}
	}
}

template <typename _Tp>
constexpr uint32_t BPTree<_Tp>::num_keys(void) const noexcept {
    return keyCount;
}

template <typename _Tp>
constexpr uint32_t BPTree<_Tp>::height(void) const noexcept {
    return heightCount;
}

template <typename _Tp>
constexpr uint64_t BPTree<_Tp>::size_in_bytes(void) const noexcept {
    return keyCount * sizeof(_Tp);
}

template <typename _Tp>
void BPTree<_Tp>::insert(const _Tp key) noexcept {
	if (root == nullptr) {
		root = new BPNode<_Tp>(minDegree);
		root->key[0] = key;
		root->size++;
	}

	else {
		BPNode<_Tp>* curr = root;
		BPNode<_Tp>* par;

		while (curr->leaf == false) {
			par = curr;
			for (uint32_t i = 0; i < curr->size; i++) {
				if (lessThan(key, curr->key[i])) {
					curr = curr->child[i];
					break;
				}

				if (i == curr->size - 1) {
					curr = curr->child[i + 1];
					break;
				}
			}
		}

		if (curr->size < 2 * minDegree - 1) {
			uint32_t i = 0;
			while (lessThan(curr->key[i], key) and i < curr->size) {
                i++;
            }

			for (uint32_t j = curr->size; j > i; j--) {
				curr->key[j] = curr->key[j - 1];
			}

			curr->key[i] = key;
			curr->size++;
			curr->child[curr->size] = curr->child[curr->size - 1];
			curr->child[curr->size - 1] = nullptr;
		}

		else {
			BPNode<_Tp>* newLeaf = new BPNode<_Tp>(minDegree);

			_Tp virtualBPNode[2 * minDegree];
			for (uint32_t i = 0; i < 2 * minDegree - 1; i++) {
				virtualBPNode[i] = curr->key[i];
			}

			uint32_t i = 0, j;
			while (lessThan(virtualBPNode[i], key) and i < 2 * minDegree - 1) {
                i++;
            }

			for (uint32_t j = 2 * minDegree; j > i; j--) {
				virtualBPNode[j] = virtualBPNode[j - 1];
			}

			virtualBPNode[i] = key; 

			curr->size = minDegree;
			newLeaf->size = minDegree;

			curr->child[curr->size] = newLeaf;

			newLeaf->child[newLeaf->size] = curr->child[2 * minDegree - 1];
			curr->child[2 * minDegree - 1] = nullptr;

			for (i = 0; i < curr->size; i++) {
				curr->key[i] = virtualBPNode[i];
			}

			for(i = 0, j = curr->size; i < newLeaf->size; i++, j++) {
				newLeaf->key[i] = virtualBPNode[j];
			}

			if (curr == root) {
				BPNode<_Tp>* newRoot = new BPNode<_Tp>(minDegree);
                newRoot->leaf = false;
				newRoot->key[0] = newLeaf->key[0];
				newRoot->child[0] = curr;
				newRoot->child[1] = newLeaf;
				newRoot->size = 1;
				root = newRoot;
			}

			else {
				insertInternal(newLeaf->key[0], par, newLeaf);
			}

            heightCount++;
		}
	}

    keyCount++;
}

template <typename _Tp>
void BPTree<_Tp>::remove(const _Tp key) {
	if (root == nullptr) {
        throw std::underflow_error("_BPLUSTREE_ROOT_EMPTY");
    }

	else {
		BPNode<_Tp>* curr = root;
		BPNode<_Tp>* par;
		uint32_t leftSibling, rightSibling;

		while (not curr->leaf) {
			for(uint32_t i = 0; i < curr->size; i++) {
				par = curr;
				leftSibling = i - 1;
				rightSibling =  i + 1;

				if (lessThan(key, curr->key[i])) {
					curr = curr->child[i];
					break;
				}

				if (i == curr->size - 1) {
					leftSibling = i;
					rightSibling = i + 2;
					curr = curr->child[i + 1];
					break;
				}
			}
		}

		bool found = false;
		uint32_t index;

		for(index = 0; index < curr->size; index++) {
			if(index < curr->size and not (lessThan(curr->key[index], key) or lessThan(key, curr->key[index]))) {
				found = true;
				break;
			}
		}

		if(not found) {
			if (printKey == nullptr) {
				throw std::out_of_range("_BPLUSTREE_KEY_NOT_FOUND");
			}
			
            std::stringstream newbuf;
            std::streambuf* oldbuf = std::cout.rdbuf(newbuf.rdbuf());
            printKey(key);
            std::cout.rdbuf(oldbuf);
            throw std::out_of_range("_BPLUSTREE_KEY_NOT_FOUND: " + newbuf.str());
        }

		for(uint32_t i = index; i < curr->size; i++) {
			curr->key[i] = curr->key[i + 1];
		}

		curr->size--;
		if (curr == root) {
			for (uint32_t i = 0; i < 2 * minDegree; i++) {
				curr->child[i] = nullptr;
			}

			if (curr->size == 0) {
				delete curr;
				root = nullptr;
			}

            return;
		}

		curr->child[curr->size] = curr->child[curr->size + 1];
		curr->child[curr->size+1] = nullptr;

		if (curr->size >= minDegree) {
            return;
        }

		if (leftSibling >= 0) {
			BPNode<_Tp>* leftBPNode = par->child[leftSibling];
			if(leftBPNode->size >= minDegree + 1) {
				for(uint32_t i = curr->size; i > 0; i--) {
					curr->key[i] = curr->key[i - 1];
				}

				curr->size++;
				curr->child[curr->size] = curr->child[curr->size - 1];
				curr->child[curr->size - 1] = nullptr;
				curr->key[0] = leftBPNode->key[leftBPNode->size - 1];

				leftBPNode->size--;
				leftBPNode->child[leftBPNode->size] = curr;
				leftBPNode->child[leftBPNode->size+1] = nullptr;

				par->key[leftSibling] = curr->key[0];
				return;
			}
		}

		if (rightSibling <= par->size) {
			BPNode<_Tp>* rightBPNode = par->child[rightSibling];
			if(rightBPNode->size >= minDegree + 1) {
				curr->size++;
				curr->child[curr->size] = curr->child[curr->size - 1];
				curr->child[curr->size - 1] = nullptr;
				curr->key[curr->size - 1] = rightBPNode->key[0];

				rightBPNode->size--;
				rightBPNode->child[rightBPNode->size] = rightBPNode->child[rightBPNode->size + 1];
				rightBPNode->child[rightBPNode->size + 1] = nullptr;

				for(uint32_t i = 0; i < rightBPNode->size; i++) {
					rightBPNode->key[i] = rightBPNode->key[i + 1];
				}

				par->key[rightSibling - 1] = rightBPNode->key[0];
				return;
			}
		}

		if (leftSibling >= 0) {
			BPNode<_Tp>* leftBPNode = par->child[leftSibling];
			for(uint32_t i = leftBPNode->size, j = 0; j < curr->size; i++, j++) {
				leftBPNode->key[i] = curr->key[j];
			}

			leftBPNode->child[leftBPNode->size] = nullptr;
			leftBPNode->size += curr->size;
			leftBPNode->child[leftBPNode->size] = curr->child[curr->size];

			removeInternal(par->key[leftSibling], par, curr);
			delete curr;
		}

		else if (rightSibling <= par->size) {
			BPNode<_Tp>* rightBPNode = par->child[rightSibling];
			for (uint32_t i = curr->size, j = 0; j < rightBPNode->size; i++, j++) {
				curr->key[i] = rightBPNode->key[j];
			}

			curr->child[curr->size] = nullptr;
			curr->size += rightBPNode->size;
			curr->child[curr->size] = rightBPNode->child[rightBPNode->size];

			removeInternal(par->key[rightSibling - 1], par, rightBPNode);
			delete rightBPNode;
		}
	}

    keyCount--;
}

template <typename _Tp>
std::pair<const BPNode<_Tp>*, uint32_t> BPTree<_Tp>::search(const _Tp key) const noexcept {
	if(root == nullptr) {
        return std::pair<const BPNode<_Tp>*, uint32_t>(nullptr, 0);
    }

	BPNode<_Tp>* curr = root;
	while (not curr->leaf) {
		for (uint32_t i = 0; i < curr->size; i++) {
			if (key < curr->key[i]) {
				curr = curr->child[i];
				break;
			}

			if (i == curr->size - 1) {
				curr = curr->child[i + 1];
				break;
			}
		}
	}

	for (uint32_t i = 0; i < curr->size; i++) {
		if(i < curr->size and not (lessThan(key, curr->key[i]) or lessThan(curr->key[i], key))) {
			return std::pair<const BPNode<_Tp>*, uint32_t>(curr, i);
		}
	}

    return std::pair<const BPNode<_Tp>*, uint32_t>(nullptr, 0);
}

template <typename _Tp>
_Tp BPTree<_Tp>::searchKey(const _Tp key) const {
    std::pair<const BPNode<_Tp>*, uint32_t> node = search(key);
    if (node.first == nullptr) {
		if (printKey == nullptr) {
			throw std::out_of_range("_BPLUSTREE_KEY_NOT_FOUND");
		}
		
        std::stringstream newbuf;
        std::streambuf* oldbuf = std::cout.rdbuf(newbuf.rdbuf());
        printKey(key);
        std::cout.rdbuf(oldbuf);
        throw std::out_of_range("_BPLUSTREE_KEY_NOT_FOUND: " + newbuf.str());
    }

    return node.first->key[node.second];
}

template <typename _Tp>
std::ostream& operator<<(std::ostream& out, const BPTree<_Tp>& bptree) {
    if (bptree.printKey != nullptr and bptree.root != nullptr) {
        out << std::endl;
        bptree.printNode(out, bptree.root, 0);
        out << std::endl;
    }

    return out;
}
