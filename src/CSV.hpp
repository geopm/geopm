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
#include <functional>
#include <map>
#include <string>
#include <fstream>
#include <sstream>

namespace geopm
{
    /// @brief CSV class provides the GEOPM interface for creation of
    ///        character separated value tabular data files.  These
    ///        CSV formatted files are created with a header
    ///        containing some meta-data prefixed by the '#' character
    ///        and then one line that defines the field name for the
    ///        column.  The separation character is a '|' not a comma.
    class CSV
    {
        public:
            CSV() = default;
            virtual ~CSV() = default;
            /// @brief Add a column with the given field name.  The
            ///        formatting of the column values will default to
            ///        geopm::string_format_double().
            /// @param [in] name The field name for the column as it will
            ///        be printed in the file header.
            virtual void add_column(const std::string &name) = 0;
            /// @brief Add a column with the given field name.  The
            ///        formatting of the column values chosen based on
            ///        the format string.
            /// @param [in] name The field name for the column as it will
            ///        be printed in the file header.
            /// @param [in] format One of five format strings:
            ///        - "double": Floating point number with up to 16
            ///                    significant decimal digits.
            ///        - "float": Floating point number with up to 6
            ///                   significant decimal digits.
            ///        - "integer": Whole number printed in decimal.
            ///        - "hex": Whole number printed in hexadecimal
            ///                 digits with 16 digits of zero padding.
            ///        - "raw64": View of raw memory contained in
            ///                   signal printed as a 16 hexadecimal
            ///                   digit number.
            virtual void add_column(const std::string &name,
                                    const std::string &format) = 0;
            /// @brief Add a column with the given field name.  The
            ///        formatting of the column values is implemented
            ///        with the format function provided.
            /// @param [in] name The field name for the column as it will
            ///        be printed in the file header.
            /// @param [in] format Function that converts a double
            ///        precision signal into the printed string for
            ///        this column in the CSV file.
            virtual void add_column(const std::string &name,
                                    std::function<std::string(double)> format) = 0;
            /// @brief Calling activate indicates that all columns
            ///        have been added to the object and calls to
            ///        update() are enabled.
            virtual void activate(void) = 0;
            /// @brief Add a row to the CSV file.
            /// @param [in] sample Values for each column of the table
            ///        in the order that the columns were added prior
            ///        to calling activate().
            virtual void update(const std::vector<double> &sample) = 0;
            /// @brief Flush all output to the CSV file.
            virtual void flush(void) = 0;
    };

    class CSVImp : public CSV
    {
        public:
            CSVImp(const std::string &file_path,
                   const std::string &host_name,
                   const std::string &start_time,
                   size_t buffer_size);
            virtual ~CSVImp();
            void add_column(const std::string &name) override;
            void add_column(const std::string &name,
                            const std::string &format) override;
            void add_column(const std::string &name,
                            std::function<std::string(double)> format) override;
            void activate(void) override;
            void update(const std::vector<double> &sample) override;
            void flush(void) override;
        private:
            void write_header(const std::string &host_name, const std::string &start_time);
            void write_names(void);

            const std::map<std::string, std::function<std::string(double)> > M_NAME_FORMAT_MAP;
            const char M_SEPARATOR;
            std::string m_file_path;
            std::vector<std::string> m_column_name;
            std::vector<std::function<std::string(double)> > m_column_format;
            std::ofstream m_stream;
            std::ostringstream m_buffer;
            off_t m_buffer_limit;
            bool m_is_active;
    };
}
