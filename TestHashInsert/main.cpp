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

unsigned FULL_SIZE,   // total keys in hash
POS_SIZE,         // total keys existing in hash that will be tested 
NEG_SIZE;         // total keys not existing in hash that will be tested

void fill_hash_from_file(string filename, float ratio = 1.0) {
	/*
	   Reads the files that are generated by GenHashTest and inserts the keys into the hash.
	   Measures insert times and outputs to an output file. 
	   Call with ratio less than 1 just to run on a portion of the file while developing the code.
	*/
	ifstream file(filename);   
	ofstream outfile(filename + "_times.txt");   // output file
	if (!outfile.is_open()) {
		cout << "Couldn't open output file." << endl;
		return;
	}
	unsigned total_data_points = 100;  // how many points we want in the graph
	unsigned log_every;                // we will collect data every `log_every` inserts

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
		cout << "NEG test set size: " << neg << " (testing with " << neg_r << ")" << endl << endl;	
		
		// i.e. if we have 1000000 keys and want 400 data points then every 2500 inserts we will measure time spent
		log_every = full_r / total_data_points;  

		string line;
		getline(file, line);
		unsigned line_no = 0;
		unsigned inserted = 0;
		uint64_t insert_time = 0;
		uint64_t total_time = 0;
		auto t1 = chrono::high_resolution_clock::now();
		auto t2 = chrono::high_resolution_clock::now();
		cout << "Logging every " << log_every << " key inserts" << endl << endl;
		while (getline(file, line)) {
			if (line_no < pos_r || (line_no >= pos + neg && key_hash.size() < full_r)) {  // keys in hash 
				t1 = chrono::high_resolution_clock::now();
				key_hash[line] = 1;
				t2 = chrono::high_resolution_clock::now();
				insert_time += chrono::duration_cast<chrono::nanoseconds>(t2 - t1).count();
				inserted++;
				if (inserted % log_every == 0) {   // log_every keys inserted so time to output
					total_time += insert_time;
					// each row has: how many keys inserted, how much total time spent for this batch, cumulative time from beginning
					outfile << inserted << "\t" << insert_time / (log_every * 1e3) << "\t" << (total_time / 1e3) << endl;
					insert_time = 0;   // restart measuring
					cout << "Processed " << inserted << " keys" << endl;
				}
			}
			if (key_hash.size() == full_r) {  // no need to read the rest of the file
				break;
			}
			line_no++;
		}
		file.close();
		outfile.close();
	}
	else {
		cout << "File couldn't be opened." << endl;
	}
}

int main(int argc, char *argv[]) {
	// string file_name = "keys_4M_L6-20.txt";
	string file_name = "keys_6M_L6-25.txt";
	
	auto t1 = chrono::high_resolution_clock::now();
	// fill_hash_from_file(file_name, 0.001);   // run on 1/1000 of total keys
	fill_hash_from_file(file_name);
	auto t2 = chrono::high_resolution_clock::now();
	cout << "Total execution: " << chrono::duration_cast<chrono::milliseconds>(t2 - t1).count() / 1000.0 << " seconds" 
		  << endl << endl;
	cout << "Press enter to continue...";
	getchar();
	return 0;
}