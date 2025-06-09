/* SPDX-License-Identifier: LGPL-3.0-or-later */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <assert.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/fanotify.h>
#include <unistd.h>

#include "source.h"

#define fanotify_event_len(buffer) \
    ((struct fanotify_event_metadata *)buffer)->event_len

void
fanotify_print(const char *buffer);
