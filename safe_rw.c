
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
safe_write(const int fd, const void *const buf_, size_t count,
           const int timeout)
{
    struct pollfd pfd;
    const char *buf = (const char *)buf_;
    ssize_t written;

    pfd.fd = fd;
    pfd.events = POLLOUT;

    wh