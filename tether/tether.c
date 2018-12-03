#if defined TETHER_WIN32
    #include "win32.c"
#elif defined TETHER_COCOA
    #include "cocoa.m"
#else
    #include "gtk.c"
#endif
