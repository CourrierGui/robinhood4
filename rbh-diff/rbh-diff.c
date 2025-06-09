/* This file is part of Robinhood 4
 * Copyright (C) 2025 Commissariat a l'energie atomique et aux energies
 *                    alternatives
 *
 * SPDX-License-Identifer: LGPL-3.0-or-later
 */

#include "rbh-diff.h"

#include <getopt.h>

static void
usage(void)
{
    const char *message =
        "usage: %s [-h] [-c PATH] URI1 URI2 [URI...]\n"
        "\n"
        "Display the differences between the entries found under each URI provided.\n"
        "At least 2 URI must be provided.\n"
        "\n"
        "Optional arguments:\n"
        "    -c,--config PATH      the configuration file to use\n"
        "    -h,--help             show this message and exit\n";


    printf(message, program_invocation_short_name);
}

static bool
backend_supports_filter(const char *name)
{
    const struct rbh_backend_plugin *plugin;
    bool has_filter;

    plugin = rbh_backend_plugin_import(name);
    if (!plugin)
        error(EXIT_FAILURE, errno, "failed to import plugin '%s'", name);

    has_filter = (plugin->capabilities & RBH_FILTER_OPS);

    return has_filter;
}

static struct rbh_backend **
open_backends(size_t count, char **uris)
{
    struct rbh_backend **backends;

    backends = malloc(sizeof(*backends) * count);
    if (!backends)
        error(EXIT_FAILURE, errno, "failed to allocate backends");

    for (size_t i = 0; i < count; i++) {
        backends[i] = rbh_backend_from_uri(uris[i], true);
        if (!backends[i])
            error(EXIT_FAILURE, errno, "failed to open backend '%s'", uris[i]);

        if (!backend_supports_filter(backends[i]->name))
            error(EXIT_FAILURE, 0, "uri '%s' does not support filtering",
                  uris[i]);
    }

    return backends;
}

static void
close_backends(struct rbh_backend **backends, size_t count)
{
    for (size_t i = 0; i < count; i++) {
        // /!\ this works because backends usually use a static string for the name!
        const char *name = backends[i]->name;

        rbh_backend_destroy(backends[i]);
        rbh_backend_plugin_destroy(name);
    }
}

int
main(int argc, char **argv)
{
    const struct option long_options[] = {
        { .name = "help",   .val = 'h' },
        { .name = "config", .val = 'c' },
        { },
    };
    struct rbh_backend **backends;
    char c;
    int rc;

    rc = rbh_config_from_args(argc - 1, argv + 1);
    if (rc)
        error(EXIT_FAILURE, errno, "failed to open configuration file");

    while ((c = getopt_long(argc, argv, "c:h", long_options,
                            NULL)) != -1) {
        switch (c) {
        case 'c':
            /* already parsed */
            break;
        case 'h':
            usage();
            return 0;
        case '?':
        default:
            /* getopt_long() prints meaningful error messages itself */
            exit(EX_USAGE);
        }
    }

    argc -= optind;
    argv += optind;

    if (argc < 2)
        error(EX_USAGE, 0, "not enough arguments, at least 2 URI must be provided.");

    backends = open_backends(argc, argv);
    assert(backends);

    close_backends(backends, argc);

    return 0;
}
