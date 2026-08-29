/*
 * Copyright (c) 2015 - 2025 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "PMTIOGroup.hpp"

#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>

#include <cstring>
#include <cerrno>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <time.h>

#include "geopm/Helper.hpp"
#include "geopm/Exception.hpp"
#include "geopm/PlatformTopo.hpp"
#include "geopm/Agg.hpp"

namespace geopm
{
    namespace {
        const char TELEMETRY_CLASS_DIR[] = "/sys/class/intel_pmt";
        // SPR PM aggregator unique id; used as a guard to ensure we match expected XML set
        const uint64_t SPR_PM_GUID = 0x9956f43full;
    }

    std::string PMTIOGroup::plugin_name(void)
    {
        return "PMT";
    }

    std::unique_ptr<IOGroup> PMTIOGroup::make_plugin(void)
    {
        return geopm::make_unique<PMTIOGroup>();
    }

    PMTIOGroup::PMTIOGroup()
        : m_telem_path{}
        , m_guid_str{}
        , m_guid_val(0)
        , m_telem_size_bytes(0)
        , m_fd(-1)
        , m_signal_def_by_name{}
        , m_pushed{}
    , m_num_cpu(geopm::platform_topo().num_domain(GEOPM_DOMAIN_CPU))
    , m_time_zero(time_zero())
    {
        // Optional override: GEOPM_PMT_DIRECT_RATE_SLEEP_NS to tune the direct read sleep for rate calculation
        if (const char *env = std::getenv("GEOPM_PMT_DIRECT_RATE_SLEEP_NS")) {
            try {
                long v = std::stol(env);
                if (v > 0 && v < 1000LL * 1000LL * 1000LL) { // cap at <1s
                    m_direct_rate_sleep_ns = v;
                }
            }
            catch (...) {}
        }
        discover_telem();
        discover_signals_from_xml();
        // Only apply SPR-specific minimal fallback when GUID matches
        if (m_signal_def_by_name.empty() && m_guid_val == SPR_PM_GUID) {
            build_minimal_field_map();
        }

    // Nothing else to register; composite and rate signals are virtual
    }

    PMTIOGroup::~PMTIOGroup()
    {
        if (m_fd >= 0) {
            (void)close(m_fd);
        }
    }

    void PMTIOGroup::discover_telem(void)
    {
        // Prefer SPR PM GUID if available; otherwise pick the first telem device.
        DIR *dir = opendir(TELEMETRY_CLASS_DIR);
        if (!dir) {
            throw Exception("PMTIOGroup: intel_pmt sysfs not found", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        struct dirent *ent;
        std::string telem_dir;
        std::string fallback_telem_dir;
        std::string fallback_guid_str;
        uint64_t fallback_guid_val = 0;
        size_t fallback_size_bytes = 0;
        while ((ent = readdir(dir)) != nullptr) {
            if (strncmp(ent->d_name, "telem", 5) != 0) continue;
            std::string path = std::string(TELEMETRY_CLASS_DIR) + "/" + ent->d_name;
            std::ifstream guid_if(path + "/guid");
            std::ifstream size_if(path + "/size");
            if (!guid_if.good() || !size_if.good()) continue;
            std::string guid_str;
            std::getline(guid_if, guid_str);
            std::string size_str;
            std::getline(size_if, size_str);
            // guid is hex like 0x9956f43f
            uint64_t guid_val = 0;
            try {
                guid_val = std::stoull(guid_str, nullptr, 0);
            }
            catch (...) {
                continue;
            }
            long size_val = 0;
            try {
                size_val = std::stol(size_str, nullptr, 0);
            }
            catch (...) {
                size_val = 0;
            }
            if (guid_val == SPR_PM_GUID && size_val > 0) {
                telem_dir = path;
                m_guid_str = guid_str;
                m_guid_val = guid_val;
                m_telem_size_bytes = static_cast<size_t>(size_val);
                break;
            }
            // Record the first valid telem as a fallback if SPR not found
            if (fallback_telem_dir.empty() && size_val > 0) {
                fallback_telem_dir = path;
                fallback_guid_str = guid_str;
                fallback_guid_val = guid_val;
                fallback_size_bytes = static_cast<size_t>(size_val);
            }
        }
        closedir(dir);
        if (telem_dir.empty()) {
            if (!fallback_telem_dir.empty()) {
                telem_dir = fallback_telem_dir;
                m_guid_str = fallback_guid_str;
                m_guid_val = fallback_guid_val;
                m_telem_size_bytes = fallback_size_bytes;
            }
            else {
                throw Exception("PMTIOGroup: No intel_pmt telem device found", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
        }
        m_telem_path = telem_dir + "/telem";
        m_fd = ::open(m_telem_path.c_str(), O_RDONLY | O_CLOEXEC);
        if (m_fd < 0) {
            throw Exception("PMTIOGroup: failed to open telem file: " + m_telem_path + ": " + std::string(strerror(errno)), GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
    }

    void PMTIOGroup::discover_signals_from_xml(void)
    {
        // Discover XML root from env; avoid introducing heavy XML deps.
        // Set GEOPM_PMT_XML_ROOT to the Intel-PMT/xml directory.
        const char *xml_root = std::getenv("GEOPM_PMT_XML_ROOT");
        if (!xml_root || !*xml_root) {
            return; // fallback to minimal map
        }

        auto slurp = [](const std::string &path) -> std::string {
            std::ifstream ifs(path);
            if (!ifs.good()) return {};
            std::ostringstream oss;
            oss << ifs.rdbuf();
            return oss.str();
        };
    // Load PMT mapping xml
    std::string pmt_path = std::string(xml_root) + "/pmt.xml";
    std::string pmt_xml = slurp(pmt_path);
        if (pmt_xml.empty()) {
            return;
        }

    std::ostringstream guid_ss;
    guid_ss << "0x" << std::hex << std::nouppercase << m_guid_val;
    std::string guid_key = guid_ss.str();

        // Find mapping block for our GUID
        std::string mapping_start_tag = std::string("<mapping guid=\"") + guid_key + "\"";
        size_t mpos = pmt_xml.find(mapping_start_tag);
        if (mpos == std::string::npos) {
            // Retry with uppercase GUID (some XMLs may use uppercase hex)
            std::ostringstream guid_ss_upper;
            guid_ss_upper << "0x" << std::hex << std::uppercase << m_guid_val;
            std::string mapping_upper = std::string("<mapping guid=\"") + guid_ss_upper.str() + "\"";
            mpos = pmt_xml.find(mapping_upper);
            if (mpos == std::string::npos) {
                return;
            }
        }
        size_t mend = pmt_xml.find("</mapping>", mpos);
        if (mend == std::string::npos) {
            return;
        }
        std::string map_block = pmt_xml.substr(mpos, mend - mpos);

        auto find_tag_value = [](const std::string &s, const std::string &tag) -> std::string {
            std::string open = std::string("<") + tag + ">";
            std::string close = std::string("</") + tag + ">";
            size_t a = s.find(open);
            if (a == std::string::npos) return {};
            a += open.size();
            size_t b = s.find(close, a);
            if (b == std::string::npos || b <= a) return {};
            return s.substr(a, b - a);
        };

        std::string basedir = find_tag_value(map_block, "basedir");
        std::string aggregator = find_tag_value(map_block, "aggregator");
        if (basedir.empty() || aggregator.empty()) {
            return;
        }

        // 2) Load the aggregator XML
        std::string agg_path = std::string(xml_root) + "/" + basedir + "/" + aggregator;
        std::string agg_xml = slurp(agg_path);
        if (agg_xml.empty()) {
            return;
        }

        // Helper to parse container index and lsb/msb for a subgroup label within its SampleGroup
        auto parse_group_for_subgroup = [&](const std::string &subgroup_label,
                                            uint32_t &container_idx,
                                            uint8_t &c2u_lsb, uint8_t &c2u_msb,
                                            uint8_t &u2c_lsb, uint8_t &u2c_msb) -> bool
        {
            std::string needle = std::string("<TELC:sampleSubGroup>") + subgroup_label + "</TELC:sampleSubGroup>";
            size_t sgpos = agg_xml.find(needle);
            if (sgpos == std::string::npos) return false;
            // Find the preceding SampleGroup start tag
            size_t grp_start = agg_xml.rfind("<TELEM:SampleGroup", sgpos);
            if (grp_start == std::string::npos) return false;
            size_t grp_name_pos = agg_xml.find("name=\"Container_", grp_start);
            if (grp_name_pos == std::string::npos || grp_name_pos > sgpos) return false;
            grp_name_pos += std::strlen("name=\"Container_");
            size_t grp_name_end = agg_xml.find("\"", grp_name_pos);
            if (grp_name_end == std::string::npos) return false;
            std::string num_str = agg_xml.substr(grp_name_pos, grp_name_end - grp_name_pos);
            try {
                container_idx = static_cast<uint32_t>(std::stoul(num_str));
            }
            catch (...) {
                return false;
            }
            // Limit the search to the group block
            size_t grp_end = agg_xml.find("</TELEM:SampleGroup>", grp_start);
            if (grp_end == std::string::npos) grp_end = sgpos + 1;
            std::string grp_block = agg_xml.substr(grp_start, grp_end - grp_start);

            auto find_sample_lsb_msb = [&](const std::string &sample_name, uint8_t &lsb, uint8_t &msb) -> bool {
                size_t s_pos = grp_block.find(std::string("<TELC:sample name=\"") + sample_name + "\"");
                if (s_pos == std::string::npos) return false;
                size_t lsb_pos = grp_block.find("<TELC:lsb>", s_pos);
                size_t lsb_end = (lsb_pos == std::string::npos) ? std::string::npos : grp_block.find("</TELC:lsb>", lsb_pos);
                size_t msb_pos = grp_block.find("<TELC:msb>", s_pos);
                size_t msb_end = (msb_pos == std::string::npos) ? std::string::npos : grp_block.find("</TELC:msb>", msb_pos);
                if (lsb_end == std::string::npos || msb_end == std::string::npos) return false;
                std::string lsb_str = grp_block.substr(lsb_pos + 10, lsb_end - (lsb_pos + 10));
                std::string msb_str = grp_block.substr(msb_pos + 10, msb_end - (msb_pos + 10));
                try {
                    int l = std::stoi(lsb_str);
                    int m = std::stoi(msb_str);
                    if (l < 0 || l > 63 || m < 0 || m > 63 || m < l) return false;
                    lsb = static_cast<uint8_t>(l);
                    msb = static_cast<uint8_t>(m);
                    return true;
                }
                catch (...) {
                    return false;
                }
            };

            bool ok1 = find_sample_lsb_msb("C2U_BW", c2u_lsb, c2u_msb);
            bool ok2 = find_sample_lsb_msb("U2C_BW", u2c_lsb, u2c_msb);
            return ok1 && ok2;
        };

        uint32_t base_port0 = 0, base_port1 = 0;
        uint8_t p0_c2u_lsb = 0, p0_c2u_msb = 31, p0_u2c_lsb = 32, p0_u2c_msb = 63;
        uint8_t p1_c2u_lsb = 0, p1_c2u_msb = 31, p1_u2c_lsb = 32, p1_u2c_msb = 63;

    bool have_p0 = parse_group_for_subgroup("IDI_PORT0_BW[0]", base_port0,
                                                p0_c2u_lsb, p0_c2u_msb, p0_u2c_lsb, p0_u2c_msb);
    bool have_p1 = parse_group_for_subgroup("IDI_PORT1_BW[0]", base_port1,
                                                p1_c2u_lsb, p1_c2u_msb, p1_u2c_lsb, p1_u2c_msb);
    uint32_t base_port2 = 0, base_port3 = 0;
    uint8_t p2_c2u_lsb = 0, p2_c2u_msb = 31, p2_u2c_lsb = 32, p2_u2c_msb = 63;
    uint8_t p3_c2u_lsb = 0, p3_c2u_msb = 31, p3_u2c_lsb = 32, p3_u2c_msb = 63;
    bool have_p2 = parse_group_for_subgroup("IDI_PORT2_BW[0]", base_port2,
                        p2_c2u_lsb, p2_c2u_msb, p2_u2c_lsb, p2_u2c_msb);
    bool have_p3 = parse_group_for_subgroup("IDI_PORT3_BW[0]", base_port3,
                        p3_c2u_lsb, p3_c2u_msb, p3_u2c_lsb, p3_u2c_msb);

    if (!have_p0 && !have_p1 && !have_p2 && !have_p3) {
            return;
        }

        // Populate signals based on discovered bases; domain-aware, per-CPU
        if (have_p0) {
            SignalDef c2u0 {base_port0, p0_c2u_lsb, p0_c2u_msb, GEOPM_DOMAIN_CPU,
                            IOGroup::M_UNITS_NONE, IOGroup::M_SIGNAL_BEHAVIOR_MONOTONE,
                            "IDI Port 0 Core-to-Uncore bandwidth counter (raw)"};
            SignalDef u2c0 {base_port0, p0_u2c_lsb, p0_u2c_msb, GEOPM_DOMAIN_CPU,
                            IOGroup::M_UNITS_NONE, IOGroup::M_SIGNAL_BEHAVIOR_MONOTONE,
                            "IDI Port 0 Uncore-to-Core bandwidth counter (raw)"};
            m_signal_def_by_name["PMT::IDI_PORT0_C2U_BW"] = c2u0;
            m_signal_def_by_name["PMT::IDI_PORT0_U2C_BW"] = u2c0;
        }
        if (have_p1) {
            SignalDef c2u1 {base_port1, p1_c2u_lsb, p1_c2u_msb, GEOPM_DOMAIN_CPU,
                            IOGroup::M_UNITS_NONE, IOGroup::M_SIGNAL_BEHAVIOR_MONOTONE,
                            "IDI Port 1 Core-to-Uncore bandwidth counter (raw)"};
            SignalDef u2c1 {base_port1, p1_u2c_lsb, p1_u2c_msb, GEOPM_DOMAIN_CPU,
                            IOGroup::M_UNITS_NONE, IOGroup::M_SIGNAL_BEHAVIOR_MONOTONE,
                            "IDI Port 1 Uncore-to-Core bandwidth counter (raw)"};
            m_signal_def_by_name["PMT::IDI_PORT1_C2U_BW"] = c2u1;
            m_signal_def_by_name["PMT::IDI_PORT1_U2C_BW"] = u2c1;
        }
        if (have_p2) {
            SignalDef c2u2 {base_port2, p2_c2u_lsb, p2_c2u_msb, GEOPM_DOMAIN_CPU,
                            IOGroup::M_UNITS_NONE, IOGroup::M_SIGNAL_BEHAVIOR_MONOTONE,
                            "IDI Port 2 Core-to-Uncore bandwidth counter (raw)"};
            SignalDef u2c2 {base_port2, p2_u2c_lsb, p2_u2c_msb, GEOPM_DOMAIN_CPU,
                            IOGroup::M_UNITS_NONE, IOGroup::M_SIGNAL_BEHAVIOR_MONOTONE,
                            "IDI Port 2 Uncore-to-Core bandwidth counter (raw)"};
            m_signal_def_by_name["PMT::IDI_PORT2_C2U_BW"] = c2u2;
            m_signal_def_by_name["PMT::IDI_PORT2_U2C_BW"] = u2c2;
        }
        if (have_p3) {
            SignalDef c2u3 {base_port3, p3_c2u_lsb, p3_c2u_msb, GEOPM_DOMAIN_CPU,
                            IOGroup::M_UNITS_NONE, IOGroup::M_SIGNAL_BEHAVIOR_MONOTONE,
                            "IDI Port 3 Core-to-Uncore bandwidth counter (raw)"};
            SignalDef u2c3 {base_port3, p3_u2c_lsb, p3_u2c_msb, GEOPM_DOMAIN_CPU,
                            IOGroup::M_UNITS_NONE, IOGroup::M_SIGNAL_BEHAVIOR_MONOTONE,
                            "IDI Port 3 Uncore-to-Core bandwidth counter (raw)"};
            m_signal_def_by_name["PMT::IDI_PORT3_C2U_BW"] = c2u3;
            m_signal_def_by_name["PMT::IDI_PORT3_U2C_BW"] = u2c3;
        }
    }

    void PMTIOGroup::build_minimal_field_map(void)
    {
        // Minimal initial support: IDI Port 0 per-core bandwidth counters
        // Each container is 64 bits; C2U_BW in bits [31:0], U2C_BW in [63:32]
        // Containers 1..(1+m_num_cpu) map to cores 0..N-1 for IDI port 0.
        // Use domain-aware signal names; container index is base + domain_idx
        SignalDef c2u {1u, 0, 31, GEOPM_DOMAIN_CPU, IOGroup::M_UNITS_NONE, IOGroup::M_SIGNAL_BEHAVIOR_MONOTONE,
                       "IDI Port 0 Core-to-Uncore bandwidth counter (raw)"};
        SignalDef u2c {1u, 32, 63, GEOPM_DOMAIN_CPU, IOGroup::M_UNITS_NONE, IOGroup::M_SIGNAL_BEHAVIOR_MONOTONE,
                       "IDI Port 0 Uncore-to-Core bandwidth counter (raw)"};
        m_signal_def_by_name["PMT::IDI_PORT0_C2U_BW"] = c2u;
        m_signal_def_by_name["PMT::IDI_PORT0_U2C_BW"] = u2c;
    }

    std::set<std::string> PMTIOGroup::signal_names(void) const
    {
        std::set<std::string> result;
        for (const auto &kv : m_signal_def_by_name) {
            result.insert(kv.first);
        }
        // Advertise composite totals/rates if there is at least one IDI signal
        bool have_any = false;
        for (const auto &kv : m_signal_def_by_name) {
            if (kv.first.rfind("PMT::IDI_PORT", 0) == 0) { have_any = true; break; }
        }
        if (have_any) {
            // totals of raw counters
            result.insert("PMT::IDI_C2U_BW_TOTAL");
            result.insert("PMT::IDI_U2C_BW_TOTAL");
            // per-port rates and totals rates
            for (int port = 0; port <= 3; ++port) {
                std::ostringstream os1; os1 << "PMT::IDI_PORT" << port << "_C2U_BW_RATE";
                std::ostringstream os2; os2 << "PMT::IDI_PORT" << port << "_U2C_BW_RATE";
                // Include only if the raw component exists
                std::ostringstream raw1; raw1 << "PMT::IDI_PORT" << port << "_C2U_BW";
                std::ostringstream raw2; raw2 << "PMT::IDI_PORT" << port << "_U2C_BW";
                if (m_signal_def_by_name.count(raw1.str())) result.insert(os1.str());
                if (m_signal_def_by_name.count(raw2.str())) result.insert(os2.str());
            }
            result.insert("PMT::IDI_C2U_BW_TOTAL_RATE");
            result.insert("PMT::IDI_U2C_BW_TOTAL_RATE");
        }
        return result;
    }

    std::set<std::string> PMTIOGroup::control_names(void) const
    {
        return {};
    }

    bool PMTIOGroup::is_valid_signal(const std::string &signal_name) const
    {
        if (m_signal_def_by_name.count(signal_name)) return true;
        if (signal_name == "PMT::IDI_C2U_BW_TOTAL" || signal_name == "PMT::IDI_U2C_BW_TOTAL" ||
            signal_name == "PMT::IDI_C2U_BW_TOTAL_RATE" || signal_name == "PMT::IDI_U2C_BW_TOTAL_RATE") {
            bool is_c2u = signal_name.find("C2U") != std::string::npos;
            for (int port = 0; port <= 3; ++port) {
                std::ostringstream os;
                os << "PMT::IDI_PORT" << port << (is_c2u ? "_C2U_BW" : "_U2C_BW");
                if (m_signal_def_by_name.count(os.str())) return true;
            }
            return false;
        }
        // per-port rate
        if (signal_name.rfind("PMT::IDI_PORT", 0) == 0 && signal_name.find("_RATE") != std::string::npos) {
            std::string base = signal_name;
            base.erase(base.find("_RATE"));
            return m_signal_def_by_name.count(base) != 0;
        }
        return false;
    }

    bool PMTIOGroup::is_valid_control(const std::string &control_name) const
    {
        return false;
    }

    int PMTIOGroup::signal_domain_type(const std::string &signal_name) const
    {
        auto it = m_signal_def_by_name.find(signal_name);
        if (it != m_signal_def_by_name.end()) {
            return it->second.domain_type;
        }
        // virtual signals are CPU domain
        if (is_valid_signal(signal_name)) return GEOPM_DOMAIN_CPU;
        throw Exception("PMTIOGroup::signal_domain_type(): invalid signal: " + signal_name, GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    int PMTIOGroup::control_domain_type(const std::string &control_name) const
    {
        throw Exception("PMTIOGroup has no controls", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    int PMTIOGroup::push_signal(const std::string &signal_name, int domain_type, int domain_idx)
    {
        // Composite total signals
        if (signal_name == "PMT::IDI_C2U_BW_TOTAL" || signal_name == "PMT::IDI_U2C_BW_TOTAL") {
            if (domain_type != GEOPM_DOMAIN_CPU) {
                throw Exception("PMTIOGroup::push_signal(): composite totals are CPU domain", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            if (domain_idx < 0 || domain_idx >= geopm::platform_topo().num_domain(domain_type)) {
                throw Exception("PMTIOGroup::push_signal(): domain_idx out of range", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            std::vector<std::string> comps;
            for (int port = 0; port <= 3; ++port) {
                std::ostringstream os;
                os << "PMT::IDI_PORT" << port << (signal_name.find("C2U") != std::string::npos ? "_C2U_BW" : "_U2C_BW");
                if (m_signal_def_by_name.count(os.str())) comps.push_back(os.str());
            }
            if (comps.empty()) {
                throw Exception("PMTIOGroup::push_signal(): composite has no components on this platform", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            PushedSignal ps;
            ps.name = signal_name;
            ps.domain_type = domain_type;
            ps.domain_idx = domain_idx;
            ps.is_composite = true;
            ps.components = std::move(comps);
            ps.value = 0.0;
            m_pushed.push_back(std::move(ps));
            return static_cast<int>(m_pushed.size()) - 1;
        }

        // Composite rate totals
        if (signal_name == "PMT::IDI_C2U_BW_TOTAL_RATE" || signal_name == "PMT::IDI_U2C_BW_TOTAL_RATE" ||
            (signal_name.rfind("PMT::IDI_PORT", 0) == 0 && signal_name.find("_RATE") != std::string::npos)) {
            if (domain_type != GEOPM_DOMAIN_CPU) {
                throw Exception("PMTIOGroup::push_signal(): rate signals are CPU domain", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            if (domain_idx < 0 || domain_idx >= geopm::platform_topo().num_domain(domain_type)) {
                throw Exception("PMTIOGroup::push_signal(): domain_idx out of range", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            std::vector<std::string> comps;
            if (signal_name.rfind("PMT::IDI_PORT", 0) == 0) {
                std::string base = signal_name;
                base.erase(base.find("_RATE"));
                if (!m_signal_def_by_name.count(base)) {
                    throw Exception("PMTIOGroup::push_signal(): base component missing for port rate", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                }
                comps.push_back(base);
            }
            else {
                // total C2U/U2C rate
                bool is_c2u = signal_name.find("C2U") != std::string::npos;
                for (int port = 0; port <= 3; ++port) {
                    std::ostringstream os;
                    os << "PMT::IDI_PORT" << port << (is_c2u ? "_C2U_BW" : "_U2C_BW");
                    if (m_signal_def_by_name.count(os.str())) comps.push_back(os.str());
                }
                if (comps.empty()) {
                    throw Exception("PMTIOGroup::push_signal(): composite rate has no components", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                }
            }
            PushedSignal ps;
            ps.name = signal_name;
            ps.domain_type = domain_type;
            ps.domain_idx = domain_idx;
            ps.is_composite = true; // sums base(s)
            ps.is_rate = true;
            ps.components = std::move(comps);
            ps.value = 0.0;
            m_pushed.push_back(std::move(ps));
            return static_cast<int>(m_pushed.size()) - 1;
        }

        auto it = m_signal_def_by_name.find(signal_name);
        if (it == m_signal_def_by_name.end()) {
            throw Exception("PMTIOGroup::push_signal(): invalid signal: " + signal_name, GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_type != it->second.domain_type) {
            throw Exception("PMTIOGroup::push_signal(): domain type mismatch", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_idx < 0 || domain_idx >= geopm::platform_topo().num_domain(domain_type)) {
            throw Exception("PMTIOGroup::push_signal(): domain_idx out of range", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
    PushedSignal ps {signal_name, domain_type, domain_idx, it->second, 0.0, false, {}};
        m_pushed.push_back(ps);
        return static_cast<int>(m_pushed.size()) - 1;
    }

    int PMTIOGroup::push_control(const std::string &control_name, int domain_type, int domain_idx)
    {
        throw Exception("PMTIOGroup has no controls", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    void PMTIOGroup::read_batch(void)
    {
        // Read unique containers referenced by pushed signals and update samples
        std::map<uint32_t, uint64_t> cache;
        double t_now = geopm_time_since(&m_time_zero);
        for (auto &ps : m_pushed) {
            if (ps.is_composite) {
                // Sum components for the same domain idx
                double total = 0.0;
                for (const auto &comp : ps.components) {
                    auto it = m_signal_def_by_name.find(comp);
                    if (it == m_signal_def_by_name.end()) continue;
                    uint32_t container_idx = it->second.base_container + static_cast<uint32_t>(ps.domain_idx);
                    auto cit = cache.find(container_idx);
                    uint64_t val = 0;
                    if (cit == cache.end()) {
                        val = read_container(container_idx);
                        cache[container_idx] = val;
                    }
                    else {
                        val = cit->second;
                    }
                    uint64_t raw = extract_bits(val, it->second.lsb, it->second.msb);
            // Apply rollover extension per base signal and cpu
            auto ro = rollover_for(comp, ps.domain_idx);
            double ext = ro ? ro->update(static_cast<double>(raw)) : static_cast<double>(raw);
            total += ext;
                }
                if (ps.is_rate) {
                    if (!std::isnan(ps.last_time)) {
                        double dt = t_now - ps.last_time;
                        if (dt > 0.0) {
                            double dy = total - ps.last_value;
                            ps.value = dy / dt;
                        }
                    }
                    ps.last_time = t_now;
                    ps.last_value = total;
                }
                else {
                    ps.value = total;
                }
                continue;
            }
            uint32_t container_idx = ps.spec.base_container + static_cast<uint32_t>(ps.domain_idx);
            auto it = cache.find(container_idx);
            uint64_t val = 0;
            if (it == cache.end()) {
                val = read_container(container_idx);
                cache[container_idx] = val;
            }
            else {
                val = it->second;
            }
            uint64_t raw = extract_bits(val, ps.spec.lsb, ps.spec.msb);
        // Apply rollover extension for base signal
        auto ro = rollover_for(ps.name, ps.domain_idx);
        ps.value = ro ? ro->update(static_cast<double>(raw)) : static_cast<double>(raw);
        }
    }

    void PMTIOGroup::write_batch(void)
    {
        // no-op
    }

    double PMTIOGroup::sample(int batch_idx)
    {
        if (batch_idx < 0 || batch_idx >= (int)m_pushed.size()) {
            throw Exception("PMTIOGroup::sample(): invalid batch_idx", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return m_pushed[batch_idx].value;
    }

    void PMTIOGroup::adjust(int batch_idx, double setting)
    {
        throw Exception("PMTIOGroup has no controls", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    double PMTIOGroup::read_signal(const std::string &signal_name, int domain_type, int domain_idx)
    {
        // Composite totals
        if (signal_name == "PMT::IDI_C2U_BW_TOTAL" || signal_name == "PMT::IDI_U2C_BW_TOTAL") {
            std::vector<std::string> comps;
            for (int port = 0; port <= 3; ++port) {
                std::ostringstream os;
                os << "PMT::IDI_PORT" << port << (signal_name.find("C2U") != std::string::npos ? "_C2U_BW" : "_U2C_BW");
                if (m_signal_def_by_name.count(os.str())) comps.push_back(os.str());
            }
            if (comps.empty()) {
                throw Exception("PMTIOGroup::read_signal(): no components found for composite", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            double total = 0.0;
            for (const auto &comp : comps) {
                total += read_signal(comp, domain_type, domain_idx);
            }
            return total;
        }

        // Rate signals (per-port or totals)
        if (signal_name == "PMT::IDI_C2U_BW_TOTAL_RATE" || signal_name == "PMT::IDI_U2C_BW_TOTAL_RATE" ||
            (signal_name.rfind("PMT::IDI_PORT", 0) == 0 && signal_name.find("_RATE") != std::string::npos)) {
            // Stateless direct compute: read twice with tiny sleep would be expensive; instead approximate using last_time kept per push.
            // For direct path, fall back to a simple finite difference using two immediate reads if needed.
            // Here we implement a naive immediate two-read approach.
            geopm_time_s tz = time_zero();
            double t0 = geopm_time_since(&tz);
            double y0 = 0.0;
            if (signal_name.rfind("PMT::IDI_PORT", 0) == 0) {
                std::string base = signal_name;
                base.erase(base.find("_RATE"));
                y0 = read_signal(base, domain_type, domain_idx);
            }
            else {
                bool is_c2u = signal_name.find("C2U") != std::string::npos;
                for (int port = 0; port <= 3; ++port) {
                    std::ostringstream os;
                    os << "PMT::IDI_PORT" << port << (is_c2u ? "_C2U_BW" : "_U2C_BW");
                    if (m_signal_def_by_name.count(os.str())) y0 += read_signal(os.str(), domain_type, domain_idx);
                }
            }
            // small sleep
            struct timespec req {0, static_cast<long>(m_direct_rate_sleep_ns)};
            nanosleep(&req, nullptr);
            double t1 = geopm_time_since(&tz);
            double y1 = 0.0;
            if (signal_name.rfind("PMT::IDI_PORT", 0) == 0) {
                std::string base = signal_name;
                base.erase(base.find("_RATE"));
                y1 = read_signal(base, domain_type, domain_idx);
            }
            else {
                bool is_c2u = signal_name.find("C2U") != std::string::npos;
                for (int port = 0; port <= 3; ++port) {
                    std::ostringstream os;
                    os << "PMT::IDI_PORT" << port << (is_c2u ? "_C2U_BW" : "_U2C_BW");
                    if (m_signal_def_by_name.count(os.str())) y1 += read_signal(os.str(), domain_type, domain_idx);
                }
            }
            double dt = t1 - t0;
            return dt > 0 ? (y1 - y0) / dt : NAN;
        }

    auto it = m_signal_def_by_name.find(signal_name);
        if (it == m_signal_def_by_name.end()) {
            throw Exception("PMTIOGroup::read_signal(): invalid signal: " + signal_name, GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_type != it->second.domain_type) {
            throw Exception("PMTIOGroup::read_signal(): domain type mismatch", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        uint32_t container_idx = it->second.base_container + static_cast<uint32_t>(domain_idx);
        uint64_t val = read_container(container_idx);
        uint64_t raw = extract_bits(val, it->second.lsb, it->second.msb);
    // Apply rollover extension for base signal
    auto ro = rollover_for(signal_name, domain_idx);
    return ro ? ro->update(static_cast<double>(raw)) : static_cast<double>(raw);
    }

    void PMTIOGroup::write_control(const std::string &control_name, int domain_type, int domain_idx, double setting)
    {
        throw Exception("PMTIOGroup has no controls", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    void PMTIOGroup::save_control(void)
    {
        // no-op
    }

    void PMTIOGroup::restore_control(void)
    {
        // no-op
    }

    void PMTIOGroup::save_control(const std::string &save_path)
    {
        (void)save_path; // no-op; no controls
    }

    void PMTIOGroup::restore_control(const std::string &save_path)
    {
        (void)save_path; // no-op; no controls
    }

    std::function<double(const std::vector<double> &)> PMTIOGroup::agg_function(const std::string &signal_name) const
    {
        if (!is_valid_signal(signal_name)) {
            // Allow composite totals to be aggregated
            if (signal_name == "PMT::IDI_C2U_BW_TOTAL" || signal_name == "PMT::IDI_U2C_BW_TOTAL" ||
                signal_name == "PMT::IDI_C2U_BW_TOTAL_RATE" || signal_name == "PMT::IDI_U2C_BW_TOTAL_RATE") {
                return geopm::Agg::sum;
            }
            throw Exception("PMTIOGroup::agg_function(): invalid signal: " + signal_name, GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        // Raw counters: sum across domains; rates: average
        if (signal_name.find("_RATE") != std::string::npos) {
            return geopm::Agg::average;
        }
        return geopm::Agg::sum;
    }

    std::function<std::string(double)> PMTIOGroup::format_function(const std::string &signal_name) const
    {
        return IOGroup::format_function(signal_name);
    }

    std::string PMTIOGroup::signal_description(const std::string &signal_name) const
    {
        if (signal_name == "PMT::IDI_C2U_BW_TOTAL") {
            return "IDI Core-to-Uncore bandwidth counter total across ports (raw, per-CPU)";
        }
        if (signal_name == "PMT::IDI_U2C_BW_TOTAL") {
            return "IDI Uncore-to-Core bandwidth counter total across ports (raw, per-CPU)";
        }
        if (signal_name == "PMT::IDI_C2U_BW_TOTAL_RATE") {
            return "IDI Core-to-Uncore bandwidth total rate across ports (per-CPU, counts per second)";
        }
        if (signal_name == "PMT::IDI_U2C_BW_TOTAL_RATE") {
            return "IDI Uncore-to-Core bandwidth total rate across ports (per-CPU, counts per second)";
        }
        if (signal_name.rfind("PMT::IDI_PORT", 0) == 0 && signal_name.find("_RATE") != std::string::npos) {
            return "IDI bandwidth counter rate (per-CPU, counts per second) for a specific port";
        }
        auto it = m_signal_def_by_name.find(signal_name);
        if (it != m_signal_def_by_name.end()) {
            return it->second.description;
        }
        throw Exception("PMTIOGroup::signal_description(): invalid signal", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    std::string PMTIOGroup::control_description(const std::string &control_name) const
    {
        throw Exception("PMTIOGroup has no controls", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    int PMTIOGroup::signal_behavior(const std::string &signal_name) const
    {
        if (signal_name == "PMT::IDI_C2U_BW_TOTAL" || signal_name == "PMT::IDI_U2C_BW_TOTAL") {
            return IOGroup::M_SIGNAL_BEHAVIOR_MONOTONE;
        }
        if (signal_name.find("_RATE") != std::string::npos) {
            return IOGroup::M_SIGNAL_BEHAVIOR_VARIABLE;
        }
        auto it = m_signal_def_by_name.find(signal_name);
        if (it == m_signal_def_by_name.end()) {
            throw Exception("PMTIOGroup::signal_behavior(): invalid signal", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return it->second.behavior;
    }

    std::string PMTIOGroup::name(void) const
    {
        return plugin_name();
    }

    uint64_t PMTIOGroup::read_container(uint32_t container_idx) const
    {
        // Each container is 8 bytes (64 bits), contiguous
        off_t off = static_cast<off_t>(container_idx) * 8;
        if (off + 8 > (off_t)m_telem_size_bytes) {
            throw Exception("PMTIOGroup: container index out of range", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        uint64_t val = 0;
        ssize_t rc = pread(m_fd, &val, sizeof(val), off);
        if (rc != (ssize_t)sizeof(val)) {
            throw Exception("PMTIOGroup: pread telem failed", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        return val;
    }

    uint64_t PMTIOGroup::extract_bits(uint64_t val, uint8_t lsb, uint8_t msb)
    {
        if (msb < lsb || msb > 63) return 0;
        uint64_t width = msb - lsb + 1;
        uint64_t mask = (width == 64) ? UINT64_MAX : ((1ull << width) - 1ull);
        return (val >> lsb) & mask;
    }

    std::shared_ptr<RolloverGenerator> PMTIOGroup::rollover_for(const std::string &signal_name,
                                                                int domain_idx)
    {
        // Only for base signals; caller passes base name for composites.
        auto map_it = m_rollover_by_signal.find(signal_name);
        if (map_it == m_rollover_by_signal.end()) {
            // Initialize vector sized to number of CPUs
            std::vector<std::shared_ptr<RolloverGenerator>> vec(m_num_cpu);
            // Determine rollover factor as 2^(field width)
            auto def_it = m_signal_def_by_name.find(signal_name);
            if (def_it == m_signal_def_by_name.end()) return nullptr;
            const auto &def = def_it->second;
            const int width = static_cast<int>(def.msb - def.lsb + 1);
            // Use double factor; if width==64, treat as no rollover handling
            double factor = (width >= 64) ? 0.0 : std::ldexp(1.0, width); // 2^width
            for (int cpu = 0; cpu < m_num_cpu; ++cpu) {
                vec[cpu] = std::make_shared<RolloverGenerator>();
                vec[cpu]->set_factor(factor);
            }
            map_it = m_rollover_by_signal.emplace(signal_name, std::move(vec)).first;
        }
        if (domain_idx < 0 || domain_idx >= m_num_cpu) return nullptr;
        return map_it->second[domain_idx];
    }
}
