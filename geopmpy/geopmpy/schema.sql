DROP TABLE IF EXISTS stats;
DROP TABLE IF EXISTS report;
DROP TABLE IF EXISTS policy;

-- Policy species the input configuration
-- (e.g., which agent and which options) for the GEOPM runtime
CREATE TABLE policy (
        policy_id INTEGER PRIMARY KEY AUTOINCREMENT,
        agent TEXT NOT NULL,
        period,
        profile TEXT NOT NULL
);

-- Report specifies the output state of the GEOPM runtime,
-- mapping a time span on each host to an input GEOPM
-- configuration and one or more summarized measurement statistics
CREATE TABLE report (
        report_id INTEGER PRIMARY KEY AUTOINCREMENT,
        host TEXT NOT NULL,
        begin_sec REAL NOT NULL,
        end_sec REAL NOT NULL
);

-- Each stat provides multiple summary statistics of a single output
-- metric from a GEOPM agent over a report's time span.
CREATE TABLE stats (
        report_id INTEGER NOT NULL,
        name TEXT NOT NULL,
        count INTEGER NOT NULL,
        first REAL,
        last REAL,
        min REAL,
        max REAL,
        mean REAL,
        std REAL,
        FOREIGN KEY(report_id) REFERENCES report(report_id)
);
