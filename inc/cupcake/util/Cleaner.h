
#ifndef CUPCAKE_CLEANER_H
#define CUPCAKE_CLEANER_H

#include <functional>

// Helper that runs a lamda when it exits scope
class Cleaner {
public:
    Cleaner(std::function<void()> cleanupFunc) : cleanupFunc(cleanupFunc) {}
    ~Cleaner() {cleanupFunc();}
    
private:
    Cleaner(const Cleaner&) = delete;
    Cleaner(Cleaner&&) = delete;
    Cleaner& operator=(const Cleaner&) = delete;
    Cleaner& operator=(Cleaner&&) = delete;
    
    std::function<void()> cleanupFunc;
};

#endif // CUPCAKE_CLEANER_H
