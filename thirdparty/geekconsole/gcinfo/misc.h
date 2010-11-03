#ifndef _MISCH_
#define _MISC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/stat.h>

#include <errno.h>

/* Assume ANSI C89 headers are available.  */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <fcntl.h>

#if !defined (whitespace)
#  define whitespace(c) ((c == ' ') || (c == '\t'))
#endif /* !whitespace */

#if !defined (whitespace_or_newline)
#  define whitespace_or_newline(c) (whitespace (c) || (c == '\n'))
#endif /* !whitespace_or_newline */

/* Add POINTER to the list of pointers found in ARRAY.  SLOTS is the number
   of slots that have already been allocated.  INDEX is the index into the
   array where POINTER should be added.  GROW is the number of slots to grow
   ARRAY by, in the case that it needs growing.  TYPE is a cast of the type
   of object stored in ARRAY (e.g., NODE_ENTRY *. */
#define add_pointer_to_array(pointer, idx, array, slots, grow, type) \
  do { \
    if (idx + 2 >= slots) \
      array = (type *)(realloc (array, (slots += grow) * sizeof (type))); \
    array[idx++] = (type)pointer; \
    array[idx] = (type)NULL; \
  } while (0)

#define maybe_free(x) do { if (x) free (x); } while (0)

#if !defined (zero_mem) && defined (HAVE_MEMSET)
#  define zero_mem(mem, length) memset (mem, 0, length)
#endif /* !zero_mem && HAVE_MEMSET */

#if !defined (zero_mem) && defined (HAVE_BZERO)
#  define zero_mem(mem, length) bzero (mem, length)
#endif /* !zero_mem && HAVE_BZERO */

#if !defined (zero_mem)
#  define zero_mem(mem, length) \
  do {                                  \
        register int zi;                \
        register unsigned char *place;  \
                                        \
        place = (unsigned char *)mem;   \
        for (zi = 0; zi < length; zi++) \
          place[zi] = 0;                \
      } while (0)
#endif /* !zero_mem */

#include <fcntl.h>
/* MS-DOS and similar non-Posix systems have some peculiarities:
    - they distinguish between binary and text files;
    - they use both `/' and `\\' as directory separator in file names;
    - they can have a drive letter X: prepended to a file name;
    - they have a separate root directory on each drive;
    - their filesystems are case-insensitive;
    - directories in environment variables (like INFOPATH) are separated
        by `;' rather than `:';
    - text files can have their lines ended either with \n or with \r\n pairs;
   These are all parameterized here except the last, which is
   handled by the source code as appropriate (mostly, in info/).  */
#ifndef O_BINARY
# ifdef _O_BINARY
#  define O_BINARY _O_BINARY
# else
#  define O_BINARY 0
# endif
#endif /* O_BINARY */

#if O_BINARY
# define FOPEN_RBIN	"rb"
# define FOPEN_WBIN	"wb"
#else
# define FOPEN_RBIN	"r"
# define FOPEN_WBIN	"w"
#endif

# define SET_BINARY(f)   (void)0
# define PATH_SEP	    ";"

#ifdef _MSC_VER
# define FILENAME_CMP	mbscasecmp
# define FILENAME_CMPN	mbsncasecmp
# define HAVE_DRIVE(n)	((n)[0] && (n)[1] == ':')
# define IS_SLASH(c)	((c) == '/' || (c) == '\\')
# define IS_ABSOLUTE(n) (IS_SLASH((n)[0]) || HAVE_DRIVE(n))
# define STRIP_DOT_EXE	1
#else /* unix */
#include <pwd.h>
# define FILENAME_CMP	strcmp
# define FILENAME_CMPN	strncmp
# define IS_SLASH(c)	((c) == '/')
# define HAVE_DRIVE(n)	(0)
# define IS_ABSOLUTE(n) ((n)[0] == '/')
# define STRIP_DOT_EXE	0
#endif

/* some old def */
#define BUILDING_LIBRARY 1

/* For convenience.  */
#define STREQ(s1,s2) (strcmp (s1, s2) == 0)
#define STRCASEEQ(s1,s2) (strcasecmp (s1, s2) == 0)
#define STRNCASEEQ(s1,s2,n) (strncasecmp (s1, s2, n) == 0)

/* XXX: mbscasecmp is a part of gnu lib, may be need add it */
#ifndef mbscasecmp
# define mbscasecmp strcasecmp
#endif

#ifndef mbsncasecmp
# define mbsncasecmp strncasecmp
#endif

#ifdef __cplusplus
}
#endif

#endif // _MISC_H_
