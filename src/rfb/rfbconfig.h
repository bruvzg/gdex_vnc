#ifndef _RFB_RFBCONFIG_H
#define _RFB_RFBCONFIG_H

/* Define to 1 if you have the `memmove' function. */
#define LIBVNCSERVER_HAVE_MEMMOVE  1 

/* Define to 1 if you have the `memset' function. */
#define LIBVNCSERVER_HAVE_MEMSET  1 

/* Define to 1 if you have the `select' function. */
#define LIBVNCSERVER_HAVE_SELECT  1 

/* Define to 1 if you have the `socket' function. */
#define LIBVNCSERVER_HAVE_SOCKET  1 

/* Define to 1 if you have the `strchr' function. */
#define LIBVNCSERVER_HAVE_STRCHR  1 

/* Define to 1 if you have the `strcspn' function. */
#define LIBVNCSERVER_HAVE_STRCSPN  1 

/* Define to 1 if you have the `strdup' function. */
#define LIBVNCSERVER_HAVE_STRDUP  1 

/* Define to 1 if you have the `strerror' function. */
#define LIBVNCSERVER_HAVE_STRERROR  1 

/* Define to 1 if you have the `strstr' function. */
#define LIBVNCSERVER_HAVE_STRSTR  1 

/* Define to the full name and version of this package. */
#define LIBVNCSERVER_PACKAGE_STRING  "LibVNCServer 0.9.14"

/* Define to the version of this package. */
#define LIBVNCSERVER_PACKAGE_VERSION  "0.9.14"
#define LIBVNCSERVER_VERSION "0.9.14"
#define LIBVNCSERVER_VERSION_MAJOR "0"
#define LIBVNCSERVER_VERSION_MINOR "9"
#define LIBVNCSERVER_VERSION_PATCHLEVEL "14"

/* Define to `int' if <sys/types.h> does not define. */
#define HAVE_LIBVNCSERVER_PID_T 1
#ifndef HAVE_LIBVNCSERVER_PID_T
typedef int pid_t;
#endif

/* The type for size_t */
#define HAVE_LIBVNCSERVER_SIZE_T 1
#ifndef HAVE_LIBVNCSERVER_SIZE_T
typedef int size_t;
#endif

/* The type for socklen */
#define HAVE_LIBVNCSERVER_SOCKLEN_T 1
#ifndef HAVE_LIBVNCSERVER_SOCKLEN_T
typedef int socklen_t;
#endif

/* once: _RFB_RFBCONFIG_H */
#endif
