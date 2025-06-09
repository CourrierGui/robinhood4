# This file is part of RobinHood 4
# Copyright (C) 2025 Commissariat a l'energie atomique et aux energies
#                    alternatives
#
# SPDX-License-Identifer: LGPL-3.0-or-later

rbhdiff_include = include_directories('include')

subdir('tests')

executable(
    'rbh-diff',
    sources: [
        'rbh-diff.c',
    ],
    dependencies: librobinhood_dep,
    include_directories: rbhdiff_include,
    install: true,
    c_args: '-DHAVE_CONFIG_H',
)
