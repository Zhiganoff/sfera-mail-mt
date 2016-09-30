#include <stdexcept>
#include <string>
#include <list>
#include <vector>
#include <cstring>
#include <iostream>

enum class AllocErrorType {
    InvalidFree,
    NoMemory,
};

class AllocError: std::runtime_error {
private:
    AllocErrorType type;

public:
    AllocError(AllocErrorType _type, std::string message):
            runtime_error(message),
            type(_type)
    {}

    AllocErrorType getType() const { return type; }
};

class Allocator;

class Pointer {
public:
    int idx;
    size_t size_;

    static std::vector<void*> pointers;

    friend Allocator;

    Pointer(void *p = nullptr, size_t new_size = 0): size_(new_size) {
        pointers.push_back(p);
        idx = pointers.size() - 1;
    }

    void *get() const { return pointers[idx]; }
    void *set(void *p) { pointers[idx] = p; }
};

struct memBlock {
    void *pointer_;
    size_t size_;
    size_t used_;
    int idx_;
    memBlock(void *pointer, size_t size, size_t used, int idx): pointer_(pointer),
                                                                size_(size),
                                                                used_(used),
                                                                idx_(idx) {}
};

class Allocator {
    void *base_;
    size_t size_;
    std::list<memBlock> blocks;

public:
    Allocator(void *base, size_t size): base_(base), size_(size) {}
    
    Pointer alloc(size_t N);
    void realloc(Pointer &p, size_t N);
    void free(Pointer &p);
    void defrag();
    void dump() {
        std::list< memBlock >::iterator it = blocks.begin();
        std::cout << std::endl;
            while (it != blocks.cend()) {
                std::cout << "pointer: " << (*it).pointer_ << " idx: " << it->idx_ << " size: " << (*it).size_ << " used: " << (*it).used_ << std::endl;
                it++;
            }
    }

    void moveMem(void * destptr, void * srcptr, size_t num);
};

