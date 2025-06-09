/* This file is part of Robinhood 4
 * Copyright (C) 2025 Commissariat a l'energie atomique et aux energies
 *                    alternatives
 *
 * SPDX-License-Identifer: LGPL-3.0-or-later
 */

#ifndef RBH_DIFF
#define RBH_DIFF

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <errno.h>
#include <error.h>
#include <robinhood.h>
#include <sysexits.h>

#include <robinhood/config.h>

void
diff(char **uris, struct rbh_backend **backends, size_t count, bool checksum);

#endif
