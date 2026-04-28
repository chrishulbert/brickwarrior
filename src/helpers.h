// This is for random helpers.

// Make strncasecmp available on all platforms.
#ifdef _WIN32
    #include <string.h>
    #define strncasecmp _strnicmp
    #define strcasecmp _stricmp
#else
    #include <strings.h>
#endif
