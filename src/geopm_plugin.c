/*
 * Copyright (c) 2015, 2016, 2017, 2018, Intel Corporation
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
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dlfcn.h>
#ifndef u_short
#define u_short unsigned short
#endif
#include <fts.h>

#include "geopm_plugin.h"
#include "geopm_env.h"
#include "config.h"

#ifndef NAME_MAX
#define NAME_MAX 1024
#endif

int geopm_plugin_load(int plugin_type, struct geopm_factory_c *factory)
{
    int err = 0;
    void *plugin;
    int (*register_func)(int, struct geopm_factory_c *, void *);
    int fts_options = FTS_COMFOLLOW | FTS_NOCHDIR;
    FTS *p_fts;
    FTSENT *file;
    int num_path = 1;
    char **paths = NULL;
    char *default_path = GEOPM_PLUGIN_PATH;
    char path_env[NAME_MAX] = {0};
    if (strlen(geopm_env_plugin_path())) {
        ++num_path;
        strncpy(path_env, geopm_env_plugin_path(), NAME_MAX - 1);
        char *path_ptr = path_env;
        while ((path_ptr = strchr(path_ptr, ':'))) {
            *path_ptr = '\0';
            ++num_path;
        }
    }
    paths = calloc(num_path + 1, sizeof(char *));
    if (!paths) {
        err = ENOMEM;
    }
    if (!err) {
        paths[0] = default_path;
        char *path_ptr = path_env;
        for (int i = 1; i < num_path; ++i) {
            paths[i] = path_ptr;
            path_ptr += strlen(path_ptr) + 1;
        }

        if ((p_fts = fts_open(paths, fts_options, NULL)) != NULL) {
            while ((file = fts_read(p_fts)) != NULL) {
                if (file->fts_info == FTS_F &&
                    (strstr(file->fts_name, ".so") ||
                     strstr(file->fts_name, ".dylib"))) {
                    plugin = dlopen(file->fts_path, RTLD_LAZY);
                    if (plugin != NULL) {
                        register_func = (int (*)(int, struct geopm_factory_c *, void *)) dlsym(plugin, "geopm_plugin_register");
                        if (register_func != NULL) {
                            err = register_func(plugin_type, factory, plugin);
                        }
                        else {
                            dlclose(plugin);
                        }
                    }
                    else {
                        err = -1;
#ifdef GEOPM_DEBUG
                        fprintf(stderr,"Error dlopen(): %s\n", dlerror());
#endif
                    }
                }
            }
            fts_close(p_fts);
        }
        free(paths);
    }

    return err;
}
