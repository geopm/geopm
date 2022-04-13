/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef DOMAINCONTROL_HPP_INCLUDE
#define DOMAINCONTROL_HPP_INCLUDE

#include <vector>
#include <memory>

#include "Control.hpp"

namespace geopm
{
    class DomainControl : public Control
    {
        public:
            DomainControl(const std::vector<std::shared_ptr<Control> > &controls);
            DomainControl(const DomainControl &other) = delete;
            virtual ~DomainControl() = default;
            void setup_batch(void) override;
            void adjust(double value) override;
            void write(double value) override;
            void save(void) override;
            void restore(void) override;
        private:
            std::vector<std::shared_ptr<Control> > m_controls;
            bool m_is_batch_ready;
    };
}

#endif
