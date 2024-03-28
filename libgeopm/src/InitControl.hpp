/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef INITCONTROL_HPP_INCLUDE
#define INITCONTROL_HPP_INCLUDE

#include "config.h"

#include <string>
#include <vector>
#include <memory>

namespace geopm
{
    class InitControl
    {
        public:
            static std::unique_ptr<InitControl> make_unique(void);
            InitControl() = default;
            virtual ~InitControl() = default;

            virtual void parse_input(const std::string &input_file) = 0;
            virtual void write_controls(void) const = 0;
    };

    class PlatformIO;

    class InitControlImp : public InitControl
    {
        public:
            InitControlImp(const InitControlImp &other) = delete;
            InitControlImp &operator=(const InitControlImp &other) = delete;

            InitControlImp();
            InitControlImp(PlatformIO &platform_io);
            virtual ~InitControlImp() = default;

            void parse_input(const std::string &input_file) override;
            void write_controls(void) const override;

        private:
            struct m_request_s {
                std::string name;
                int domain;
                int domain_idx;
                double setting;
            };

            PlatformIO &m_platform_io;
            std::vector<m_request_s> m_requests;
    };
}
#endif
