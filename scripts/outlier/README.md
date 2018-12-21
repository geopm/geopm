The outlier detection script is intended to identify outlier nodes based on
time-series statistics gathered from a benchmark run. Once trace data has
been collected from a benchmark run, the outlier run is invoked with the
individual trace files as arguments:

```
$ python outlierdetection.py trace_dir/*.trace
```

This will (eventually) emit a table out outlier nodes:

```
OUTLIERS IDENTIFIED:
Node 3135, 95.30%, Runt,  10.8W,  0.9C
Node 2810, 96.40%, Pick, -12.9W, -3.3C
Node 2938, 97.80%, Pick, -12.5W, -2.9C
Node  733, 98.40%, Runt,  18.5W, -1.9C
Node 2425, 99.89%, Pick, - 5.7W, -7.0C
Node 1805, 99.95%, Runt,  15.4W,  6.4C
Node 1709, 99.99%, Pick, -18.1W, -3.9C
```

This is the node id, the probability that the node is an outlier, a string
indicating whether this is especially strong or weak performance, and the
difference between mean power and temperature for this node versus the ensemble
(positive means that the node used more power or exhibited a higher temperature
than average).
