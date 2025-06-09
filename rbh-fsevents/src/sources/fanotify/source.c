/* SPDX-License-Identifier: LGPL-3.0-or-later */

#include "fanotify.h"

struct fanotify_source {
    struct source source;
    struct rbh_iterator iterator;
    int notifyfd;
    int rootfd;
    char buffer[4096];
};

static const void *
fanotify_iter_next(void *iterator)
{
    struct fanotify_source *source = iterator;
    ssize_t offset = 0;
    ssize_t rc;

    rc = read(source->notifyfd, source->buffer, sizeof(source->buffer));
    if (rc == -1)
        error(EXIT_FAILURE, errno, "failed to read next fanotify events");

    if (rc == 0) {
        errno = ENODATA;
        return NULL;
    }

    assert(rc >= fanotify_event_len(source->buffer));

    while (offset < rc) {
        fanotify_print(source->buffer + offset);
        offset += fanotify_event_len(source->buffer);
    }

    errno = EAGAIN;
    return NULL;
}

static void
fanotify_iter_destroy(void *iterator)
{
    free(iterator);
}

static const struct rbh_iterator_operations FANOTIFY_ITER_OPS = {
    .next = fanotify_iter_next,
    .destroy = fanotify_iter_destroy,
};

static const struct source FANOTIFY_SOURCE = {
    .name = "fanotify",
    .fsevents = {
        .ops = &FANOTIFY_ITER_OPS,
    },
};

struct source *
source_from_fanotify(const char *path)
{
    struct fanotify_source *source;

    source = malloc(sizeof(*source));
    if (source == NULL)
        error(EXIT_FAILURE, errno, "malloc");

    source->notifyfd = fanotify_init(FAN_CLASS_NOTIF |
                                      FAN_UNLIMITED_QUEUE |
                                      FAN_UNLIMITED_MARKS |
                                      /* inode file_handle in the event */
                                      FAN_REPORT_FID |
                                      /* parent file_handle on non dir */
                                      FAN_REPORT_DIR_FID |
                                      FAN_REPORT_NAME |
                                      FAN_REPORT_TARGET_FID,
                                      O_RDONLY | O_LARGEFILE | O_NOATIME);
    if (source->notifyfd == -1)
        error(EXIT_FAILURE, errno, "failed to create fanotify for '%s'", path);

    if ((source->rootfd = openat(AT_FDCWD, path, O_RDONLY | O_DIRECTORY)) == -1)
        error(EXIT_FAILURE, errno, "failed to open directory '%s'",
              path);

    if (fanotify_mark(source->notifyfd,
                      FAN_MARK_ADD | FAN_MARK_ONLYDIR,
                      FAN_ACCESS | FAN_MODIFY | FAN_CLOSE_WRITE | FAN_CLOSE_NOWRITE |
                      FAN_CREATE | FAN_DELETE | FAN_ATTRIB | FAN_RENAME | FAN_ONDIR,
                      source->rootfd, NULL) == -1)
        error(EXIT_FAILURE, errno, "failed to set fanotify mark for '%s'", path);

    source->source = FANOTIFY_SOURCE;
    return &source->source;
}
