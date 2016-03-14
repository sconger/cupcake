
#ifndef CUPCAKE_ASYNC_H
#define CUPCAKE_ASYNC_H

#include <functional>

namespace Cupcake {
    
namespace Async {

    /*
     * Runs the passed callback in either a system thread pool, or one generated
     * by the library if the native OS lacks a built in thread pool.
     */
    void runAsync(std::function<void()> callback);
}

}

#endif // CUPCAKE_ASYNC_H
