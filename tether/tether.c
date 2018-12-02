#if defined TETHER_WIN32
    #include "win32.c"
#elif defined TETHER_MACOS
    #include "macos.c"
#else
    #include "gtk.c"
#endif
