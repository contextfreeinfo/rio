#if defined(__aarch64__)
    #include "gen-aarch64.c"
#elif defined(__thumb2__)
    #include "gen-thumb.c"
#elif defined(__x86_64__) || defined(_M_X64)
    #if defined(_WIN32)
        #include "gen-x64-win.c"
    #else
        #include "gen-x64.c"
    #endif
#endif
