
#include <sys/types.h>

#include <assert.h>
#include <errno.h>
#ifndef _WIN32
# include <poll.h>
#endif
#include <stdlib.h>
#include <unistd.h>

#include "safe_rw.h"

#ifndef _WIN32
ssize_t
safe_write(const int fd, const void *const buf