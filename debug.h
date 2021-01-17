#ifndef DEBUG_H
#define DEBUG_H

// Set this if allow debug output
#define DEBUG 1

// Assumes C99 or later compiler
#define debug_print(fmt, ...) \
        do { if (DEBUG) fprintf(stderr, fmt, ##__VA_ARGS__); } while(0)

#define info_print(fmt, ...) \
        do { if (DEBUG) fprintf(stdout, fmt, ##__VA_ARGS__); } while(0)

#endif //DEBUG_H
