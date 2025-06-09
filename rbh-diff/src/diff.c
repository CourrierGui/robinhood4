/* This file is part of Robinhood 4
 * Copyright (C) 2025 Commissariat a l'energie atomique et aux energies
 *                    alternatives
 *
 * SPDX-License-Identifer: LGPL-3.0-or-later
 */

#include "rbh-diff.h"

#include <sys/stat.h>

struct rbh_filter_sort PATH_FILTER = {
    .field = {
        .fsentry = RBH_FP_NAMESPACE_XATTRS,
        .xattr = "path",
    },
    .ascending = true,
};

struct backend_stream {
    struct rbh_mut_iterator *fsentries;
    struct rbh_fsentry *current_entry;
    bool ended;
};

struct backend_streams {
    struct rbh_backend **backends;
    char **uris;
    struct backend_stream *streams;
    size_t count;
};

static struct backend_streams *
open_streams(char **uris, struct rbh_backend **backends, size_t count)
{
    struct rbh_filter_options options = {
        .sort.items = &PATH_FILTER,
        .sort.count = 1,
    };
    struct backend_streams *streams;
    int save_errno;
    size_t i;

    streams = malloc(sizeof(*streams));
    if (!streams)
        return NULL;

    streams->uris = uris;
    streams->streams = calloc(count, sizeof(*streams->streams));
    save_errno = errno;
    if (!streams->streams)
        goto free_streams;

    streams->backends = backends;
    streams->count = count;
    for (i = 0; i < count; i++) {
        streams->streams[i].fsentries = rbh_backend_filter(backends[i],
                                                           NULL,
                                                           &options,
                                                           NULL);
        save_errno = errno;
        if (!streams->streams[i].fsentries)
            goto free_iterators;

        streams->streams[i].current_entry =
            rbh_mut_iter_next(streams->streams[i].fsentries);
        if (!streams->streams[i].current_entry)
            streams->streams[i].ended = true;
    }

    return streams;

free_iterators:
    for (size_t j = 0; j < i; j++)
        rbh_mut_iter_destroy(streams->streams[j].fsentries);
    free(streams->streams);
free_streams:
    free(streams);
    errno = save_errno;
    return NULL;
}

static void
streams_free(struct backend_streams *streams)
{
    for (size_t i = 0; i < streams->count; i++)
        rbh_mut_iter_destroy(streams->streams[i].fsentries);
    free(streams->streams);
    free(streams);
}

static const char *
current_stream_path(struct backend_streams *streams, size_t i)
{
    const struct rbh_fsentry *fsentry = streams->streams[i].current_entry;

    return rbh_fsentry_path(fsentry);
}

static size_t
min_fsentries(struct backend_streams *streams, bool *mins)
{
    size_t count = 0;
    size_t min = 0;

    while (!streams->streams[min].current_entry)
        min++;

    if (min == streams->count)
        return 0;

    mins[min] = true;
    count++;

    for (size_t i = min + 1; i < streams->count; i++) {
        if (!streams->streams[i].current_entry)
            continue;

        int res = strcmp(current_stream_path(streams, min),
                         current_stream_path(streams, i));
        if (res == 0) {
            count++;
            mins[i] = true;
            continue;
        }

        if (res > 0) {
            memset(mins, false, sizeof(*mins) * streams->count);

            count = 1;
            min = i;
            mins[i] = true;
        }
    }

    return count;
}

static bool
streams_next(struct backend_streams *streams, bool *mins)
{
    bool keep_going = false;

    for (size_t i = 0; i < streams->count; i++) {
        if (streams->streams[i].ended || !mins[i])
            continue;

        streams->streams[i].current_entry =
            rbh_mut_iter_next(streams->streams[i].fsentries);

        if (!streams->streams[i].current_entry)
            streams->streams[i].ended = true;

        if (!streams->streams[i].ended)
            keep_going = true;
    }

    return keep_going;
}

static void
missing_entry(struct backend_streams *streams, size_t i, const char *path)
{
    printf("%s: missing '%s'\n", streams->uris[i], path);
}

static const char *
find_minimal_path(struct backend_streams *streams, bool *mins)
{
    const struct rbh_fsentry *fsentry;
    size_t i = 0;

    while (!mins[i])
        i++;

    fsentry = streams->streams[i].current_entry;
    assert(fsentry);

    return rbh_fsentry_path(fsentry);
}

static const struct rbh_value *
checksum_xattr(struct rbh_fsentry *fsentry)
{
    const struct rbh_value *checksum =
        rbh_fsentry_find_inode_xattr(fsentry, "user.hash");

    if (!checksum) {
        errno = ENOENT;
        return NULL;
    }

    if (checksum->type != RBH_VT_BINARY &&
        checksum->type != RBH_VT_STRING) {
        errno = EFAULT;
        return NULL;
    }

    return checksum;
}

static bool match(const struct rbh_value *csum1,
                  const struct rbh_value *csum2)
{
    if (csum1->type != csum2->type)
        return false;

    if (csum1->type == RBH_VT_STRING)
        return !strcmp(csum1->string, csum2->string);

    if (csum1->binary.size != csum2->binary.size)
        return false;
    return !strncmp(csum1->binary.data, csum2->binary.data,
                    csum1->binary.size);
}

static void
checksum_match(struct backend_streams *streams, const char *path, bool *mins)
{
    const struct rbh_value *previous_checksum = NULL;
    bool all_match = true;

    for (size_t i = 0; i < streams->count; i++) {
        struct rbh_fsentry *fsentry;
        const struct rbh_value *checksum;

        if (!mins[i] || !streams->streams[i].current_entry)
            continue;

        fsentry = streams->streams[i].current_entry;
        if (!S_ISREG(fsentry->statx->stx_mode))
            continue;

        checksum = checksum_xattr(fsentry);
        if (!checksum) {
            switch (errno) {
            case EFAULT:
                printf("%s: '%s' invalid checksum xattr\n", streams->uris[i],
                       path);
                break;
            case ENOENT:
                printf("%s: '%s' missing checksum xattr\n", streams->uris[i], path);
                break;
            default:
                abort();
            }
        }

        all_match = all_match &&
            (previous_checksum ? match(previous_checksum, checksum) : true);
    }

    if (!all_match)
        printf("'%s' checksum missmatch\n", path);
}

static bool
check_mode(struct backend_streams *streams, bool *mins, const char *path)
{
    const struct rbh_fsentry *previous = NULL;

    for (size_t i = 0; i < streams->count; i++) {
        const struct rbh_fsentry *fsentry;

        if (!mins[i])
            continue;

        if (!previous) {
            previous = streams->streams[i].current_entry;
            assert(previous);
            continue;
        }

        fsentry = streams->streams[i].current_entry;
        if (previous->statx->stx_mode != fsentry->statx->stx_mode) {
            printf("'%s' mode mismatch", path);
            return false;
        }
    }

    return true;
}

static void
diff_entries(struct backend_streams *streams, bool *mins, bool checksum)
{
    const char *path = find_minimal_path(streams, mins);

    for (size_t i = 0; i < streams->count; i++) {
        if (mins[i])
            continue;

        missing_entry(streams, i, path);
    }

    if (!check_mode(streams, mins, path))
        return;

    if (checksum)
        checksum_match(streams, path, mins);
}

void
diff(char **uris, struct rbh_backend **backends, size_t count, bool checksum)
{
    struct backend_streams *streams;
    size_t min_count;
    bool *mins;

    streams = open_streams(uris, backends, count);
    if (!streams)
        error(EXIT_FAILURE, errno, "failed to build filters");

    mins = calloc(streams->count, sizeof(*mins));
    if (!mins)
        goto free_streams;

    while ((min_count = min_fsentries(streams, mins)) != 0) {
        diff_entries(streams, mins, checksum);

        streams_next(streams, mins);
        memset(mins, false, sizeof(*mins) * streams->count);
    }

free_streams:
    streams_free(streams);
}
