# =============================================================================
# djinterp - build/cmake/djinterp/sync.cmake
#
#   djinterp_c_sync: the concurrent library (ruled 2026.11.17). The atomic
# and mutex kinds of the counter and the timer, on c/sync's primitives. It
# links the runtime, the platform's threads, and libatomic where the
# platform's atomics need it.
# =============================================================================

find_package(Threads REQUIRED)

djinterp_add_library(djinterp_c_sync
    SOURCES
        "${DJINTERP_SRC_ROOT}/djinterp/c/sync/datomic.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/sync/dmutex.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/util/counter/counter_atomic.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/util/counter/counter_mutex.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/util/timer/timer_atomic.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/util/timer/timer_mutex.c"
    LINKS
        djinterp_c_runtime
        Threads::Threads)

#   A 64-bit atomic without a library: where it fails to link (32-bit ARM
# and x86 with some toolchains), libatomic supplies it.
if (NOT MSVC)
    include(CheckCSourceCompiles)
    set(_djinterp_atomic_probe "
#include <stdatomic.h>
#include <stdint.h>
int main(void)
{
    _Atomic uint64_t value = 0;

    atomic_fetch_add(&value, 1);

    return (int)atomic_load(&value);
}")
    check_c_source_compiles("${_djinterp_atomic_probe}"
                            DJINTERP_ATOMIC_WITHOUT_LIBRARY)
    if (NOT DJINTERP_ATOMIC_WITHOUT_LIBRARY)
        set(CMAKE_REQUIRED_LIBRARIES atomic)
        check_c_source_compiles("${_djinterp_atomic_probe}"
                                DJINTERP_ATOMIC_WITH_LIBRARY)
        unset(CMAKE_REQUIRED_LIBRARIES)
        if (DJINTERP_ATOMIC_WITH_LIBRARY)
            target_link_libraries(djinterp_c_sync PUBLIC atomic)
        endif()
    endif()
endif()
