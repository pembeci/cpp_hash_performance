#include <stdio.h>
#include <tchar.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <chrono>
#include <iomanip>  // setw etc. in cout

using namespace std;

typedef unordered_map<string, int> user_map;

user_map key_hash;
unordered_set<string> test_POS;
unordered_set<string> test_NEG;

unsigned FULL_SIZE,   // total keys in hash
POS_SIZE,         // total keys existing in hash that will be tested 
NEG_SIZE;         // total keys not existing in hash that will be tested

void fill_hash_from_file(string filename, float ratio = 1.0) {
	/*
	Reads the files that are generated by GenHashTest and inserts the keys into the hash test sets.
	Call with ratio less than 1 just to run on a portion of the file while developing the code.
	*/
	ifstream file(filename);
	cout << "ratio: " << ratio << "   filename:" << filename << endl;
	if (file.is_open()) {
		unsigned pos, neg, full;
		unsigned pos_r, neg_r, full_r;
		file >> pos >> neg >> full;
		pos_r = pos*ratio; 
		neg_r = neg*ratio;
		full_r = full*ratio;
		// cout << pos << ":" << neg << ":" << full << endl;
		cout << "Hash size: " << full << " (testing with " << full_r << ")" << endl;
		cout << "POS test set size: " << pos << " (testing with " << pos_r << ")" << endl;
		cout << "NEG test set size: " << neg << " (testing with " << neg_r << ")" << endl;		
		string line;
		getline(file, line);
		unsigned line_no = 0;
		while (getline(file, line)) {
			if (line_no < pos) {  // positive test cases
				if (line_no < pos_r) {
					test_POS.insert(line);
					key_hash[line] = 1;
				}
			}
			else if (line_no < pos + neg) {  // negative test cases
				if (line_no < pos + neg_r) {
					test_NEG.insert(line);
			    }
			}
			else {  // other elements in hash (not a positive test case)
				if (key_hash.size() < full_r) {
					key_hash[line] = 1;
				}
				else {
					break;
				}
			}
			line_no++;
		}
		file.close();
	}
	else {
		cout << "File couldn't be opened." << endl;
	}
}

int main(int argc, char *argv[]) {
	string file_name = "keys_2M_L10-70.txt";
	unsigned min_key_length = 10;
	unsigned max_key_length = 70;

	unordered_map<int, double> total_time_POS;
	unordered_map<int, double> total_time_NEG;
	unordered_map<int, int> key_count_POS;
	unordered_map<int, int> key_count_NEG;
	for (int i = min_key_length; i <= max_key_length; i++) {  // init total times and counts for each key length value
		// for each key length i, initialize time and key counts
		total_time_POS[i] = 0;
		total_time_NEG[i] = 0;
		key_count_POS[i] = 0;
		key_count_NEG[i] = 0;
	}

	auto t1 = chrono::high_resolution_clock::now();
	// fill_hash_from_file(file_name, 0.001);
	fill_hash_from_file(file_name);
	auto t2 = chrono::high_resolution_clock::now();
	cout << "Hash and test sets creation: " << chrono::duration_cast<chrono::milliseconds>(t2 - t1).count() / 1000.0 << " seconds" 
		  << endl << endl;
	/* printf("Length of hash: %d\n", key_hash.size());
	printf("Length of positive test cases: %d\n", test_POS.size());
	printf("Length of negative test cases: %d\n", test_NEG.size()); */
	
	// test positives, keys found in the hash
	double total_time1 = 0;
	// int i = 0;
	uint64_t time_diff;
	for (const string& key : test_POS) {
		t1 = chrono::high_resolution_clock::now();
		auto it = key_hash.find(key);
		t2 = chrono::high_resolution_clock::now();
		time_diff = chrono::duration_cast<chrono::nanoseconds>(t2 - t1).count();
		total_time1 += time_diff;
		// update time spent and total key counts for this key's length
		total_time_POS[key.length()] += time_diff;
		key_count_POS[key.length()]++;
		if (it == key_hash.end()) {
			cout << "ERROR. key " << key << " should be in hash." << endl;
			return 0;
		}
		/* if (i <= 100) {
			cout << chrono::duration_cast<chrono::microseconds>(t2 - t1).count() << endl;
			i++;
		} */
	}
	cout << "POS test time total: " << total_time1 / 1e9 << " seconds" << endl;
	cout << "POS test time average lookup: " << (total_time1 / 1e3) / test_POS.size() << " microseconds" << endl;

	// test negatives, keys not found in the hash
	double total_time2 = 0;
	for (const string& key : test_NEG) {
		t1 = chrono::high_resolution_clock::now();
		auto it = key_hash.find(key);
		t2 = chrono::high_resolution_clock::now();
		time_diff = chrono::duration_cast<chrono::nanoseconds>(t2 - t1).count();
		total_time2 += time_diff;
		total_time_NEG[key.length()] += time_diff;
		key_count_NEG[key.length()]++;
		if (it != key_hash.end()) {
			cout << "ERROR. key " << key << " should NOT be in hash." << endl;
			return 0;
		}
	}
	cout << "NEG test time total: " << total_time2 / 1e9 << " seconds" << endl;
	cout << "NEG test time average lookup: " << (total_time2 / 1e3) / test_NEG.size()  << " microseconds" << endl << endl;

	cout << "Average lookup together: " << ((total_time1 + total_time2) / 1e3) / (test_POS.size() + test_NEG.size()) << endl << endl;

	double avg;
	cout << "Lookup times per key length: " << endl;
	cout << "Key Length    POS_Avg      NEG_Avg         Key Total" << endl;
	cout << "----------   ---------    ---------       -----------" << endl;
	for (int i = min_key_length; i <= max_key_length; i++) {
		cout << right << setw(10) << setfill(' ') << i << "    ";
		avg = (total_time_POS[i] / key_count_POS[i]) / 1e3;
		cout << right << setw(7) << setfill(' ') << avg << "      ";
		avg = (total_time_NEG[i] / key_count_NEG[i]) / 1e3;
		cout << right << setw(7) << setfill(' ') << avg << "          ";
		cout << right << setw(9) << setfill(' ') << (key_count_POS[i]+ key_count_NEG[i]) << endl ;
	}
	cout << "Press enter to continue...";
	getchar();
	return 0;
}