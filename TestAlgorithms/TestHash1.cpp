// TestHash1.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
// Modified from: https://github.com/kephir4eg/trie
// #include "trie.h"   
// to use boost library in VS: http://www.boost.org/doc/libs/1_65_0/more/getting_started/windows.html#build-from-the-visual-studio-ide
// #include <boost/unordered_map.hpp>  // boost version 1.65

// boost and std unordered_map implementations have the same API so it is enough to change the typedef to test one of them
typedef std::unordered_map<std::string, int> user_map;
// typedef boost::unordered_map<std::string, int> user_map;
// typedef trie::trie_map<char, int> user_map_trie;

void random_string(char *s, const int len) {
	/* there may be a faster approach but since we are not benchmarking key creation I just used this which seems fast enough */
	static const char alphanum[] =    // allowed chars in key
		"0123456789"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz"
		".,!*%";

	for (int i = 0; i < len; ++i) {
		s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
	}
	s[len] = 0;
}

user_map single_hash;
// user_map_trie my_trie;
std::unordered_set<std::string> test_POS;
std::unordered_set<std::string> test_NEG;
unsigned FULL_SIZE = 5000,   // total keys in hash
	POS_SIZE = 100,          // total keys existing in hash that will be tested 
	NEG_SIZE = POS_SIZE;        // total keys not existing in hash that will be tested 


void fill_hash() {
	/* fills the hash table by producing random strings. at the same time generates the test cases. */
	unsigned log_every = 50000;
	unsigned threshhold = log_every;
	while (single_hash.size() < FULL_SIZE) {
		char new_str[21];
		// random_string(new_str, rand() % 15 + 6); // key length is random between 6-20
		random_string(new_str, 20); // string size 20
		if (test_NEG.find(new_str) == test_NEG.end()) {  // make sure new key is not in the negative test cases
			single_hash[new_str] = 1;
		}
		// now generate some keys sharing the same prefix to mimic real usernames
		for (int i = 0; i < 9; i++) {
			int suffix_len = 1 + rand() % 4; // how many chars to change: 1-4
			random_string(new_str + strlen(new_str) - suffix_len, suffix_len); // change the last suffix_len chars	
			if (single_hash.find(new_str) != single_hash.end()) // already in hash
				continue; 
			if (test_POS.size() < POS_SIZE && rand() % 4 == 0 && test_NEG.find(new_str) == test_NEG.end()) {
				single_hash[new_str] = 1;
				test_POS.insert(new_str);
			}
			else if (test_NEG.size() < NEG_SIZE && rand() % 4 == 0) {
				test_NEG.insert(new_str);
			}
		}
		if (single_hash.size() > threshhold) {
			std::cout << single_hash.size() << " keys created" << std::endl;
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
		char new_str[21];
		random_string(new_str, rand() % 15 + 6); 		
		printf("New string %s", new_str);
		if (single_hash.find(new_str) == single_hash.end()) {  // not found in hash so we can add as a negative test case
			test_NEG.insert(new_str);
		}
	}
}



int main(int argc, char *argv[]) {
	srand(42); // random seed to generate consistent strings
	fill_hash();
	std::string file_name = argv[1]; 
	std::ofstream file (file_name); // caution: overrides
	bool first_line = true;
	if (file.is_open())
	{
		for (auto& kv : single_hash) {
			if (!first_line) file << std::endl;
			else first_line = false;
			file << kv.first;			
		}
		file.close();
	}
	else std::cout << "Unable to open file";
	return 0;
}

int main1()
{
	srand(42); // random seed to generate consistent strings
	auto t1 = std::chrono::high_resolution_clock::now();
	fill_hash();
	auto t2 = std::chrono::high_resolution_clock::now();
	std::cout << "Key creation: " << std::chrono::duration_cast<std::chrono::milliseconds>(t2-t1).count() / 1000.0 << " seconds" << std::endl;
	printf("Length of hash: %d\n", single_hash.size());
	printf("Length of positive test cases: %d\n", test_POS.size());
	printf("Length of negative test cases: %d\n", test_NEG.size());

	/*
	// test positives, keys found in the hash
	t1 = std::chrono::high_resolution_clock::now();
	for (const std::string& key : test_POS) {
		if (single_hash.find(key) == single_hash.end()) {
			printf("ERROR. This key should be in hash.");
		}
	}
	t2 = std::chrono::high_resolution_clock::now();
	std::cout << "POS test time: " << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count() / 1000.0 << " seconds" << std::endl;
	*/
	double total_time = 0;
	int i = 0;
	for (const std::string& key : test_POS) {
		t1 = std::chrono::high_resolution_clock::now();
		single_hash.find(key);
		t2 = std::chrono::high_resolution_clock::now();
		total_time += std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count();
		if (i <= 100) {
			std::cout << std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count() << std::endl;
			i++;
		}
	}
	std::cout << "POS test time: " << total_time / 1e9 << " seconds" << std::endl;

	// test negatives, keys not found in the hash
	t1 = std::chrono::high_resolution_clock::now();
	for (const std::string& key : test_NEG)  {
		if (single_hash.find(key) != single_hash.end()) {
			printf("ERROR. This key should not be in hash.");
		}
	}
	t2 = std::chrono::high_resolution_clock::now();
	std::cout << "NEG test time: " << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count() / 1000.0 << " seconds" << std::endl;
	// Comment out the following and trie related definitions above to test radix tree approach
	/*
	std::cout << "Constructing trie..." << std::endl;
	t1 = std::chrono::high_resolution_clock::now();
	for (auto& kv : single_hash) {
		my_trie.insert(kv.first, kv.second);
	}
	t2 = std::chrono::high_resolution_clock::now();
	std::cout << "Trie constructed: " << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count() / 1000.0 << " seconds" << std::endl;

	// test positives, keys found in the hash
	t1 = std::chrono::high_resolution_clock::now();
	for (const std::string& key : test_POS) {
		if (!my_trie.contains(key)) {
			printf("ERROR. This key should be in hash.");
		}
	}
	t2 = std::chrono::high_resolution_clock::now();
	std::cout << "POS trie test time: " << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count() / 1000.0 << " seconds" << std::endl;

	// test negatives, keys not found in the hash
	t1 = std::chrono::high_resolution_clock::now();
	for (const std::string& key : test_NEG) {
		if (my_trie.contains(key)) {
			printf("ERROR. This key should not be in hash.");
		}
	}
	t2 = std::chrono::high_resolution_clock::now();
	std::cout << "NEG trie test time: " << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count() / 1000.0 << " seconds" << std::endl;
	*/

	return 0;
}

