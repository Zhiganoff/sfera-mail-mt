#include <Memory.h>
#include <ctime>
#include <cstdlib>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <cstring>
#include <zlib.h>

#define MEMORY_SIZE 1024 * 1024 * 10 // 10mb
#define MAX_BODY_SIZE 10 * 1024      //10Kb

Memory::Memory() {
	master = 1;
}

Memory::~Memory() {
	if (master) {
		fprintf(stderr, "Master destroing memory\n");
		int r = shmctl(shmid, IPC_RMID, NULL);
		if (r == -1) {
			fprintf(stderr, "Memory destroy failed: %d %s\n", errno, strerror(errno));
		} else {
			fprintf(stderr, "Memory destroyed successfully\n");
		}
	}
}


void Memory::create() {
	if (!master) {
		return;
	}

	int flags = 0644 | IPC_CREAT | IPC_EXCL;

	key_t key = ftok("/dev/sda1", 'R');
	shmid = shmget(key, MEMORY_SIZE, flags);
	if (shmid == -1) {
		fprintf(stderr, "Shared memory failed to create for %d: %s\n", key, strerror(errno));
		throw "memory_create";
	} else {
		fprintf(stderr, "Memory created\n");
	}

	this->attach(shmid);

	// Headers_Len + Keys_Count + { Key TTL Val_ptr }
	Headers headers;
	memset(&headers, 0, sizeof(headers));

	headers.len = sizeof(headers);
	headers.key_count = 0;
	headers.offset = headers.len;

	memcpy(shm, &headers, headers.len); // push to shared memory
	fprintf(stderr, "Memory created on %d bytes\nMemory Headers Len %u bytes\n", MEMORY_SIZE, headers.len);
}

void Memory::attach(int _shmid) {
	this->shmid = _shmid;
	this->shm = (char *) shmat(shmid, (void *) 0, 0);
	fprintf(stderr, "Memory attached: %d\n", shmid);
}

void Memory::detach() {
	shmdt(shm);
	fprintf(stderr, "Memory detached: %d\n", shmid);
}

void Memory::defrag() {
	Headers headers = _get_headers();

	uint32_t old_offset = headers.offset;
	uint32_t prev = headers.len;	 // Start of Body
	for (uint i = 0; i < headers.key_count; i++) {

		if (prev < headers.data[i].val_ptr) {
			uint32_t old = headers.data[i].val_ptr;

			// 1. Correct header:
			headers.data[i].val_ptr = prev;

			// 2. Correct body:
			memcpy( shm + prev, shm + old, headers.data[i].body_len );
		}

		prev = headers.data[i].val_ptr + headers.data[i].body_len;
	}

	if (headers.key_count > 0) {
		Node last = headers.data[ headers.key_count - 1 ];
		headers.offset = last.val_ptr + last.body_len;
	} else {
		headers.offset = headers.len;
	}

	uint32_t defragmentated = old_offset - headers.offset;
	if (defragmentated > 0) {
		fprintf(stderr, "Defragmentated: %u\n", defragmentated);
	}

	_set_headers(headers);
}

void Memory::ttlkiller() {
	Headers headers = _get_headers();

	uint32_t now = time(NULL);
	for (uint i = 0; i < headers.key_count; i++) {
		if (headers.data[i].ttl < now) {
			fprintf(stderr, "TTL Expired: %u\n", headers.data[i].key);

			for (uint j = i; j < headers.key_count - 1; j++) {
				headers.data[j] = headers.data[j + 1];
			}

			headers.key_count -= 1;
			i -= 1;
		}
	}

	_set_headers(headers);
}


/**************************************************************************************************/
/* API to export/import Headers from Shared Memory */

Headers Memory::_get_headers() {
	uint32_t head_len = *((uint32_t *) shm);
	// fprintf(stderr, "Head %u\n", head_len);

	Headers headers;
	if (sizeof(headers) != head_len) {
		fprintf(stderr, "Fucking length are different! %lu!=%u\n", sizeof(headers), head_len);
		throw "Length!";
	} else {
		memcpy(&headers, shm, head_len);
	}

	// fprintf(stderr, "offset = %u\n", headers.offset);
	return headers;
}

void Memory::_set_headers(const Headers & headers) {
	memcpy(shm, &headers, sizeof(headers));
}

/**************************************************************************************************/



/**************************************************************************************************/
/* Public API Methods: */

void Memory::set(uint32_t ttl, string key, string val) {
	Headers headers = _get_headers();

	uint32_t death = time(NULL) + ttl;
	uint32_t hash = crc32(0, (const unsigned char *) key.c_str(), key.length());
	uint32_t val_len = val.length();

	fprintf(stderr, "val_len = %u\n", val_len);
	if (val_len > MAX_BODY_SIZE) {
		throw std::string("too_big_value");
	}

	Node found = headers.find(hash);
	if (found == Node::Empty) {
		fprintf(stderr, "Not found. Creating\n");

		char * ptr = shm + headers.offset;
		memcpy(ptr, val.c_str(), val_len);                          // push value
		headers.push(hash, death, val_len, headers.offset);         // set key
		headers.offset += val_len;

	} else {
		fprintf(stderr, "Found. ");
		if (found.body_len >= val_len) {
			fprintf(stderr, "Updating\n");

			char * ptr = shm + found.val_ptr;
			memcpy(ptr, val.c_str(), val_len);                      // push value
			headers.push(hash, death, val_len, found.val_ptr);      // set key

		} else {
			fprintf(stderr, "Shifting\n");

			char * ptr = shm + headers.offset;
			memcpy(ptr, val.c_str(), val_len);                      // push value
			headers.update(hash, death, val_len, headers.offset);   // set key
			headers.offset += val_len;
		}
	}

	_set_headers(headers);
}

string Memory::get(string key) {
	Headers headers = _get_headers();
	uint32_t hash = crc32(0, (const unsigned char *) key.c_str(), key.length());

	Node found = headers.find(hash);
	if (found == Node::Empty) {
		fprintf(stderr, "Not Found\n");
		throw std::string("not_found");
	} else {
		fprintf(stderr, "Found: ttl=%u; hash=%u; len=%u; val_ptr=%u\n", found.ttl, found.key, found.body_len, found.val_ptr);
	}

	if (found.ttl < time(NULL)) {
		fprintf(stderr, "TTL expired\n");
		throw std::string("not_found");
	}

	char body[found.body_len + 1];
	memcpy(body, shm + found.val_ptr, found.body_len);   // extract body

	// TODO: bad idea! Need struct with length
	body[found.body_len] = '\0';
	fprintf(stderr, "body: %s\n", body);

	return body;
}

void Memory::remove(string key) {
	Headers headers = _get_headers();

	uint32_t hash = crc32(0, (const unsigned char *) key.c_str(), key.length());
	Node found = headers.find(hash);
	if (found == Node::Empty) {
		fprintf(stderr, "Not Found\n");
		return;
	} else {
		fprintf(stderr, "ttl=%u; hash=%u; len=%u; val_ptr=%u\n", found.ttl, found.key, found.body_len, found.val_ptr);
	}

	headers.remove(hash);
	_set_headers(headers);
}

/**************************************************************************************************/
