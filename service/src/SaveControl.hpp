/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SAVECONTROL_HPP_INCLUDE
#define SAVECONTROL_HPP_INCLUDE

#include <memory>
#include <vector>
#include <string>
#include <set>

namespace geopm
{
    class IOGroup;
    class PlatformTopo;

    /// @brief Class that enables the save/restore feature for IOGroups
    ///
    /// This is a helper class that can be used by IOGroups to enable
    /// control settings to be stored to and loaded from disk in JSON
    /// format.  Additionally it can write all the settings to
    /// facilitate the restore.
    ///
    /// The JSON format for the data structure is a list of maps.
    /// Each map represents a m_setting_s structure by mapping a
    /// string naming the structure field to the value.  An example
    /// JSON string follows
    ///
    ///     [{"name": "MSR::PERF_CTL:FREQ",
    ///       "domain_type": 2,
    ///       "domain_idx": 0,
    ///       "setting": 2.4e9},
    ///      {"name": "MSR::PERF_CTL:FREQ",
    ///       "domain_type": 2,
    ///       "domain_idx": 1,
    ///       "setting": 2.4e9}]
    ///
    class SaveControl
    {
        public:
            /// @brief Structure that holds the parameters passed to
            ///        the IOGroup::write_control() method.
            struct m_setting_s {
                /// @brief Name of the control
                std::string name;
                /// @brief Domain to apply the setting
                int domain_type;
                /// @brief Index of the domain to apply the setting
                int domain_idx;
                /// @brief Value for restoring the control
                double setting;
            };
            SaveControl() = default;
            virtual ~SaveControl() = default;
            /// @brief Create a unique pointer to SaveControl object
            ///        from a vector of setting structures
            ///
            /// This method enables the construction when the user
            /// wants explicit control of the setting parameters.
            ///
            /// @param settings Vector of setting structures for all
            ///        controls to be restored
            ///
            /// @return A unique pointer to an object that implements
            ///         the SaveControl interface.
            static std::unique_ptr<SaveControl> make_unique(const std::vector<m_setting_s> &settings);
            /// @brief Create a unique pointer to SaveControl object
            ///        from a JSON formatted string
            ///
            /// @param json_string [in] A JSON representation of a vector
            ///                    of m_setting_s structures.
            ///
            /// @return A unique pointer to an object that implements
            ///         the SaveControl interface.
            static std::unique_ptr<SaveControl> make_unique(const std::string &json_string);
            /// @brief Create a unique pointer to SaveControl object
            ///        by querying an IOGroup
            ///
            /// A list of all low level control names is determined
            /// based on the control_names() return values that are
            /// within the IOGroup namespace.  The corresponding
            /// signal is read for all these low level controls at
            /// their native domain.  The values that are read are
            /// stored in the SaveControl object that is returned.
            ///
            /// @param io_group [in] An IOGroup that implements controls
            ///
            /// @return A unique pointer to an object that implements
            ///         the SaveControl interface.
            static std::unique_ptr<SaveControl> make_unique(IOGroup &io_group);
            /// @brief Get saved control settings JSON
            ///
            /// @return A JSON representation of a vector of
            ///         m_setting_s structures
            virtual std::string json(void) const = 0;
            /// @brief Get saved control settings structures
            ///
            /// @return Vector of setting structures that represent
            ///         the saved control state.
            virtual std::vector<m_setting_s> settings(void) const = 0;
            /// @brief Write the JSON formatted settings to a file
            ///
            /// Writes the string to the specified output file.  The
            /// file is overwritten if it already exists.  An
            /// exception is raised if the directory containing the
            /// output does not exist, or if the file cannot be
            /// created for any other reason.
            ///
            /// @param save_path [in] The file path where the JSON
            ///                  string is written.
            virtual void write_json(const std::string &save_path) const = 0;
            /// @brief Write all of the control settings to the platform
            ///
            /// Make a sequence of calls to io_group.write_control() with
            /// the parameters returned by the settings() method.
            ///
            /// @param io_group [in] An IOGroup that implements controls
            virtual void restore(IOGroup &io_group) const = 0;
            virtual std::set<std::string> unsaved_controls(void) const = 0;
    };

    class SaveControlImp : public SaveControl
    {
        public:
            SaveControlImp(const std::vector<m_setting_s> &settings);
            SaveControlImp(const std::string &json_string);
            SaveControlImp(IOGroup &io_group, const PlatformTopo &topo);
            virtual ~SaveControlImp() = default;
            std::string json(void) const override;
            std::vector<m_setting_s> settings(void) const override;
            void write_json(const std::string &save_path) const override;
            void restore(IOGroup &io_group) const override;
            std::set<std::string> unsaved_controls(void) const override;

            static std::string json(const std::vector<m_setting_s> &settings);
            static std::vector<m_setting_s> settings(const std::string &json_string);
            static std::vector<m_setting_s> settings(IOGroup &io_group,
                                                     const PlatformTopo &topo);
        private:
            const std::vector<m_setting_s> m_settings;
            const std::string m_json;
    };
}

#endif
