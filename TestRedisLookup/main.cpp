#include <cpp_redis/cpp_redis>
#include <iostream>

#ifdef _WIN32
  #include <Winsock2.h>
  #pragma comment(lib, "ws2_32.lib")
#endif /* _WIN32 */

#include <stdio.h>
#include <tchar.h>
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

const bool CREATE_HASH = true;  // if we should create the hash in redix. set to false if it was created in a previous run.
const string REDIS_HASH_NAME = "myhash";

void fill_hash_from_file(string filename, cpp_redis::redis_client& client, float ratio = 1.0) {
	unsigned commit_every = 1000;
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
		unsigned keys_sent = 0;
		while (getline(file, line)) {
			if (line_no < pos) {  // positive test cases
				if (line_no < pos_r) {
					test_POS.insert(line);
					if (CREATE_HASH) {
						client.send({ "HSET", REDIS_HASH_NAME, line, "1" });
						keys_sent++;
						if (keys_sent % commit_every == 0) {
							client.commit();
						} 
					}
				}
			}
			else if (line_no < pos + neg) {  // negative test cases
				if (line_no < pos + neg_r) {
					test_NEG.insert(line);
				}
			}
			else {  // other elements in hash (not a positive test case)
				if (!CREATE_HASH) break;
				else if (keys_sent < full_r) {
					keys_sent++;
					client.send({ "HSET", REDIS_HASH_NAME, line, "1" });
					if (keys_sent % commit_every == 0) {
						client.commit();
					}
				}
				else {
					break;
				}
			}
			line_no++;
		}
		if (CREATE_HASH) {
			client.sync_commit(); 
		}
		file.close();
	}
	else {
		cout << "File couldn't be opened." << endl;
	}
}

int main(int argc, char *argv[]) {
#ifdef _WIN32
	//! Windows netword DLL init
	WORD version = MAKEWORD(2, 2);
	WSADATA data;

	if (WSAStartup(version, &data) != 0) {
		std::cerr << "WSAStartup() failure" << std::endl;
		return -1;
	}
#endif /* _WIN32 */,

	// setup redis client
	cpp_redis::active_logger = std::unique_ptr<cpp_redis::logger>(new cpp_redis::logger);

	cpp_redis::redis_client client;

	client.connect("127.0.0.1", 6379, [](cpp_redis::redis_client&) {
		std::cout << "client disconnected (disconnection handler)" << std::endl;
	});

	if (CREATE_HASH) {
		// reset hash in Redis
		client.send({ "DEL", REDIS_HASH_NAME}).sync_commit();
	}


	string file_name = "C:\\Users\\exper\\Documents\\Visual Studio 2015\\Projects\\TestHashInsert\\Debug\\keys_6M_L6-25.txt";
	// string file_name = "keys_4M_L6-20.txt";
	// string file_name = "keys_1M_L10.txt";
	
	auto t1 = chrono::high_resolution_clock::now();
	// fill_hash_from_file(file_name, client, 0.01);
	fill_hash_from_file(file_name, client);
	auto t2 = chrono::high_resolution_clock::now();
	cout << "Hash and test sets creation: " << chrono::duration_cast<chrono::milliseconds>(t2 - t1).count() / 1000.0 << " seconds"
		<< endl << endl;
	
	/* printf("Length of hash: %d\n", key_hash.size());
	printf("Length of positive test cases: %d\n", test_POS.size());
	printf("Length of negative test cases: %d\n", test_NEG.size()); */

	// test positives, keys found in the hash
	double total_time_POS = 0;
	// uint64_t time_diff;
	vector <string> cmd = { "HMGET", REDIS_HASH_NAME };
	unsigned batch_size = 50;
	unsigned total_batches_POS = 0;
	vector<chrono::high_resolution_clock::time_point> send_times;
	vector<chrono::high_resolution_clock::time_point> receive_times;
	for (const string& key : test_POS) {
		cmd.push_back(key);
		if (cmd.size() - 2 == batch_size) {			
			client.send(cmd, [&receive_times](cpp_redis::reply& reply) {
				receive_times.push_back(chrono::high_resolution_clock::now());
			});
			send_times.push_back(chrono::high_resolution_clock::now());
			client.sync_commit();
			total_batches_POS++;
			cmd.clear();
			cmd = { "HMGET", REDIS_HASH_NAME };
		}
	}
	while (!send_times.empty()) {
		// cout << "  Time vectors:" << send_times.size() << " " << receive_times.size() << endl;
		while (receive_times.empty()) {
			// wait for the response
			// Sleep(0);
		}
		t1 = send_times.back();
		t2 = receive_times.back();
		total_time_POS += chrono::duration_cast<chrono::nanoseconds>(t2 - t1).count();
		send_times.pop_back();
		receive_times.pop_back();		
	}
	// cout << "Time vectors:" << send_times.size() << " " << receive_times.size() << endl << endl;
	cout << "POS test time total: " << total_time_POS / 1e9 << " seconds" << endl;
	cout << "POS test time average lookup: " << total_time_POS / (1e3 * total_batches_POS * batch_size) << " microseconds" << endl;

	// test negatives, keys not found in the hash
	double total_time_NEG = 0;
	unsigned total_batches_NEG = 0;
	cmd.clear();
	cmd = { "HMGET", REDIS_HASH_NAME };
	for (const string& key : test_NEG) {
		cmd.push_back(key);
		if (cmd.size() - 2 == batch_size) {
			client.send(cmd, [&receive_times](cpp_redis::reply& reply) {
				receive_times.push_back(chrono::high_resolution_clock::now());
			});
			send_times.push_back(chrono::high_resolution_clock::now());
			client.sync_commit();
			total_batches_NEG++;
			cmd.clear();
			cmd = { "HMGET", REDIS_HASH_NAME };
		}
	}
	while (!send_times.empty()) {
		// cout << "  Time vectors:" << send_times.size() << " " << receive_times.size() << endl;
		while (receive_times.empty()) {
			// wait for the response
			// Sleep(0);
		}
		t1 = send_times.back();
		t2 = receive_times.back();
		total_time_NEG += chrono::duration_cast<chrono::nanoseconds>(t2 - t1).count();
		send_times.pop_back();
		receive_times.pop_back();
	}
	cout << "NEG test time total: " << total_time_NEG / 1e9 << " seconds" << endl;
	cout << "NEG test time average lookup: " << total_time_NEG / (1e3 * total_batches_NEG * batch_size)  << " microseconds" << endl;

	cout << "Average lookup together: " << (total_time_POS + total_time_NEG) / (1e3 * batch_size * (total_batches_POS + total_batches_NEG)) << endl << endl;


#ifdef _WIN32
	WSACleanup();
#endif /* _WIN32 */
	test_POS.clear();
	test_NEG.clear();
	cout << "Press enter to continue...";
	getchar();
	return 0;
}

/*
int main_ex (void) {



	// same as client.send({ "SET", "hello", "42" }, ...)
	client.set("hhello", "82", [](cpp_redis::reply& reply) {
		std::cout << "set hello 42: " << reply << std::endl;
		// if (reply.is_string())
		//   do_something_with_string(reply.as_string())
	});

	// same as client.send({ "DECRBY", "hello", 12 }, ...)
	client.decrby("hhello", 33, [](cpp_redis::reply& reply) {
		std::cout << "decrby hello 12: " << reply << std::endl;
		// if (reply.is_integer())
		//   do_something_with_integer(reply.as_integer())
	});

	// same as client.send({ "GET", "hello" }, ...)
	client.get("hhello", [](cpp_redis::reply& reply) {
		std::cout << "get hello: " << reply << std::endl;
		if (reply.is_string())
			std::cout << "hhello is: " << reply.as_string() << std::endl;
	});

	// commands are pipelined and only sent when client.commit() is called
	// client.commit();

	// synchronous commit, no timeout
	client.sync_commit();

	// synchronous commit, timeout
	// client.sync_commit(std::chrono::milliseconds(100));



	std::cout << "Enter to finish ...";
	std::getchar();
	return 0;
}
*/