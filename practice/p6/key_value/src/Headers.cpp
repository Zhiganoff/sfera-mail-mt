#include <Memory.h>

Node Headers::find(uint32_t key) {
	for (uint i = 0; i < key_count; i++) {
		if (data[i].key == key) {
			return data[i];
		}
	}
	return Node::Empty;
}

Node Headers::push(uint32_t hash, uint32_t death, uint32_t len, uint32_t start) {
	uint i = 0;
	for (; i < key_count; i++) {
		if (data[i].key == hash) {
			break;
		}
	}

	if (i == key_count) {
		data[key_count] = Node(hash, death, len, start);
		return data[key_count++];
	}

	data[i].ttl = death;
	data[i].body_len = len;
	data[i].val_ptr = start;

	for (uint i = 0; i < key_count; i++) {
		fprintf(stderr, "i=%u; ttl=%u; hash=%u; len=%u; val_ptr=%u\n", i, data[i].ttl, data[i].key, data[i].body_len, data[i].val_ptr);
	}

	return data[i];
}

Node Headers::update(uint32_t hash, uint32_t death, uint32_t len, uint32_t start) {
	uint32_t i = 0;
	for (; i < key_count; i++) {
		if (data[i].key == hash) {
			break;
		}
	}

	fprintf(stderr, "i=%u; ttl=%u; hash=%u; len=%u; val_ptr=%u\n", i, data[i].ttl, data[i].key, data[i].body_len, data[i].val_ptr);

	Node key_info = data[i];
	key_info.ttl = death;
	key_info.body_len = len;
	key_info.val_ptr = start;

	// no goes shifting:
	for (; i < key_count - 1; i++) {
		data[i] = data[i + 1];
	}
	data[key_count - 1] = key_info;
	return key_info;
}

void Headers::remove(uint32_t hash) {
	uint32_t i = 0;
	for (; i < key_count; i++) {
		if (data[i].key == hash) {
			break;
		}
	}

	fprintf(stderr, "Remove i=%u; ttl=%u; hash=%u; len=%u; val_ptr=%u\n", i, data[i].ttl, data[i].key, data[i].body_len, data[i].val_ptr);
	
	// no goes shifting:
	for (; i < key_count - 1; i++) {
		data[i] = data[i + 1];
	}
	key_count--;
}