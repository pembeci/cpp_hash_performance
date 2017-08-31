// TestHash1.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include <tchar.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <stdlib.h>     /* srand, rand */
#include <unordered_map>
#include <unordered_set>
#include <chrono>
#include <math.h> 
#include <algorithm> /* max */

using namespace std;

typedef unordered_map<string, int> user_map;

void random_string(string& s, const int len) {
	/* there may be a faster approach but since we are not benchmarking key creation I just used this which seems fast enough */
	static const char alphanum[] =    // allowed chars in key
		"0123456789"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"aaaaaaaeeeeeeeiiiiiioooouuuu"   // vowels higher probability
		"aaaaaaaeeeeeeeiiiiiioooouuuu"
		"bcdfghjklmnpqrstvwxyz"          // more lowercase than uppercase
		"bcdfghjklmnpqrstvwxyz"
		"bcdfghjklmnpqrstvwxyz"
		".,!%&";

	for (int i = 0; i < len; ++i) {
		s.push_back(alphanum[rand() % (sizeof(alphanum) - 1)]);
	}
}

user_map single_hash;
unordered_set<string> test_POS;
unordered_set<string> test_NEG;

unsigned FULL_SIZE,   // total keys in hash
	POS_SIZE,         // total keys existing in hash that will be tested 
	NEG_SIZE;         // total keys not existing in hash that will be tested

unsigned min_key_length;
unsigned max_key_length;

void fill_hash() {
	/* fills the hash table by producing random strings. at the same time generates the test cases. */
	unsigned log_every = 50000;
	unsigned threshhold = log_every;
	while (single_hash.size() < FULL_SIZE) {
		string new_str = "";
		if (min_key_length == max_key_length) random_string(new_str, max_key_length);
		else random_string(new_str, (rand() % (max_key_length - min_key_length + 1)) + min_key_length);
		if (test_NEG.find(new_str) == test_NEG.end()) {  // make sure new key is not in the negative test cases
			single_hash[new_str] = 1;
			if (single_hash.size() == FULL_SIZE) break;
		}
		if (new_str.empty()) {
			continue;
		}
		
		// now generate some keys sharing the same prefix to mimic real usernames
		for (int i = 0; i < 10; i++) {
			int suffix_len = 1 + rand() % (new_str.length() / 2); // how many chars to change
			new_str.erase(new_str.length() - suffix_len, -1);
			random_string(new_str, suffix_len); // change the last suffix_len chars	
			if (single_hash.find(new_str) != single_hash.end()) // already in hash
				continue;
			if (test_POS.size() < POS_SIZE && rand() % 2 == 0 && test_NEG.find(new_str) == test_NEG.end()) {
				single_hash[new_str] = 1;
				test_POS.insert(new_str);
				if (single_hash.size() == FULL_SIZE) break;
			}
			else if (test_NEG.size() < NEG_SIZE && rand() % 2 == 0) {
				test_NEG.insert(new_str);
			}
		}
		if (single_hash.size() > threshhold) {
			cout << single_hash.size() << " keys created" << endl;
			threshhold += log_every;
		}
	}
	// probably not necessary but let's make sure test sets are filled since we are picking test cases randomly 
	while (test_POS.size() < POS_SIZE) {
		// iterate over hash and fill the rest
		for (auto& kv : single_hash) {
			test_POS.insert(kv.first);  // no need to check for existence since this is a test
			if (test_POS.size() >= POS_SIZE) break;
		}
	}
	while (test_NEG.size() < NEG_SIZE) {
		// generate new random strings and fill
		string new_str = "";
		if (min_key_length == max_key_length) random_string(new_str, max_key_length);
		else random_string(new_str, (rand() % (max_key_length - min_key_length + 1)) + min_key_length);
		printf("New string %s", new_str);
		if (single_hash.find(new_str) == single_hash.end()) {  // not found in hash so we can add as a negative test case
			test_NEG.insert(new_str);
		}
	}
}


void read_file_and_test(string filename) {
	ifstream file(filename); 
	if (file.is_open()) {
		int pos, neg, full;
		file >> pos >> neg >> full;
		cout << pos << ":" << neg << ":" << full << endl;
		string line;
		getline(file, line);
		int line_no = 0;
		while (getline(file, line)) {
			if (line_no < pos) {  // positive test cases
				if (single_hash.find(line) == single_hash.end() || test_POS.find(line) == test_POS.end()) {
					cout << "ERROR. Should be in POS and Hash: " << line << endl;
					return;
				}
			}
			else if (line_no < pos+neg) {  // negative test cases
				if (single_hash.find(line) != single_hash.end() || test_NEG.find(line) == test_NEG.end()) {
					cout << "ERROR. Should be in NEG and not in Hash: " << line << endl;
					return;
				}
			}
			else {  // other elements in hash (not a positive test case)
				if (single_hash.find(line) == single_hash.end() || test_POS.find(line) != test_POS.end()) {
					cout << "ERROR. Should be in HASH and not in POS: " << line << endl;
					return;
				}
			}
			line_no++;
		}		
		file.close();
	}
}

int main(int argc, char *argv[]) {
	if (argc == 1) {
		cout << "Please give an output filename as a command argument." << endl;
		return 0;
	}
	srand(42); // random seed to generate consistent strings. change 42 to some other number for different randomization
	string response;
	cout << "Min key length: ";
	getline(cin, response); stringstream(response) >> min_key_length;
	cout << "Max key length (" << min_key_length << "): ";
	getline(cin, response); 
	if (response.empty()) max_key_length = min_key_length; 
	else stringstream(response) >> max_key_length;
	if (max_key_length < min_key_length) {
		cout << "Max should be greater than min ..." << endl;
		return 0;
	}
	cout << "Total unique keys in hash: ";
	getline(cin, response); stringstream(response) >> FULL_SIZE;
	cout << "Total of positive tests - i.e. key exists in hash: ";
	getline(cin, response); stringstream(response) >> POS_SIZE;
	if (POS_SIZE > FULL_SIZE) {
		cout << "POS_SIZE should be less than FULL_SIZE ..." << endl;
		return 0;
	}
	cout << "Total of negative tests - i.e. key doesn't exist in hash (" << POS_SIZE << "): ";
	getline(cin, response);
	if (response.empty()) NEG_SIZE = POS_SIZE;
	else stringstream(response) >> NEG_SIZE;
	fill_hash();
	string file_name = argv[1];
	ofstream file(file_name); // caution: overrides
	if (file.is_open())	{
		file << POS_SIZE << " " << NEG_SIZE << " " << FULL_SIZE;
		for (auto& key : test_POS) {
			file << endl;
			file << key;
		}
		for (auto& key : test_NEG) {
			file << endl;
			file << key;
		}
		for (auto& kv : single_hash) {
			if (test_POS.find(kv.first) == test_POS.end()) {
				file << endl;
				file << kv.first;
			}
		}
		file.close();
		// read_file_and_test(file_name);
	}
	else cout << "Unable to open file";
	return 0;
}

