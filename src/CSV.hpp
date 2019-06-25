/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation
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

#include <vector>
#include <map>
#include <string>
#include <fstream>
#include <sstream>

namespace geopm
{
    class CSV
    {
        public:
            CSV() = default;
            virtual ~CSV() = default;
            virtual void add_column(const std::string &name) = 0;
            virtual void add_column(const std::string &name, const std::string &format) = 0;
            virtual void activate(void) = 0;
            virtual void update(const std::vector<double> &sample) = 0;
            virtual void flush(void) = 0;
    };

    class CSVImp : public CSV
    {
        public:
            CSVImp(const std::string &file_path, const std::string &host_name, const std::string &start_time);
            virtual ~CSVImp();
            void add_column(const std::string &name) override;
            void add_column(const std::string &name, const std::string &format) override;
            void activate(void) override;
            void update(const std::vector<double> &sample) override;
            void flush(void) override;
        private:
            enum {
                M_FORMAT_DOUBLE,
                M_FORMAT_FLOAT,
                M_FORMAT_INTEGER,
                M_FORMAT_HEX,
                M_FORMAT_RAW64,
                M_NUM_FORMAT
            } m_format_e;

            void write_header(const std::string &host_name, const std::string &start_time);
            void write_names(void);

            const std::map<std::string, int> M_NAME_FORMAT_MAP;
            const char M_SEPARATOR;
            std::string m_file_path;
            std::vector<std::string> m_column_name;
            std::vector<int> m_column_format;
            std::ofstream m_stream;
            std::ostringstream m_buffer;
            off_t m_buffer_limit;
            bool m_is_active;
    };
}
