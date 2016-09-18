#include <stdexcept>
#include <string>
#include <list>
#include <cstring>

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

struct memBlock;

class Pointer {
    void *p_;
public:
    Pointer(void *p = nullptr): p_(p) {}
    void *get() const { return p_; }
    void *set(void *p) { p_ = p; }
};

struct memBlock {
    void *pointer_;
    size_t size_;
    size_t used_;
    memBlock(void *pointer, size_t size, size_t used): pointer_(pointer), size_(size), used_(used) {}
};

class Allocator {
    void *base_;
    size_t size_;
    std::list<memBlock> blocks;
public:
    Allocator(void *base, size_t size): base_(base), size_(size) {
        //freeBlocks.push_back(memBlock(base, size));
    }
    
    Pointer alloc(size_t N);
    void realloc(Pointer &p, size_t N);
    void free(Pointer &p);
    void defrag();
    std::string dump() { /*return ""; }*/
        //std::cout << "Used: " << (*blocks.begin()).used_ << std::endl;
    }

    void moveMem(void * destptr, void * srcptr, size_t num);
    void *getBase() { return base_; }
    size_t getSize() { return size_; }
};

