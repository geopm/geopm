/*
 * Copyright (c) 2015, 2016, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY LOG OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dlfcn.h>
#ifndef u_short
#define u_short unsigned short
#endif
#include <fts.h>

#include "geopm_plugin.h"

int geopm_plugin_load(int plugin_type, struct geopm_factory_c *factory)
{
    int err = 0;
    void *plugin;
    int (*register_func)(int, struct geopm_factory_c *factory);
    int fts_options = FTS_COMFOLLOW | FTS_NOCHDIR;
    FTS *p_fts;
    FTSENT *file;
    char *plugin_dir = PLUGINDIR;
    char *paths[3];

    paths[0] = plugin_dir;
    paths[1] = getenv("GEOPM_PLUGIN_DIR");
    paths[2] = NULL;

    if ((p_fts = fts_open(paths, fts_options, NULL)) != NULL) {
        while ((file = fts_read(p_fts)) != NULL) {
            if (file->fts_info == FTS_F &&
                (strstr(file->fts_name, ".so") ||
                 strstr(file->fts_name, ".dylib"))) {
                plugin = dlopen(file->fts_path, RTLD_LAZY);
                if (plugin != NULL) {
                    register_func = (int (*)(int, struct geopm_factory_c *)) dlsym(plugin, "geopm_plugin_register");
                    if (register_func != NULL) {
                        register_func(plugin_type, factory);
                    }
                }
            }
        }
        fts_close(p_fts);
    }

    return err;
}
