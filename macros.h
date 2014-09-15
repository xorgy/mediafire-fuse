#ifndef _MACROS_H_
#define _MACROS_H_

// calculate the absolute value on a 2s-complement system
#define ABSINT(x)   ((x^(x>>((sizeof(x)*8)-1)))-(x>>((sizeof(x)*8)-1)))

// count the number of elements in the first axis of an array
#define DIM(x)      (sizeof(x)/sizeof(x[0]))

// portable implementation of getpagesize().  see NOTE 1 below.
#if (!defined getpagesize) && (defined _SC_PAGESIZE)
#define getpagesize(x)  (sysconf(_SC_PAGESIZE))
#endif
#if (!defined getpagesize) && (defined _SC_PAGE_SIZE)
#define getpagesize(x)  (sysconf(_SC_PAGE_SIZE))
#endif

/*
    Note 1: Regarding getpagesize()

    In SUSv2 the getpagesize() call is labeled LEGACY, and in POSIX.1-2001
    it has been dropped;  HP-UX does not have this call.

    I suspect on most systems that do have getpagesize() it is a macro so
    we'll assume it to be.  If not, we'll define it ourselves.

    Note: That on some systems both _SC_PAGESIZE and _SC_PAGE_SIZE are
    interchangeable so we'll try both.
*/



#endif

