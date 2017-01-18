#pragma once
#include <sys/types.h>

enum types {
	MESSAGE_CONNECT,
	MESSAGE_DISCONNECT,

	MESSAGE_GET,
	MESSAGE_SET,
	MESSAGE_DELETE
};

struct Message {
	uint32_t len;
 	char command;
	char *buf;
	Message() {
		len = 0;
		buf = nullptr;
	}
	void resize_buf (uint32_t bytes) {
		buf = (char *) malloc(bytes);
	}
	~Message() {
		if (buf) {
			free(buf);
		}
	}
};


