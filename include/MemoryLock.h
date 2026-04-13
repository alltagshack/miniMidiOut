#ifndef __MEMORY_LOCK_H
#define __MEMORY_LOCK_H 1

#include <sys/mman.h>
#include <system_error>

class MemoryLock {
public:
    MemoryLock (int flags = MCL_CURRENT | MCL_FUTURE) {
        if (int err = mlockall(flags); err != 0) {
            throw std::system_error(errno, std::generic_category(), "mlockall failed");
        }
    }
    ~MemoryLock () {
        munlockall();
    }
};
#endif // __MEMORY_LOCK_H
