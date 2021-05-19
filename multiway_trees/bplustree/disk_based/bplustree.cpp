#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <numeric>
#include <algorithm>
#include <cmath>
#include <chrono>
#include <fstream>
using namespace std;

const string FOLDER("temp/");
const string INIT_FILE("tree_data/init_data.txt");
const string QUERY_FILE("tree_data/query_data.txt");
const uint32_t MAX_KEYS = 512;

uint32_t nodeNoGlobalCounter = 1;
uint32_t objectNoGlobal = 1;
uint32_t Root = 1;

uint64_t pointQuery = 0;
uint64_t insertQuery = 0;
uint64_t rangeQuery = 0;
uint64_t invalidQuery = 0;
uint64_t diskReads = 0;
uint64_t diskWrites = 0;
uint64_t diskReadsQueries = 0;
uint64_t diskWritesQueries = 0;

chrono::_V2::system_clock::time_point startTime;
chrono::_V2::system_clock::time_point endTime;

bool isFirst = true;

map<uint32_t, uint32_t> parent;

vector<double> statsInsert;
vector<double> statsPoint;
vector<double> statsRange;

class BPTree {
	public:
		uint32_t leaf;
		uint32_t nodeNo;

		vector <double> keys;
		vector <uint32_t> children;

		int previous;
		int next;

		BPTree(uint32_t tempNodeNo, uint32_t mleaf = 1)
			: nodeNo(tempNodeNo), leaf(mleaf),
			previous(-1), next(-1) {}

		BPTree (uint32_t tempNodeNo, vector<double>& tempKeys, vector<uint32_t>& tempChildren)
			: nodeNo(tempNodeNo), leaf(1), keys(tempKeys), children(tempChildren),
			previous(-1), next(-1) {}

		BPTree(uint32_t tempNodeNo, uint32_t mleaf, vector<double>& tempKeys, vector<uint32_t>& tempChildren, int tempPrevious, int tempNext)
			: nodeNo(tempNodeNo), leaf(mleaf), keys(tempKeys), children(tempChildren),
			previous(tempPrevious), next(tempNext) {}
};

void benchmark(string header, vector<double>& timeVector) {
	double sum = accumulate(timeVector.begin(), timeVector.end(), 0.0);
	double mean = sum / timeVector.size();
	double squaredSum = inner_product(timeVector.begin(), timeVector.end(), timeVector.begin(), 0.0);
	double standardDev = sqrt(squaredSum / timeVector.size() - mean * mean);

	cout << header << ":\n";
	cout << "\t\t1) Minimum Time:\t\t" << *min_element(timeVector.begin(), timeVector.end()) << " ms\n";
	cout << "\t\t2) Maximum Time:\t\t" << *max_element(timeVector.begin(), timeVector.end()) << " ms\n";
	cout << "\t\t3) Average Time:\t\t" << mean << " ms\n";
	cout << "\t\t4) Standard Deviation:\t\t" << standardDev << " ms\n";
}

void pointRead(uint32_t fname) {
	string line;
	ifstream inFile(FOLDER + "data_" + to_string(fname));

	while (getline(inFile, line)) {
		//cout<< line << endl;
		continue;
	}

	inFile.close();
}

void rangeRead(vector<uint32_t>& files) {
	for (uint32_t i = 0; i < files.size(); i++) {
		pointRead(files[i]);
	}
}

uint32_t NewObjectFile(string data) {			
	uint32_t fname = objectNoGlobal++;
	string inPath = FOLDER + "data_" + to_string(fname);

	ofstream out(inPath);
	out << data;
	out.close();

	return fname;
}

void Write(BPTree* tree) {
	ofstream out(FOLDER + to_string(tree->nodeNo));	
	out.write((char*) &tree->leaf, sizeof(tree->leaf));
	
	uint32_t tempSize = tree->keys.size();
	uint32_t i = 0;
	out.write((char*) &tempSize, sizeof(tempSize));
		
	for (i = 0; i < tempSize; i++) {
		out.write((char*) &tree->keys[i], sizeof(tree->keys[i]));
	}

	out.write((char*) &tree->previous, sizeof(tree->previous));	
	out.write((char*) &tree->next, sizeof(tree->next));	

	tempSize = tree->children.size();			
	out.write((char*) &tempSize, sizeof(tempSize));
		
	for (i = 0; i < tempSize; i++) {
		out.write((char*) &tree->children[i], sizeof(tree->children[i]));
	}

	out.close();
	diskWrites++;
	diskWritesQueries++;
}

BPTree* Read(string inPath, uint32_t fName) {
	vector<double> keys;
	vector<uint32_t> children;

	uint32_t leaf, child;
	int previous, next;
	double key = 0;

	uint32_t tempSize, i;

	ifstream look(inPath);

	look.read((char*) &leaf, sizeof(leaf));
	look.read((char*) &tempSize, sizeof(tempSize));

	for (i = 0 ; i < tempSize ; i++ ) {
		look.read((char*) &key, sizeof(key));
		keys.push_back(key);
	}

	look.read((char*) &previous, sizeof(previous));	
	look.read((char*) &next, sizeof(next));
	look.read((char*) &tempSize, sizeof(tempSize));

	for (i = 0 ; i < tempSize ; i++ ) {
		look.read((char*) &child, sizeof(child));
		children.push_back(child);
	}

	look.close();

	BPTree* node = new BPTree(fName, leaf, keys, children, previous, next);	
	diskReads++;
	diskReadsQueries++;

	return node;
}

BPTree* Find(double key) {
	BPTree* current = Read(FOLDER + to_string(Root), Root);
	uint32_t nodeName = 0;
	uint32_t i = 0;

	while (not current->leaf) {
		for (i = 0; i < current->keys.size(); i++) {
			if (current->keys[i] > key) {
				break;
			}
		}

		nodeName = current->children[i];
		parent[nodeName] = current->nodeNo;
		delete current;
		current = Read(FOLDER + to_string(nodeName), nodeName);
	}

	return current;	
}

void insertLeaf(BPTree* leafNode, double Value, uint32_t obj) {
	vector<double>::iterator insertKey = leafNode->keys.begin();
	vector<uint32_t>::iterator insertChildren = leafNode->children.begin();

	if (Value >= leafNode->keys[0]) {
		uint32_t vSize = leafNode->keys.size();
		uint32_t index;

		for (index = 0; index < vSize - 1; index++) {
			if (leafNode->keys[index + 1] > Value) {
				break;
			}
		}

		insertKey += index + 1;
		insertChildren += index + 1;
	}
	
	leafNode->keys.insert(insertKey, Value);	
	leafNode->children.insert(insertChildren, obj);	
}

void insertParent(BPTree* current, double Value, uint32_t obj) {
	if (current->nodeNo == Root) {
		BPTree* tempNode = new BPTree(nodeNoGlobalCounter, 0);
		Root = nodeNoGlobalCounter++;

		tempNode->children.push_back(current->nodeNo);
		tempNode->children.push_back(obj);
		tempNode->keys.push_back(Value);
		Write(tempNode);

		delete tempNode;
		delete current;
	}

	else {
		uint32_t tempParent = parent[current->nodeNo];
		uint32_t nodeNo = current->nodeNo;

		delete current;
		current = Read(FOLDER + to_string(tempParent), tempParent);
		bool done = false;

		for (uint32_t i = 0; i < current->children.size(); i++) {
			if (current->children[i] == nodeNo) {
				current->children.insert(current->children.begin() + i + 1, obj);
				current->keys.insert(current->keys.begin() + i , Value);
				done = true;
				break;
			}
		}
			
		if (current->keys.size() <= MAX_KEYS) {
			Write(current);
			delete current;
		}

		else {
			BPTree* tempNode = new BPTree(nodeNoGlobalCounter++, 0);

			vector<uint32_t> children(make_move_iterator(current->children.begin() + (current->children.size() + 1) / 2), make_move_iterator(current->children.end()));
			current->children.erase(current->children.begin() + (current->children.size() + 1) / 2, current->children.end());
			tempNode->children = children;
			children.clear();

			double nKey = current->keys[current->keys.size() / 2];

			vector<double> keys(make_move_iterator(current->keys.begin() + current->keys.size() / 2 + 1), make_move_iterator(current->keys.end()));
			current->keys.erase(current->keys.begin() + (current->keys.size()) / 2, current->keys.end());
			tempNode->keys = keys;
			keys.clear();

			Write(tempNode);
			Write(current);

			uint32_t nObj = tempNode->nodeNo;
			delete tempNode;
			insertParent(current, nKey, nObj);
		}
	}
}

void Insert(double Value, uint32_t obj) {
	parent.clear();

	if (isFirst) {
		BPTree* current = new BPTree(nodeNoGlobalCounter);
		Root = nodeNoGlobalCounter++;

		current->children.push_back(obj);	
		current->keys.push_back(Value);					
		Write(current);

		delete current;
		isFirst = false;
	}

	else {
		BPTree* current = Find(Value);

		if (current->keys.size() >= MAX_KEYS) {
			insertLeaf(current, Value, obj);
			BPTree* tempNode = new BPTree(nodeNoGlobalCounter++, 1);
				
			tempNode->previous = current->nodeNo;

			if (current->next != -1) {
				BPTree* temp = Read(FOLDER + to_string(current->next), current->next);
				temp->previous = tempNode->nodeNo;
				Write(temp);
				delete temp;
			}

			vector<uint32_t> children(make_move_iterator(current->children.begin() + (current->children.size() + 1)  /2), make_move_iterator(current->children.end()));
			current->children.erase(current->children.begin() + (current->children.size() + 1) / 2, current->children.end());
			tempNode->children = children;
			children.clear();

			vector<double> keys(make_move_iterator(current->keys.begin() + (current->keys.size() + 1) / 2), make_move_iterator(current->keys.end()));
			current->keys.erase(current->keys.begin() + (current->keys.size() + 1) / 2, current->keys.end());
			tempNode->keys = keys;

			tempNode->next = current->next;
			current->next = tempNode->nodeNo;
			keys.clear();

			Write(tempNode);
			uint32_t newObject = tempNode->nodeNo;
			double newValue = tempNode->keys[0];

			delete tempNode;
			Write(current);
			insertParent(current, newValue, newObject);

		}

		else {
			insertLeaf(current, Value, obj);
			Write(current);
			delete current;
		}		
	}
}

uint32_t PointQuery(double key) {
	BPTree* current = Find(key);
	bool found = false;
	uint32_t i = 0;

	for (i = 0; i < current->keys.size(); i++) {
		if (current->keys[i] == key) {
			found = true;
			break;
		}
	}

	if (found) {
		uint32_t res = current->children[i];
		delete current;
		return res;
	}

	delete current;
	return 0;
}

void RangeQuery(double point, double range, vector<uint32_t>& result) {
	BPTree* current = Find(point + range);
	BPTree* next = nullptr;

	bool done = false;
	double lower = point - range;
	double higher = point + range;

	while (not done) {
		for (vector<double>::reverse_iterator temp = current->keys.rbegin(); temp != current->keys.rend(); temp++) {
			if (*temp < lower) {
				done = true;
				break;
			}

			if (*temp <= higher) {
				result.push_back(current->children[current->keys.rend() - temp - 1]);
			}
		}

		if (not done) {
			if (current->previous == -1) {
				break;
			}

			next = Read(FOLDER + to_string(current->previous), current->previous);
			delete current;
			current = next;
		}

		else { 
			break;
		}
	}

	delete current;
}

int main(void) {
	cout << "Max Keys: " << MAX_KEYS << endl;

	ifstream iFile(INIT_FILE);
	if (not iFile.is_open()) {
		cerr << "Initialization of the tree should be done from: " << INIT_FILE << endl;
		return 1;
	}

	int qCode,inserted=0;
	double key;
	string data;

	cout << "Tree creation in progress .....................\n";
	while (true) {
		iFile >> key;

		if (iFile.eof()) {
			break;
		}
		iFile >> data;

		using namespace std::chrono;
		startTime = high_resolution_clock::now();
		Insert(key, NewObjectFile(to_string(key) + "\t" + data));
		endTime = high_resolution_clock::now();

		statsInsert.push_back((double) duration_cast<milliseconds>(endTime - startTime).count());
		inserted++;
	}

	iFile.close();
	cout << "Tree creation successful ..........................\n";
	cout << "Queries processing from file: " << QUERY_FILE << endl;

	ifstream queries(QUERY_FILE);
	if (not queries.is_open()) {
		cerr << "Queries should be read from: " << QUERY_FILE << endl;
		return 1;
	}

	diskWritesQueries = 0;
	diskReadsQueries = 0;

	while (true) {
		queries >> qCode;
		double pt1;

		if (queries.eof()) {
			break;
		}
		queries >> pt1;

		if (qCode == 0) {
			queries >> data;

			using namespace std::chrono;
			startTime = high_resolution_clock::now();
			Insert(pt1, NewObjectFile(to_string(pt1) + "\t" + data));
			endTime = high_resolution_clock::now();

			statsInsert.push_back((double) duration_cast<milliseconds>(endTime - startTime).count());
			insertQuery++;

			//cout << "Inserted\n";
		}

		else if (qCode == 1) {
			using namespace std::chrono;
			startTime = high_resolution_clock::now();
			uint32_t temp = PointQuery(pt1);
			endTime = high_resolution_clock::now();

			statsPoint.push_back((double) duration_cast<milliseconds>(endTime - startTime).count());
			if (temp != 0) {
				//cout << pt1 << ": Found\n";
				pointRead(temp);
			}

			/*else {
				cout << "Not Found\n" << qCode << "\t" << pt1 << endl;
			}*/

			pointQuery++;
		}

		else if (qCode == 2) {
			double Range;
			queries >> Range;

			/*cout << "Range Query:\n";
			cout << "Point: " << pt1 <<  " Range: " << Range << endl;*/

			vector<uint32_t> result;

			using namespace std::chrono;
			startTime = high_resolution_clock::now();
			RangeQuery(pt1, Range, result);
			rangeRead(result);
			endTime = high_resolution_clock::now();

			statsRange.push_back((double) duration_cast<milliseconds>(endTime - startTime).count());
			rangeQuery++;
		}

		else {
			//cout << "Query Code " << qCode << " not supported\n";
			invalidQuery++;
		}
		
		//cout << endl;
	}

	cout << "\n-----------------------------------------\n\nBENCHMARK RESULTS:\n";
	
	benchmark("\n\tInsertion Queries", statsInsert);
	cout << "\n\t\tNumber of insertion Queries:\t" << insertQuery << endl;

	benchmark("\n\tPoint Queries", statsPoint);
	cout << "\n\t\tNumber of point Queries:\t" << pointQuery << endl;

	benchmark("\n\tRange Queries", statsRange);
	cout << "\n\t\tNumber of range Queries:\t" << rangeQuery << endl;

	cout << "\n\n\tNumber of invalid Queries:\t\t" << invalidQuery<< endl;
	cout << "\n\tTotal Number of disks Reads:\t\t" << diskReads << endl;
	cout << "\tTotal Number of disks Writes:\t\t" << diskWrites << endl;
	cout << "\tTotal Number of disk Access:\t\t" << diskReads + diskWrites << endl;
	cout << "\tNumber of disks Reads in queries:\t" << diskReadsQueries << endl;
	cout << "\tNumber of disks Writes in queries:\t" << diskWritesQueries << endl;
	cout << "\tNumber of disk Access in queries:\t" << diskReadsQueries + diskWritesQueries << endl;

    return 0;
}
