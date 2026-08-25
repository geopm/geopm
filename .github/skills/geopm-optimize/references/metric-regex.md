# Building a metric regex

`--metric-regex` and `regex:` sources scrape one number from the workload's
standard output. Getting this right before the campaign starts saves hours,
because a pattern that never matches turns every trial into a failure.

## Rules

- **Exactly one capturing group.** The captured text must parse as a number.
- Matched against the workload's **stdout**. Output on stderr is not scraped.
- If a pattern matches several times, the **last** match is used, which is
  usually what you want when a workload prints per-iteration progress and then
  a summary.
- Python regular expression syntax.

## Develop against real output first

Capture a real run, then iterate on the file. Never debug a pattern by spending
trials.

```bash
./scripts/geopm-check-workload.sh --save-output out.txt -- ./workload.sh
```

Then test candidates:

```bash
python3 -c "
import re
text = open('out.txt').read()
pattern = r'GFLOPS: ([0-9.]+)'
m = re.findall(pattern, text)
print('matches:', m)
print('value used:', m[-1] if m else 'NO MATCH')
"
```

Or let the checker confirm it end to end:

```bash
./scripts/geopm-check-workload.sh --regex 'GFLOPS: ([0-9.]+)' --runs 3 -- ./workload.sh
```

Three runs also reveals whether the number is stable enough to optimize.

## Worked examples

### A labelled throughput figure

```
Benchmark complete.
GFLOPS: 482.19
```

```
--metric-regex 'GFLOPS: ([0-9.]+)'
```

### Images per second, with units attached

```
Throughput: 1245.8 images/sec
```

```
--metric-regex 'Throughput: ([0-9.]+) images/sec'
```

Including the trailing text guards against matching some other number that
happens to follow "Throughput:" in a future version.

### Elapsed time, minimized

```
Total elapsed time: 128.44 seconds
```

```
--metric-regex 'elapsed time: ([0-9.]+) seconds' --minimize
```

Lower is better here, so the bare `--minimize` flips the default maximize
sense.

### A value inside JSON-ish output

```
{"run": 3, "status": "ok", "score": 91.7, "host": "node01"}
```

```
--metric-regex '"score":\s*([0-9.]+)'
```

Prefer keying on the field name rather than position. If the workload can emit
real JSON, parsing it in a wrapper script and printing a single clean line is
more robust than a regex.

### Scientific notation

```
Final residual = 3.21e-07
```

```
--metric-regex 'residual = ([0-9.eE+-]+)' --minimize
```

Include `e`, `E`, `+`, and `-` in the character class or the exponent is
truncated and the value is wrong rather than missing — a silent failure.

### Per-iteration output with a summary

```
iter 1: 44.2 GF
iter 2: 46.8 GF
iter 3: 47.1 GF
Average: 46.03 GF
```

Match the summary line specifically:

```
--metric-regex 'Average: ([0-9.]+) GF'
```

Matching `([0-9.]+) GF` would also match the per-iteration lines. That happens
to work, because the last match is used and the summary is printed last, but it
breaks the moment the workload prints anything after the summary. Be specific.

## When the workload prints nothing usable

Three options, in order of preference:

1. **Wrap it.** A small script that runs the workload and prints one clean line
   is more robust than any regex, and you control it.

   ```bash
   #!/bin/bash
   start=$(date +%s.%N)
   ./real-workload "$@" > /dev/null
   end=$(date +%s.%N)
   awk -v s="$start" -v e="$end" 'BEGIN{printf "FOM: %.4f\n", 1.0/(e-s)}'
   ```

   Note the reciprocal: a figure of merit is maximized by default, so print
   something where larger is better, or remember to pass `--minimize`.

2. **Use runtime as the objective.** Give no metric flags at all and the
   optimizer minimizes wall-clock time. Adequate for "make it finish sooner".

3. **Use a signal metric.** If the goal is energy or power rather than
   application performance, no scraping is needed:

   ```bash
   --energy-domain cpu --minimize energy
   ```

## Common failures

| Symptom | Cause | Fix |
|---|---|---|
| Never matches | Output goes to stderr | Redirect in a wrapper: `./app 2>&1` |
| Never matches | Pattern anchored to text that varies | Loosen it; test against real output |
| Matches, wrong value | Several groups, or greedy matching | Use exactly one group; make it specific |
| Truncated value | Character class omits `.`, `e`, `-` | Widen the class |
| Works alone, fails in campaign | Workload behaves differently under a frequency cap, for example printing a warning first | Match the summary line specifically |

Under `--penalty auto`, a non-matching regex is scored as a failed trial and the
campaign continues to a meaningless conclusion. Use `--penalty none` for the
smoke test so this fails loudly instead.

## Quoting

Wrap the pattern in single quotes so the shell leaves it alone:

```bash
--metric-regex 'GFLOPS: ([0-9.]+)'          # correct
--metric-regex "GFLOPS: ([0-9.]+)"          # risky: shell may expand
```

In the `--metric` form the prefix is outside the quotes:

```bash
--metric fom=regex:'GFLOPS: ([0-9.]+)'
```
