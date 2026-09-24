// geopmexporter: Prometheus exporter for GEOPM metrics (Rust prototype)
// Mirrors subset of Python exporter functionality using libgeopmd FFI.
//
// NOTE: This is an initial implementation focused on batch read + StatsCollector
// aggregation. Additional behaviors (TLS, alternate summary via native
// prometheus Summary/Counter) can be added incrementally.
//
// Copyright (c) 2015 - 2025 Intel Corporation
// SPDX-License-Identifier: BSD-3-Clause

mod ffi;
use crate::ffi::*;
use clap::{Parser, ArgAction};
use prometheus::{Encoder, TextEncoder, Gauge, Counter, register_gauge, register_counter};
use std::ffi::CString;
use std::time::{Duration, Instant};
use std::thread::{self, sleep};
use std::os::raw::c_int;
use std::net::TcpListener;
use std::io::{Write, Read};
use std::sync::Arc;
use thiserror::Error;

const STARTUP_SLEEP: Duration = Duration::from_millis(5);

#[derive(Error, Debug)]
enum ExporterError {
    #[error("IO error: {0}")] Io(#[from] std::io::Error),
    #[error("UTF8 error: {0}")] Utf8(#[from] std::str::Utf8Error),
    #[error("Other: {0}")] Other(String),
}

#[derive(Parser, Debug)]
#[command(about="Prometheus exporter for GEOPM metrics")]
struct Args {
    #[arg(short='v', long="version", action=ArgAction::SetTrue)]
    version: bool,
    #[arg(short='t', long="period", default_value_t=0.1)]
    period: f64,
    #[arg(short='p', long="port", default_value_t=8000)]
    port: u16,
    #[arg(short='i', long="signal-config")]
    config_path: Option<String>,
    #[arg(long="summary", default_value="geopm")] 
    summary: String,
    #[arg(short='c', long="certfile")]
    certfile: Option<String>,
    #[arg(short='k', long="keyfile")]
    keyfile: Option<String>,
    #[arg(long="insecure-http", action=ArgAction::SetTrue)]
    insecure_http: bool,
}

#[derive(Clone)]
struct MetricSet {
    gauges: Arc<Vec<Gauge>>, // order matches requests * 7 stat fields
}

fn sanitize_metric_name(name: &str) -> String {
    let mut out = String::from("geopm_");
    for c in name.chars() {
        if c.is_ascii_alphanumeric() || c == '_' { out.push(c.to_ascii_lowercase()); } else { out.push('_'); }
    }
    out
}

fn default_requests() -> Result<Vec<geopm_request_s>, ExporterError> {
    // Simplified: iterate over signal name indices and filter by substrings
    let mut result = Vec::new();
    let num = unsafe { geopm_pio_num_signal_name() };
    if num <= 0 { return Err(ExporterError::Other(format!("no signals (rc={})", num))); }
    for idx in 0..num { // header doc says >0? but treat inclusive 0..num
        let mut buf = vec![0i8; GEOPM_NAME_MAX];
        let rc = unsafe { geopm_pio_signal_name(idx as c_int, GEOPM_NAME_MAX, buf.as_mut_ptr()) };
        if rc != 0 { continue; }
        let name = cstr_to_string(&buf);
        let include_strings = ["POWER", "ENERGY", "FREQ", "TEMPERATURE"]; // same as python
        let exclude = ["::", "CONTROL", "MAX", "MIN", "STEP", "LIMIT", "STICKER"];        
        if exclude.iter().any(|e| name.contains(e)) { continue; }
        if !include_strings.iter().any(|i| name.contains(i)) { continue; }
        // Domain 0 (BOARD) and index 0
        let mut req_name: [i8; GEOPM_NAME_MAX] = [0; GEOPM_NAME_MAX];
        for (i, b) in name.bytes().take(GEOPM_NAME_MAX-1).enumerate() { req_name[i] = b as i8; }
        result.push(geopm_request_s { domain_type: 0, domain_idx: 0, name: req_name });
    }
    if result.is_empty() { return Err(ExporterError::Other("Failed to find any signals".into())); }
    Ok(result)
}

fn build_stats_metrics(requests: &[geopm_request_s]) -> MetricSet {
    let mut gauges = Vec::new();
    let stat_fields = ["count","first","last","min","max","mean","std"];
    for req in requests {
        let base = cstr_to_string(&req.name);
        let base_san = sanitize_metric_name(&base);
        for stat in &stat_fields {
            let full_name = format!("{}_{}", base_san, stat);
            let gauge = register_gauge!(full_name.as_str(), &format!("{} {}", base, stat)).unwrap();
            gauges.push(gauge);
        }
    }
    MetricSet { gauges: Arc::new(gauges) }
}

fn make_request(name: &str, domain_type: c_int, domain_idx: c_int) -> geopm_request_s {
    let mut req_name: [i8; GEOPM_NAME_MAX] = [0; GEOPM_NAME_MAX];
    for (i,b) in name.bytes().take(GEOPM_NAME_MAX-1).enumerate() { req_name[i] = b as i8; }
    geopm_request_s { domain_type, domain_idx, name: req_name }
}

fn parse_config(path: &str) -> Result<Vec<geopm_request_s>, ExporterError> {
    let mut out = Vec::new();
    let mut rdr: Box<dyn Read> = if path == "-" { Box::new(std::io::stdin()) } else { Box::new(std::fs::File::open(path)?) };
    let mut text = String::new();
    rdr.read_to_string(&mut text)?;
    for (ln,line) in text.lines().enumerate() {
        let line = line.trim();
        if line.is_empty() || line.starts_with('#') { continue; }
        let parts: Vec<&str> = line.split_whitespace().collect();
        if parts.len() != 3 { return Err(ExporterError::Other(format!("Invalid config line {}: {}", ln+1, line))); }
        let sig = parts[0];
        let dom = parts[1];
        let idx = parts[2];
        let dom_c = CString::new(dom).unwrap();
        let dom_type = unsafe { geopm_topo_domain_type(dom_c.as_ptr()) };
        if dom_type < 0 { return Err(ExporterError::Other(format!("Unknown domain '{}' line {}", dom, ln+1))); }
        let max_dom = unsafe { geopm_topo_num_domain(dom_type) };
        if idx == "*" {
            for di in 0..max_dom { out.push(make_request(sig, dom_type, di)); }
        } else {
            let di: i32 = idx.parse().map_err(|_| ExporterError::Other(format!("Bad domain index '{}' line {}", idx, ln+1)))?;
            if di < 0 || di >= max_dom { return Err(ExporterError::Other(format!("Domain index out of range '{}' line {}", di, ln+1))); }
            out.push(make_request(sig, dom_type, di));
        }
    }
    if out.is_empty() { return Err(ExporterError::Other("No requests parsed".into())); }
    Ok(out)
}

fn build_requests(args: &Args) -> Result<Vec<geopm_request_s>, ExporterError> {
    if let Some(ref path) = args.config_path { parse_config(path) } else { default_requests() }
}

fn run_geopm_summary(args: &Args) -> Result<(), ExporterError> {
    let requests = build_requests(args)?;
    let metrics = build_stats_metrics(&requests);
    let mut collector = StatsCollector::new(requests)?;
    // indices: stats order matches C header order
    let period = Duration::from_secs_f64(args.period);
    let gauges = metrics.gauges.clone();
    let port = args.port; // copy for 'static move into thread
    thread::spawn(move || {
        // Simple HTTP endpoint exporting metrics
        let addr = format!("0.0.0.0:{}", port);
        let listener = TcpListener::bind(addr).expect("bind port");
        loop {
            if let Ok((mut stream, _)) = listener.accept() {
                let metric_families = prometheus::gather();
                let mut buffer = Vec::new();
                let encoder = TextEncoder::new();
                encoder.encode(&metric_families, &mut buffer).unwrap();
                let _ = write!(stream, "HTTP/1.1 200 OK\r\nContent-Type: text/plain; version=0.0.4\r\nContent-Length: {}\r\n\r\n", buffer.len());
                let _ = stream.write_all(&buffer);
            }
        }
    });
    sleep(STARTUP_SLEEP); // warm
    let stat_len = 7; // metrics per signal
    loop {
        let start = Instant::now();
        unsafe { geopm_pio_read_batch(); }
        collector.update()?;
        let metric_stats = collector.report()?; // vector length == num signals
        for (sig_idx, m) in metric_stats.iter().enumerate() {
            for s in 0..stat_len { // order matches creation
                let gauge_idx = sig_idx * stat_len + s;
                gauges[gauge_idx].set(m.stats[s]);
            }
        }
        collector.reset()?;
        let elapsed = start.elapsed();
        if elapsed < period { sleep(period - elapsed); }
    }
}

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let args = Args::parse();
    if args.version { println!("geopmexporter-rust 0.1.0"); return Ok(()); }
    if !args.insecure_http {
        if args.certfile.is_none() || args.keyfile.is_none() {
            eprintln!("Error: TLS selected but certfile/keyfile missing");
            std::process::exit(-1);
        }
        eprintln!("Warning: TLS not implemented yet in Rust exporter; using insecure HTTP");
    }
    match args.summary.as_str() {
        "geopm" => run_geopm_summary(&args)?,
        "prometheus" => run_native_prometheus(&args)?,
        other => { eprintln!("Unknown summary '{}'", other); std::process::exit(-1); }
    }
    Ok(())
}

fn run_native_prometheus(args: &Args) -> Result<(), ExporterError> {
    // Similar to Python PrometheusMetricExporter
    let requests = build_requests(args)?;
    // Push signals and classify behavior
    let mut signal_indices = Vec::new();
    let mut counters: Vec<Counter> = Vec::new();
    let mut gauges: Vec<Gauge> = Vec::new();
    let mut behaviors = Vec::new();
    for req in &requests {
        // reconstruct name
        let raw_name = cstr_to_string(&req.name);
        let c_name = CString::new(raw_name.clone()).unwrap();
        let idx = unsafe { geopm_pio_push_signal(c_name.as_ptr(), req.domain_type, req.domain_idx) };
        if idx < 0 { return Err(ExporterError::Other(format!("push_signal failed {}", idx))); }
        signal_indices.push(idx);
        let mut agg=0; let mut fmt=0; let mut beh=0;
        unsafe { geopm_pio_signal_info(c_name.as_ptr(), &mut agg, &mut fmt, &mut beh); }
        behaviors.push(beh);
        // behavior: 1 monotone -> Counter; 2 variable -> treat as Gauge with observe semantics (store last sample); others error
        let metric_base = {
            // append domain if not board
            if req.domain_type == 0 { raw_name } else {
                // domain name
                let mut dom_buf = vec![0i8; GEOPM_NAME_MAX];
                unsafe { geopm_topo_domain_name(req.domain_type, GEOPM_NAME_MAX, dom_buf.as_mut_ptr()); }
                let dom = cstr_to_string(&dom_buf);
                format!("{}_{dom}_{}", raw_name, req.domain_idx)
            }
        };
        let prom_name = sanitize_metric_name(&metric_base);
        if beh == 1 { // monotone -> Counter
            counters.push(register_counter!(prom_name.as_str(), &metric_base).unwrap());
            gauges.push(register_gauge!(format!("{prom_name}_last"), &format!("last sample for {metric_base}")).unwrap());
        } else if beh == 2 { // variable -> Gauge
            gauges.push(register_gauge!(prom_name.as_str(), &metric_base).unwrap());
            counters.push(register_counter!(format!("{prom_name}_count"), &format!("count of samples for {metric_base}")).unwrap());
        } else {
            return Err(ExporterError::Other(format!("Unsupported behavior {} for {}", beh, metric_base)));
        }
    }
    let port = args.port;
    thread::spawn(move || {
        let addr = format!("0.0.0.0:{port}");
        let listener = TcpListener::bind(addr).expect("bind port");
        loop {
            if let Ok((mut stream,_)) = listener.accept() {
                let metric_families = prometheus::gather();
                let mut buffer = Vec::new();
                let encoder = TextEncoder::new();
                encoder.encode(&metric_families, &mut buffer).unwrap();
                let _ = write!(stream, "HTTP/1.1 200 OK\r\nContent-Type: text/plain; version=0.0.4\r\nContent-Length: {}\r\n\r\n", buffer.len());
                let _ = stream.write_all(&buffer);
            }
        }
    });
    sleep(STARTUP_SLEEP);
    let period = Duration::from_secs_f64(args.period);
    let mut last_samples: Vec<Option<f64>> = vec![None; signal_indices.len()];
    loop {
        let start = Instant::now();
        unsafe { geopm_pio_read_batch(); }
        for (i, idx) in signal_indices.iter().enumerate() {
            let mut val = 0f64;
            let rc = unsafe { geopm_pio_sample(*idx, &mut val) };
            if rc != 0 { continue; }
            match behaviors[i] {
                1 => { // monotone use delta for counter
                    if let Some(prev) = last_samples[i] { if val >= prev { counters[i].inc_by(val - prev); } }
                    last_samples[i] = Some(val);
                    // update gauge with last value if created (paired gauge index matches counter index)
                    gauges[i].set(val);
                }
                2 => { // variable -> gauge current, counter increment count
                    gauges[i].set(val);
                    counters[i].inc();
                }
                _ => {}
            }
        }
        let elapsed = start.elapsed();
        if elapsed < period { sleep(period - elapsed); }
    }
}
