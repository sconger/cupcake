
#ifndef _WIN32
#error "This file should only be included in a Windows build"
#endif

#include "cupcake/internal/async/Async.h"

#include <Windows.h>

#include <memory>

static
void CALLBACK workFunc(PTP_CALLBACK_INSTANCE callbackInstance,
    PVOID threadParam,
    PTP_WORK work) {
    // Aquire ownership of the pointer as a unique_ptr so it is automatically
    // deleted.
    std::unique_ptr<std::function<void()>> uniquePtr(
        (std::function<void()>*)threadParam);

    // Run the function
    (*uniquePtr)();
}

namespace Cupcake {
namespace Async {

void runAsync(std::function<void()> callback) {
    // A std::function can be of varrying size, but we need to pass a pointer.
    // Allocate a space for it with new and wrap it in a unique_ptr for
    // automatic cleanup. In the case where the thread starts, we let the
    // thread delete it.
    std::unique_ptr<std::function<void()>> uniquePtr(
        new std::function<void()>(std::move(callback)));

    PTP_WORK ptpWork = ::CreateThreadpoolWork(workFunc,
        (void*)uniquePtr.get(),
        NULL);

    if (ptpWork == NULL) {
        // TODO: Log errors? Probably only fails due to alloc failure.
        throw std::bad_alloc();
    }

    ::SubmitThreadpoolWork(ptpWork);

    // Disable the normal unique_ptr automatic deletion as the thread
    // will delete it.
    uniquePtr.release();
}

}
}
