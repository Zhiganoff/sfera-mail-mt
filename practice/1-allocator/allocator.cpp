#include "allocator.h"

std::vector<void*> Pointer::pointers;

Pointer Allocator::alloc(size_t N) {
	if (blocks.empty()) {
		if (N <= size_) {
			memBlock new_block(base_, N, N, Pointer::pointers.size());
			blocks.push_back(new_block);
			return Pointer(base_, N);
		} else {
			throw AllocError(AllocErrorType::NoMemory, "NoMemory");
		}
	} else {
		auto it = blocks.begin();
		auto nxt = blocks.begin();
		nxt++;
		while (nxt != blocks.end()) {
			if ((char*)nxt->pointer_ - (char*)it->pointer_ - it->size_ >= N) {
				// вставляем блок
				memBlock new_block((char *)(it->pointer_) + it->size_, N, it->used_ + N, Pointer::pointers.size());
				blocks.insert(nxt, new_block);
				return Pointer((char *)it->pointer_ + it->size_, N);				
			}
			it++;
			nxt++;
		}
		if (size_ - it->used_ >= N) {
			memBlock new_block((char *)(it->pointer_) + it->size_, N, it->used_ + N, Pointer::pointers.size());
			blocks.push_back(new_block);
			return Pointer((char *)it->pointer_ + it->size_, N);
		} else {
			throw AllocError(AllocErrorType::NoMemory, "NoMemory");
		}		
	}
}

void Allocator::realloc(Pointer &p, size_t N) {
	if (p.get() == nullptr) {
		p = alloc(N);
		return;
	}
	void *source = p.get();
	size_t num;
	if (p.get() == nullptr) {
		p = alloc(N);
		return;
	}
	if (blocks.empty()) {
		throw AllocError(AllocErrorType::InvalidFree, "InvalidFree");
	} else {
		auto it = blocks.begin();
		while (it != blocks.cend() && it->pointer_ != p.get()) {
			it++;
		}
		if (it == blocks.cend()) {
			throw AllocError(AllocErrorType::NoMemory, "InvalidFree");
		} else {
			num = (*it).size_;
			blocks.erase(it);
		}
	}
	p = alloc(N);
	moveMem(p.get(), source, std::min(num, N));
}

void Allocator::free(Pointer &p) {
	if (blocks.empty() || p.get() == nullptr) {
		throw AllocError(AllocErrorType::InvalidFree, "InvalidFree");
	} else {
		auto it = blocks.begin();
		while (it != blocks.cend() && it->pointer_ != p.get()) {
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
		auto it = blocks.begin();
		if (base_ != it->pointer_) {
			Pointer::pointers[it->idx_] = base_;
			it->used_ = it->size_;
			moveMem(base_, (*it).pointer_, (*it).size_);
		}
		if (blocks.size() == 1) {
			return;
		}
		auto nxt = blocks.begin();
		nxt++;
		while (nxt != blocks.cend()) {
			if ((char *)base_ + (*it).used_ != (*nxt).pointer_) {
				moveMem((char *)base_ + (*it).used_, (*nxt).pointer_, (*nxt).size_);
				nxt->pointer_ = (char *)base_ + (*it).used_;
				nxt->used_ += (char*)it->pointer_ - (char*)nxt->pointer_;
				Pointer::pointers[nxt->idx_] = nxt->pointer_;
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