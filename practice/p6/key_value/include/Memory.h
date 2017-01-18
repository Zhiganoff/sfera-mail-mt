#pragma once
#include <string>
#include <cstring>

#define MAX_KEY_COUNT 1000

using namespace std;

struct Node {
	uint32_t key;
	uint32_t ttl;
	uint32_t body_len;
	uint32_t val_ptr;

	Node() {};
	Node(uint32_t hash, uint32_t death, uint32_t len, uint32_t val)
	: key(hash), ttl(death), body_len(len), val_ptr(val) { };

	static Node Empty;
	friend bool operator==(const Node &n1, const Node &n2);
};

inline bool operator==(const Node &n1, const Node &n2) {
	return n1.key == n2.key;
}


class Headers {
public:
	uint32_t len;
	uint32_t offset;
	uint32_t key_count;
	Node data[MAX_KEY_COUNT];

	Node find(uint32_t key);
	Node push(uint32_t hash, uint32_t death, uint32_t len, uint32_t start);
	Node update(uint32_t hash, uint32_t death, uint32_t len, uint32_t start);
	void remove(uint32_t hash);
};

class Memory {
	char * shm;
	int shmid;
	int master;

	Headers _get_headers();
	void _set_headers(const Headers & hdrs);

public:
	Memory();
	string get(string key);
	void set(uint32_t ttl, string key, string val);
	void remove(string key);

	void become_child() {
		master = 0;
	}

	void create();
	void attach(int _shmid);
	void detach();

	void defrag();
	void ttlkiller();

	int get_id() {
		return shmid;
	}

	~Memory();
};