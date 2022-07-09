#ifndef LOGGER_H
#define LOGGER_H
/**
 * Logger
 *
 * @link http://en.wikipedia.org/wiki/Syslog
 */

#include "compat.h"

#ifndef _WIN32
#include <syslog.h>
#undef SYSLOG_NAMES
#else
#define	LOG_EMERG	0           /* system is unusable */
#define	LOG_ALERT	1       