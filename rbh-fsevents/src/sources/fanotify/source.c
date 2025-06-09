/* SPDX-License-Identifier: LGPL-3.0-or-later */

#include "fanotify.h"

struct fanotify_source {
    struct source source;
};

static const void *
fanotify_iter_next(void *iterator)
{
    errno = ENODATA;
    return NULL;
}

static void
fanotify_iter_destroy(void *iterator)
{
    free(iterator);
}

static const struct rbh_iterator_operations SOURCE_ITER_OPS = {
    .next = fanotify_iter_next,
    .destroy = fanotify_iter_destroy,
};

static const struct source FANOTIFY_SOURCE = {
    .name = "fanotify",
    .fsevents = {
        .ops = &SOURCE_ITER_OPS,
    },
};

struct source *
source_from_fanotify(void)
{
    struct fanotify_source *source;

    source = malloc(sizeof(*source));
    if (source == NULL)
        error(EXIT_FAILURE, errno, "malloc");

    source->source = FANOTIFY_SOURCE;
    return &source->source;
}
