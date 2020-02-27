The outlier detection script is intended to identify outlier nodes based on
time-series statistics gathered from a benchmark run. Once trace data has
been collected from a benchmark run, the outlier run is invoked with the
individual trace files as arguments:

```
$ python outlierdetection.py trace_dir/*.trace
```

Before executing, please edit `config.py` with the location (absolute or
relative from the directory where this is being executed) of the trace and
report files, a list of the node ids to be analyzed (a python iterable is
fine), the powers used in the sweep (again, a python iterable may be used) and
the iterations you would like analyzed (yet again, go with a python iterable).

Then, run `fit_by_nid.py`; it will emit the ids of outliers according to each
statistic of interest.

Statistic 0: slope of power/frequency curve at sticker.

Statistic 1: Energy consumption at TDP.

Statistic 2: Minimum energy consumption across power caps.

Statistic 3: Power cap at which energy consumption is minimized.

Statistic 4: Processor-Sensor Heat Capacity

Statistic 5: Thermal Conduction to Cooling Fluid (Fan-Driven Air)

Statistic 6: Estimated Cooling Fluid Temperature
