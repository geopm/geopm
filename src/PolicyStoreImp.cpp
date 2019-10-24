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

#include "config.h"
#ifdef GEOPM_ENABLE_BETA

#include "PolicyStoreImp.hpp"

#include <cmath>
#include <functional>
#include <iostream>
#include <memory>
#include <sstream>

#include <sqlite3.h>

#include "Agent.hpp"
#include "Environment.hpp"
#include "Exception.hpp"
#include "Helper.hpp"
#include "geopm_error.h"

namespace geopm
{
    // Many SQLite string interfaces accept a size argument for strings. A negative
    // size can be provided when the string is known to be terminated.
    static const int SQLITE_TERMINATED_STRING = -1;

    static const char CREATE_TABLES[] =
        "CREATE TABLE IF NOT EXISTS DefaultPolicies("
        " agent TEXT NOT NULL,"
        " offset INTEGER NOT NULL,"
        " value REAL,"
        " PRIMARY KEY (agent, offset)"
        "); "
        "CREATE TABLE IF NOT EXISTS BestPolicies("
        " profile TEXT NOT NULL,"
        " agent TEXT NOT NULL,"
        " offset INTEGER NOT NULL,"
        " value REAL,"
        " PRIMARY KEY (profile, agent, offset)"
        ");";

    // Throw an exception due to an SQLite error. Appends the SQLite error string
    // to the error message.
    [[noreturn]] static void
    throw_sqlite_error(int sqlite_ret, const std::string &context_message, int line)
    {
        std::ostringstream oss;
        oss << context_message << ": " << sqlite3_errstr(sqlite_ret);
        throw Exception(oss.str(), GEOPM_ERROR_DATA_STORE, __FILE__, line);
    }

    // Given a ready-to-execute statement that will report policy offsets and
    // values, return the policy vector.
    static std::vector<double> sqlite_results_to_policy_vector(sqlite3 *database,
                                                               sqlite3_stmt *statement)
    {
        std::vector<double> policy;
        int sqlite_ret;
        while ((sqlite_ret = sqlite3_step(statement)) == SQLITE_ROW) {
            auto offset = sqlite3_column_int(statement, 0);
            auto value = sqlite3_column_type(statement, 1) == SQLITE_NULL
                             ? NAN
                             : sqlite3_column_double(statement, 1);

            // Fill in NaN gaps if the policy is stored in a sparse manner
            std::fill_n(std::back_inserter(policy), offset - policy.size(), NAN);

            policy.push_back(value);
        }

        if (sqlite_ret != SQLITE_DONE) {
            throw_sqlite_error(sqlite_ret, "Error querying policies", __LINE__);
        }

        return policy;
    }

    // Create a smart pointer to a SQLite statement that releases the sqlite
    // resources when ownership of the pointer is released.  Throws if a SQLite
    // error is encountered.
    typedef std::unique_ptr<sqlite3_stmt, std::function<void(sqlite3_stmt *)> > UniqueStatement;
    static UniqueStatement make_statement(sqlite3 *database, const char *statement)
    {
        // Prepared statements only handle the first SQL statement in a
        // multi-statement string. They can point to the start of the next
        // statement if desired. This NULL pointer indicates that the caller
        // does not need to know.
        static const std::nullptr_t IGNORE_UNUSED_STATEMENTS = nullptr;

        sqlite3_stmt *statement_raw_ptr;
        int sqlite_ret =
            sqlite3_prepare_v2(database, statement, SQLITE_TERMINATED_STRING,
                               &statement_raw_ptr, IGNORE_UNUSED_STATEMENTS);

        if (sqlite_ret != SQLITE_OK) {
            // Statements can still have associated resources on failure to
            // create. Free them.
            sqlite3_finalize(statement_raw_ptr);
            throw_sqlite_error(sqlite_ret, "Error building statement", __LINE__);
        }

        return UniqueStatement(statement_raw_ptr, [](sqlite3_stmt *statement) {
            static_cast<void>(sqlite3_finalize(statement));
        });
    }

    // Bind text to a sqlite statement
    static int bind_value_to_statement(sqlite3_stmt *statement, int index,
                                       const std::string &text)
    {
        // SQLite statement bindings can automatically free text or blob pointers
        // after binding. This constant can be used in place of the destructor
        // pointer in cases where the caller maintains ownership of the pointer.
        static const std::nullptr_t UNUSED_STRING_DESTRUCTOR = nullptr;

        return sqlite3_bind_text(statement, index, text.c_str(),
                                 SQLITE_TERMINATED_STRING, UNUSED_STRING_DESTRUCTOR);
    }

    // Bind an integer to a sqlite statement
    static int bind_value_to_statement(sqlite3_stmt *statement, int index, int value)
    {
        return sqlite3_bind_int(statement, index, value);
    }

    // Bind a double to a sqlite statement
    static int bind_value_to_statement(sqlite3_stmt *statement, int index, double value)
    {
        return sqlite3_bind_double(statement, index, value);
    }

    // Prevent dispatching by implicit conversion of the value.
    template<typename T>
    void bind_value_to_statement(sqlite3_stmt *, int, T) = delete;

    // Bind a value to a sqlite statement. Throw an exception if it fails.
    template<typename T>
    static void bind_value_or_throw(sqlite3_stmt *statement, int index,
                                    const T &value, int line)
    {
        int sqlite_ret = bind_value_to_statement(statement, index, value);
        if (sqlite_ret != SQLITE_OK) {
            throw_sqlite_error(sqlite_ret, "Error binding value to statement", line);
        }
    }

    // Try to get a policy from the BestPolicies table. If none is found, an
    // empty vector is returned.
    static std::vector<double>
    get_policy_from_best_policies(sqlite3 *database, const std::string &profile_name,
                                  const std::string &agent_name)
    {
        static const char SELECT_BEST_POLICY[] =
            "SELECT offset,value "
            "FROM BestPolicies "
            "WHERE profile = ?1 AND agent = ?2;";
        auto statement = make_statement(database, SELECT_BEST_POLICY);

        bind_value_or_throw(statement.get(), 1, profile_name, __LINE__);
        bind_value_or_throw(statement.get(), 2, agent_name, __LINE__);

        return sqlite_results_to_policy_vector(database, statement.get());
    }

    // Try to get an agent's policy from a given table. Return true if the
    // policy is successfully obtained, or false if it does not exist.
    // Exceptions are thrown for any errors.
    static std::vector<double> get_default(sqlite3 *database, const std::string &agent_name)
    {
        static const char SELECT_default[] =
            "SELECT offset,value "
            "FROM DefaultPolicies "
            "WHERE agent = ?1;";
        auto statement = make_statement(database, SELECT_default);

        bind_value_or_throw(statement.get(), 1, agent_name, __LINE__);

        return sqlite_results_to_policy_vector(database, statement.get());
    }

    // Begin a sqlite transaction. Throw an exception if it fails to begin.
    static void begin_transaction_or_throw(sqlite3 *database)
    {
        char *sqlite_error_message = 0;
        int sqlite_ret = sqlite3_exec(database, "BEGIN TRANSACTION;", nullptr,
                                      nullptr, &sqlite_error_message);
        if (sqlite_ret != SQLITE_OK) {
            std::ostringstream oss;
            oss << "Error beginning a transaction: " << sqlite_error_message;
            sqlite3_free(sqlite_error_message);
            throw Exception(oss.str(), GEOPM_ERROR_DATA_STORE, __FILE__, __LINE__);
        }
    }

    // Commit a sqlite transaction. Throw an exception if it fails to commit.
    static void commit_transaction_or_throw(sqlite3 *database)
    {
        char *sqlite_error_message = 0;
        int sqlite_ret = sqlite3_exec(database, "COMMIT TRANSACTION;", nullptr,
                                      nullptr, &sqlite_error_message);
        if (sqlite_ret != SQLITE_OK) {
            std::ostringstream oss;
            oss << "Error committing a transaction: " << sqlite_error_message;
            sqlite3_free(sqlite_error_message);
            throw Exception(oss.str(), GEOPM_ERROR_DATA_STORE, __FILE__, __LINE__);
        }
    }

    PolicyStoreImp::PolicyStoreImp(const std::string &database_path)
        : m_database(0)
    {
        auto ret = sqlite3_open(database_path.c_str(), &m_database);
        if (ret != SQLITE_OK) {
            std::ostringstream oss;
            oss << "Error opening " << database_path << ": " << sqlite3_errstr(ret);

            // Close may be necessary to release resources even on failure to
            // open. Close is only expected to fail due to unfinalized
            // statements or unfinished backups. Neither applies here.
            static_cast<void>(sqlite3_close(m_database));
            throw Exception(oss.str(), GEOPM_ERROR_DATA_STORE, __FILE__, __LINE__);
        }

        char *sqlite_error_message = 0;
        int sqlite_ret = sqlite3_exec(m_database, CREATE_TABLES, nullptr,
                                      nullptr, &sqlite_error_message);
        if (sqlite_ret != SQLITE_OK) {
            std::ostringstream oss;
            oss << "Error creating tables: " << sqlite_error_message;
            sqlite3_free(sqlite_error_message);
            static_cast<void>(sqlite3_close(m_database));
            throw Exception(oss.str(), GEOPM_ERROR_DATA_STORE, __FILE__, __LINE__);
        }
    }

    PolicyStoreImp::~PolicyStoreImp()
    {
        auto ret = sqlite3_close(m_database);
        if (ret != SQLITE_OK) {
            std::cerr << "Warning: <geopm> PolicyStore: Error while closing database. "
                      << sqlite3_errstr(ret) << std::endl;
        }
    }

    std::vector<double> PolicyStoreImp::get_best(const std::string &profile_name,
                                                 const std::string &agent_name) const
    {
        auto policy = get_policy_from_best_policies(m_database, profile_name, agent_name);
        if (policy.empty()) {
            policy = get_default(m_database, agent_name);
        }

        size_t policy_value_count =
            Agent::num_policy(agent_factory().dictionary(agent_name));
        if (policy.empty() && policy_value_count != 0) {
            std::ostringstream oss;
            oss << "No policy found for profile " << profile_name
                << " with agent " << agent_name;
            throw Exception(oss.str(), GEOPM_ERROR_DATA_STORE, __FILE__, __LINE__);
        }

        if (policy.size() < policy_value_count) {
            // Pad any non-set policy values with NAN so they will use the
            // agent's default values.
            policy.resize(policy_value_count, NAN);
        }
        return policy;
    }

    void PolicyStoreImp::set_best(const std::string &profile_name,
                                  const std::string &agent_name,
                                  const std::vector<double> &policy)
    {
        begin_transaction_or_throw(m_database);
        {
            // Remove existing policy values for this record in case the new
            // policy does not explicitly overwrite all values.
            auto statement = make_statement(
                m_database, "DELETE FROM BestPolicies WHERE profile=?1 AND agent=?2;");

            bind_value_or_throw(statement.get(), 1, profile_name, __LINE__);
            bind_value_or_throw(statement.get(), 2, agent_name, __LINE__);

            int sqlite_ret = sqlite3_step(statement.get());
            if (sqlite_ret != SQLITE_DONE) {
                throw_sqlite_error(
                    sqlite_ret, "Error replacing an existing policy", __LINE__);
            }
        }
        for (size_t offset = 0; offset < policy.size(); ++offset) {
            auto statement = make_statement(
                m_database,
                "INSERT INTO BestPolicies "
                "(profile, agent, offset, value) VALUES (?1, ?2, ?3, ?4);");

            bind_value_or_throw(statement.get(), 1, profile_name, __LINE__);
            bind_value_or_throw(statement.get(), 2, agent_name, __LINE__);
            bind_value_or_throw(statement.get(), 3, static_cast<int>(offset), __LINE__);
            bind_value_or_throw(statement.get(), 4, policy[offset], __LINE__);

            int sqlite_ret = sqlite3_step(statement.get());
            if (sqlite_ret != SQLITE_DONE) {
                throw_sqlite_error(sqlite_ret, "Error setting the best policy", __LINE__);
            }
        }
        commit_transaction_or_throw(m_database);
    }

    void PolicyStoreImp::set_default(const std::string &agent_name,
                                     const std::vector<double> &policy)
    {
        begin_transaction_or_throw(m_database);
        {
            // Remove existing policy values for this record in case the new
            // policy does not explicitly overwrite all values.
            auto statement = make_statement(
                m_database, "DELETE FROM DefaultPolicies WHERE agent=?1;");

            bind_value_or_throw(statement.get(), 1, agent_name, __LINE__);

            int sqlite_ret = sqlite3_step(statement.get());
            if (sqlite_ret != SQLITE_DONE) {
                throw_sqlite_error(
                    sqlite_ret, "Error replacing an existing policy", __LINE__);
            }
        }
        for (size_t offset = 0; offset < policy.size(); ++offset) {
            auto statement =
                make_statement(m_database,
                               "INSERT INTO DefaultPolicies "
                               "(agent, offset, value) VALUES (?1, ?2, ?3);");

            bind_value_or_throw(statement.get(), 1, agent_name, __LINE__);
            bind_value_or_throw(statement.get(), 2, static_cast<int>(offset), __LINE__);
            bind_value_or_throw(statement.get(), 3, policy[offset], __LINE__);

            int sqlite_ret = sqlite3_step(statement.get());
            if (sqlite_ret != SQLITE_DONE) {
                throw_sqlite_error(sqlite_ret,
                                   "Error setting the default policy", __LINE__);
            }
        }
        commit_transaction_or_throw(m_database);
    }
}

#endif // GEOPM_ENABLE_BETA
