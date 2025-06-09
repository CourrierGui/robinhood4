/* SPDX-License-Identifier: LGPL-3.0-or-later */

#include "fanotify.h"

static void
print_dfid_name(struct fanotify_event_info_header *header)
{
    struct fanotify_event_info_fid *fid = (void *)header;
    struct file_handle *handle = (void *)fid->handle;
    const char *name = (char *)handle->f_handle + handle->handle_bytes;

    printf("%s\n", name); 
}

static void
print_event(uint64_t mask)
{
    if (mask & FAN_ACCESS)
        printf("access ");
    if (mask & FAN_OPEN)
        printf("open ");
    if (mask & FAN_OPEN_EXEC)
        printf("open exec ");
    if (mask & FAN_ATTRIB)
        printf("attrib ");
    if (mask & FAN_CREATE)
        printf("create ");
    if (mask & FAN_DELETE)
        printf("delete ");
    if (mask & FAN_DELETE_SELF)
        printf("delete self ");
    if (mask & FAN_RENAME)
        printf("rename ");
    if (mask & FAN_MOVED_FROM)
        printf("moved from ");
    if (mask & FAN_MOVED_TO)
        printf("moved to ");
    if (mask & FAN_MOVE_SELF)
        printf("move self ");
    if (mask & FAN_MODIFY)
        printf("modify ");
    if (mask & FAN_CLOSE_WRITE)
        printf("close write ");
    if (mask & FAN_CLOSE_NOWRITE)
        printf("close nowrite ");
    if (mask & FAN_FS_ERROR)
        printf("fs error ");
    if (mask & FAN_Q_OVERFLOW)
        printf("q overflow ");
    if (mask & FAN_ACCESS_PERM)
        printf("access perm ");
    if (mask & FAN_OPEN_PERM)
        printf("open perm ");
    if (mask & FAN_OPEN_EXEC_PERM)
        printf("open exec perm ");
    if (mask & FAN_CLOSE)
        printf("close ");
    if (mask & FAN_MOVE)
        printf("move ");
    if (mask & FAN_ONDIR)
        printf("ondir ");
    printf("\n");
}

void
fanotify_print(const char *buffer)
{
    struct fanotify_event_metadata *meta = (struct fanotify_event_metadata *)buffer;
    struct fanotify_event_info_header *header =
        (struct fanotify_event_info_header *)(buffer + meta->metadata_len);

    printf("Event received %d (%u/%u bytes)\n",
           header->info_type,
           meta->metadata_len,
           meta->event_len);
    print_event(meta->mask);

    switch (header->info_type) {
    case FAN_EVENT_INFO_TYPE_FID:
        break;
    case FAN_EVENT_INFO_TYPE_DFID_NAME:
        print_dfid_name(header);
        break;
    case FAN_EVENT_INFO_TYPE_DFID:
        break;
    case FAN_EVENT_INFO_TYPE_PIDFD:
        break;
    case FAN_EVENT_INFO_TYPE_ERROR:
        break;
    case FAN_EVENT_INFO_TYPE_RANGE:
        break;
    default:
        error(EXIT_FAILURE, errno, "unmanaged event type '%u'", header->info_type);
    }
}
