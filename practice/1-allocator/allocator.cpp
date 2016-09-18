#include "allocator.h"

Pointer Allocator::alloc(size_t N) {
	if (blocks.empty()) {
		if (N <= size_) {
			blocks.push_back(memBlock(base_, N, N));
			//dump();
			return Pointer(base_);
		} else throw AllocError(AllocErrorType::NoMemory, "NoMemory");
	} else {
		std::list< memBlock >::iterator it = blocks.end();
		it--;
		if ( size_ - (*it).used_ >= N) {
			blocks.push_back(memBlock((char *)(*it).pointer_ + (*it).size_, N, (*it).used_ + N));
			return Pointer((char *)(*it).pointer_ + (*it).size_);
		} else throw AllocError(AllocErrorType::NoMemory, "NoMemory");
	}
}

/*void Allocator::realloc(Pointer &p, size_t N) {
	p = alloc(N);
	free(p);
}*/

void Allocator::realloc(Pointer &p, size_t N) {
	Pointer source = p;
	size_t num;
	if (blocks.empty() || p.get() == nullptr) {
		throw AllocError(AllocErrorType::InvalidFree, "InvalidFree");
	} else {
		std::list< memBlock >::iterator it = blocks.begin();
		while (it != blocks.cend() && (*it).pointer_ != p.get()) {
			it++;
		}
		if (it == blocks.cend()) {
			throw AllocError(AllocErrorType::NoMemory, "NoMemory");
		} else {
			num = (*it).size_;
			blocks.erase(it);
		}
	}
	p = alloc(N);
	moveMem(p.get(), source.get(), std::min(num, N));
}

void Allocator::free(Pointer &p) {
	if (blocks.empty() || p.get() == nullptr) {
		throw AllocError(AllocErrorType::InvalidFree, "InvalidFree");
	} else {
		std::list< memBlock >::iterator it = blocks.begin();
		while (it != blocks.cend() && (*it).pointer_ != p.get()) {
			it++;
		}
		if (it == blocks.cend()) {
			throw AllocError(AllocErrorType::NoMemory, "NoMemory");
		} else {
			blocks.erase(it);
		}
	}
	p.set(nullptr);
}

void Allocator::defrag() {
	if (blocks.empty()) {
		return;
	} else {
		std::list< memBlock >::iterator it = blocks.begin();
		if (base_ != (*it).pointer_) {
			moveMem(base_, (*it).pointer_, (*it).size_);
		}
		if (blocks.size() == 1) {
			return;
		}
		std::list< memBlock >::iterator nxt = blocks.begin();
		nxt++;
		while (nxt != blocks.cend()) {
			if ((char *)base_ + (*it).used_ != (*nxt).pointer_) {
				moveMem((char *)base_ + (*it).used_, (*nxt).pointer_, (*nxt).size_);
				(*nxt).pointer_ = (char *)base_ + (*it).used_;
			}
			it++; 
			nxt++;
		}
	}
}

void Allocator::moveMem(void * destptr, void * srcptr, size_t num) {
	void *tmp = srcptr;
	for (size_t idx = 0; idx <= num; idx++) {
		*((char*)destptr + idx) = *((char*)srcptr + idx);
	}
}