/*
 * Copyright (c) 2015, 2016, 2017, Intel Corporation
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

#include <mpi.h>
#include <unistd.h>
#include <iostream>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory>
#include <string>
#include <sstream>
#include <fstream>
#include <dirent.h>

#include "gtest/gtest.h"
#include "geopm_env.h"

#ifndef NAME_MAX
#define NAME_MAX 256
#endif

class MPIControllerDeathTest: public :: testing :: Test
{
    public:
        std::vector<int> getProcessList(void);
        bool areShmKeysPresent(void);
        bool isMessageInLog(std::string);
};


std::vector<int> MPIControllerDeathTest::getProcessList(void)
{
    char buf[512];
    std::string output;
    std::shared_ptr<FILE> std_out(popen("pgrep -fu `whoami` MPIControllerDeathTest", "r"),  pclose);
    std::string token;

    while (!feof(std_out.get())) {
        if (fgets(buf, 512, std_out.get()) != NULL) {
            output += buf;
        }
    }

    std::istringstream stream(output);
    std::vector<int> pids;
    while (std::getline(stream, token, '\n')) {
        pids.push_back(std::stoi(token, nullptr));
    }

    return pids;
}

bool MPIControllerDeathTest::areShmKeysPresent(void)
{
    DIR *did = opendir("/dev/shm");

    if (did &&
        strlen(geopm_env_shmkey()) &&
        *(geopm_env_shmkey()) == '/' &&
        strchr(geopm_env_shmkey(), ' ') == NULL &&
        strchr(geopm_env_shmkey() + 1, '/') == NULL) {

        struct dirent *entry;
        char shm_key[NAME_MAX];
        shm_key[0] = '/';
        shm_key[NAME_MAX - 1] = '\0';
        while ((entry = readdir(did))) {
            if (strstr(entry->d_name, geopm_env_shmkey() + 1) == entry->d_name) {
               strncpy(shm_key + 1, entry->d_name, NAME_MAX - 2);
                return true;
            }
        }
    }
    return false;
}

bool MPIControllerDeathTest::isMessageInLog(std::string message)
{
    std::ifstream ifs ("test/gtest_links/MPIControllerDeathTest.shm_clean_up.log", std::ifstream::in);
    if (!ifs) {
        std::cerr << "ERROR: Unable to open log file!!" << std::endl;
        return false;
    }

    size_t pos;
    std::string line;
    while (ifs.good() && !ifs.eof()) {
        std::getline(ifs, line);
        pos = line.find(message);
        if (pos != std::string::npos) {
            ifs.close();
            return true;
        }
    }

    ifs.close();
    return false;
}

TEST_F(MPIControllerDeathTest, shm_clean_up)
{
    std::vector<int> pids = getProcessList();
    // Give the controller a moment to initialize before we try to kill it.
    sleep(5);

    // This only works properly with 2 ranks: 1 for the controller and 1 for this
    // test.  Any more than that and we'd need code here to track which rank is trying
    // to issue the kill so the non-controller ranks don't kill each other.
    kill(pids.front(), SIGINT); // Assuming that the lowest PID is the controller.

    sleep(5); // Give the controller a moment to handle the signal and do its clean up.
    EXPECT_FALSE(areShmKeysPresent());

    std::ostringstream message;
    message << "Error: <geopm> Runtime error: Signal " << SIGINT << " raised";
    EXPECT_TRUE(isMessageInLog(message.str()));
}

