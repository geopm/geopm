// Auto-generated minimal Rust FFI bindings to selected libgeopmd C interfaces needed
// for PlatformIO (PIO) and StatsCollector usage from Rust.  This is an initial
// subset; expand as required.
//
// Copyright (c) 2015 - 2025 Intel Corporation
// SPDX-License-Identifier: BSD-3-Clause

use std::os::raw::{c_char, c_int, c_double};

pub const GEOPM_NAME_MAX: usize = 255; // keep in sync with geopm_pio.h

#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct geopm_request_s {
    pub domain_type: c_int,
    pub domain_idx: c_int,
    pub name: [c_char; GEOPM_NAME_MAX],
}

// Stats collector related enums (mirroring geopm_stats_collector.h)
#[allow(dead_code)]
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub enum geopm_sample_stats_e {
    GEOPM_SAMPLE_TIME_TOTAL,
    GEOPM_SAMPLE_COUNT,
    GEOPM_SAMPLE_PERIOD_MEAN,
    GEOPM_SAMPLE_PERIOD_STD,
    GEOPM_NUM_SAMPLE_STATS,
}

#[allow(dead_code)]
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub enum geopm_metric_stats_e {
    GEOPM_METRIC_COUNT,
    GEOPM_METRIC_FIRST,
    GEOPM_METRIC_LAST,
    GEOPM_METRIC_MIN,
    GEOPM_METRIC_MAX,
    GEOPM_METRIC_MEAN,
    GEOPM_METRIC_STD,
    GEOPM_NUM_METRIC_STATS,
}

#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct geopm_metric_stats_s {
    pub name: [c_char; GEOPM_NAME_MAX],
    pub stats: [c_double; 7], // GEOPM_NUM_METRIC_STATS
}

#[repr(C)]
#[derive(Debug)]
pub struct geopm_report_s {
    pub host: [c_char; GEOPM_NAME_MAX],
    pub sample_time_first: [c_char; GEOPM_NAME_MAX],
    pub sample_stats: [c_double; 4], // GEOPM_NUM_SAMPLE_STATS
    pub num_metric: usize,
    pub metric_stats: *mut geopm_metric_stats_s,
}

// Opaque handle types
#[repr(C)]
pub struct geopm_stats_collector_s { _private: [u8; 0] }

extern "C" {
    // PlatformIO subset (subset required for exporter)
    pub fn geopm_pio_num_signal_name() -> c_int;
    pub fn geopm_pio_signal_name(name_idx: c_int, result_max: usize, result: *mut c_char) -> c_int;
    pub fn geopm_pio_signal_domain_type(signal_name: *const c_char) -> c_int;
    pub fn geopm_pio_push_signal(signal_name: *const c_char, domain_type: c_int, domain_idx: c_int) -> c_int;
    pub fn geopm_pio_read_batch() -> c_int;
    pub fn geopm_pio_sample(signal_idx: c_int, result: *mut c_double) -> c_int;
    pub fn geopm_pio_signal_info(signal_name: *const c_char,
                                 aggregation_type: *mut c_int,
                                 format_type: *mut c_int,
                                 behavior_type: *mut c_int) -> c_int;

    // Topology helpers
    pub fn geopm_topo_domain_type(domain_name: *const c_char) -> c_int;
    pub fn geopm_topo_num_domain(domain_type: c_int) -> c_int;
    pub fn geopm_topo_domain_name(domain_type: c_int, domain_name_max: usize, domain_name: *mut c_char) -> c_int;

    // Stats collector subset
    pub fn geopm_stats_collector_create(num_requests: usize,
                                        requests: *const geopm_request_s,
                                        collector: *mut *mut geopm_stats_collector_s) -> c_int;
    pub fn geopm_stats_collector_update(collector: *mut geopm_stats_collector_s) -> c_int;
    pub fn geopm_stats_collector_report(collector: *const geopm_stats_collector_s,
                                        num_requests: usize,
                                        report: *mut geopm_report_s) -> c_int;
    pub fn geopm_stats_collector_reset(collector: *mut geopm_stats_collector_s) -> c_int;
    pub fn geopm_stats_collector_free(collector: *mut geopm_stats_collector_s) -> c_int;
}

// Safe-ish Rust wrappers
pub fn last_os_error(code: c_int, context: &str) -> std::io::Result<()> {
    if code == 0 { Ok(()) } else { Err(std::io::Error::new(std::io::ErrorKind::Other, format!("{} (errno={})", context, code))) }
}

pub fn cstr_to_string(buf: &[c_char]) -> String {
    let nul_pos = buf.iter().position(|&c| c == 0).unwrap_or(buf.len());
    let slice = &buf[..nul_pos];
    let bytes: Vec<u8> = slice.iter().map(|c| *c as u8).collect();
    String::from_utf8_lossy(&bytes).to_string()
}

// Removed unused SignalBatch and push_signal helper (use direct FFI in exporter)

pub struct StatsCollector {
    handle: *mut geopm_stats_collector_s,
    requests: Vec<geopm_request_s>,
}

impl StatsCollector {
    pub fn new(requests: Vec<geopm_request_s>) -> std::io::Result<Self> {
        let mut handle: *mut geopm_stats_collector_s = std::ptr::null_mut();
        let rc = unsafe { geopm_stats_collector_create(requests.len(), requests.as_ptr(), &mut handle) };
        last_os_error(rc, "geopm_stats_collector_create")?;
        Ok(Self { handle, requests })
    }
    pub fn update(&mut self) -> std::io::Result<()> {
        let rc = unsafe { geopm_stats_collector_update(self.handle) };
        last_os_error(rc, "geopm_stats_collector_update")
    }
    pub fn report(&self) -> std::io::Result<Vec<geopm_metric_stats_s>> {
        // allocate array for metric stats (num_requests)
        let mut metrics: Vec<geopm_metric_stats_s> = vec![geopm_metric_stats_s { name: [0; GEOPM_NAME_MAX], stats: [0.0; 7] }; self.requests.len()];
        let mut report = geopm_report_s { host: [0; GEOPM_NAME_MAX], sample_time_first: [0; GEOPM_NAME_MAX], sample_stats: [0.0; 4], num_metric: self.requests.len(), metric_stats: metrics.as_mut_ptr() };
        let rc = unsafe { geopm_stats_collector_report(self.handle, self.requests.len(), &mut report) };
        last_os_error(rc, "geopm_stats_collector_report")?;
        Ok(metrics)
    }
    pub fn reset(&mut self) -> std::io::Result<()> {
        let rc = unsafe { geopm_stats_collector_reset(self.handle) };
        last_os_error(rc, "geopm_stats_collector_reset")
    }
}

impl Drop for StatsCollector {
    fn drop(&mut self) {
        if !self.handle.is_null() {
            unsafe { geopm_stats_collector_free(self.handle); }
        }
    }
}
