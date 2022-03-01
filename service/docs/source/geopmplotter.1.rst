.. role:: raw-html-m2r(raw)
   :format: html


geopmplotter(1) -- visualize reports and traces
===============================================






SYNOPSIS
--------

``geopmplotter``  [\ *OPTIONS*\ ] [\ *PATH*\ ]

DESCRIPTION
-----------

Used to produce plots and other analysis files from report and/or trace files.
The *PATH* argument is the input path to be searched for report/trace files.
If *PATH* is not provided, the current working directory is used.

OPTIONS
-------


* 
  ``-h``\ , ``--help``\ :
  show help message and exit.

* 
  ``-r``\ , ``--report_base`` _REPORT\ *BASE*\ :
  The base report string to be searched.  All files that are located in the *PATH* directory
  that begin with _REPORT\ *BASE* will be selected and interpreted as report files.  If this
  option is not specified then all files with the ".report" extension in *PATH* are selected.

* 
  ``-t``\ , ``--trace_base`` _TRACE\ *BASE*\ :
  The base trace string to be searched. All files that are located in the *PATH* directory
  that begin with _TRACE\ *BASE* will be selected and interpreted as trace files.  If this option
  is not specified then all files with the ".trace" extension in *PATH* are selected.  If 'None'
  is specified as the trace base then no trace files will be parsed.

* 
  ``-p``\ , ``--plot_type`` *bar*\ |\ *box*\ |\ *power*\ |\ *epoch*\ |\ *freq*\ |\ *debug*\ :
  The type of plot to be generated (default: *bar*\ ).  Multiple plots can
  be specified by providing a comma separated list (e.g. -p bar,box).  The debug plot can be
  used to explore parsed data from a Python command prompt.

* 
  ``-s``\ , ``--shell``\ :
  Drop to a python shell after plotting.  Useful for data analysis.

* 
  ``-c``\ , ``--csv``\ :
  Generate CSV files for the plotted data.

* 
  ``--normalize``\ :
  Normalize the data that is plotted.  For per-node plots this will also use uniform node
  names in the legend.

* 
  ``-o``\ , ``--output_dir`` _OUTPUT\ *DIR*\ :
  The output directory where files generated from the plotter will go. (default: *figures*\ )

* 
  ``-O``\ , ``--output_types`` *svg*\ |\ *png*\ |\ *pdf*\ |\ *eps*\ |\ *ps*\ :
  The file type(s) for the plot file output. Multiple file types can
  be created simultaneously if a comma separated list of types is
  provided (e.g. -o svg,eps,png). (default: *svg*\ )

* 
  ``-n``\ , ``--profile_name`` _PROFILE\ *NAME*\ :
  Name of the profile to be used in file names / plot titles.
  (default: *"Test Data"*\ )

* 
  ``-m``\ , ``--misc_text`` _MISC\ *TEXT*\ :
  Text to be appended to the plot title.

* 
  ``-v``\ , ``--verbose``\ :
  Print debugging information.

* 
  ``--speedup``\ :
  When a user requests barplots, plot the speedup instead of the raw data
  value.

* 
  ``--ref_version`` *VERSION*\ :
  Use this version string as the reference dataset to compare the target against.

* 
  ``--ref_profile_name`` _PROFILE\ *NAME*\ :
  Use this profile name as the reference dataset to compare the target against.

* 
  ``--ref_plugin`` _PLUGIN\ *NAME*\ :
  Use this tree decider plugin as the reference dataset to compare the target
  against. (default: _power\ *governing*\ )

* 
  ``--tgt_version`` *VERSION*\ :
  Use this version string as the target dataset to compare the reference against.

* 
  ``--tgt_name`` _PROFILE\ *NAME*\ :
  Use this profile name as the target dataset to compare the reference against.

* 
  ``--tgt_plugin`` _PLUGIN\ *NAME*\ :
  Use this tree decider plugin as the target dataset to compare the reference
  against. (default: _power\ *balancing*\ )

* 
  ``--datatype`` *runtime*\ |\ *energy*\ |\ *frequency*\ |\ *count*\ :
  The datatype to be plotted. Only used for report file plots presently. (default: *runtime*\ )

* 
  ``--smooth`` _NUM\ *SAMPLES*\ :
  Perform a _NUM\ *SAMPLES* moving average on on the Y-axis data. Only used for
  trace file plots presently. (default: *1*\ )

* 
  ``--analyze``\ :
  Perform a basic analysis of the data plotted and save the statistics to a file.
  This applies to all trace plots, and additionally does the following to these specific plots:

  *power*\ : Plot the power cap and the aggregate power for all the nodes.\ :raw-html-m2r:`<br>`
  *epoch*\ : Adjust the Y-axis bounds to drop outlier data.

* 
  ``--min_drop`` _BUDGET\ *WATTS*\ :
  The minimum power budget for data to be included in the plot.  All data below this threshold
  will be dropped.

* 
  ``--max_drop`` _BUDGET\ *WATTS*\ :
  The maximum power budget for data to be included in the plot.  All data above this threshold
  will be dropped.

* 
  ``--base_clock`` _FREQ\ *GHZ*\ :
  The base clock frequency in GHz to be used when making calculations from the CLK counter data.

* 
  ``--focus_node`` _NODE\ *NAME*\ :
  The name of a node to focus on when generating per node plots.  This node's data will be highlighted
  in red and will be drawn on top the other nodes.  Analysis data will still be drawn on top of this
  line if enabled with ``--analyze``.

* 
  ``--show``\ :
  This will display an interactive plot using the default Matplotlib backend.

* 
  ``--cache`` _FILE\ *NAME*\ :
  Use _FILE\ *NAME* as the prefix for data cache files to try to load.  If no files are found, data will
  be parsed and cache files will be written in the CWD.  Glob patterns for report and trace files will
  be ignored if the cache files do not exist.  In this case, all report files matching the "*report"
  pattern and all trace files matching the "*trace-*" pattern will be parsed.  When used properly this
  drastically speeds up the time taken to create plots.

  WARNING: If the data used to generate the cache files changes, you must manually delete the
  corresponding .p files.  That is, if _FILE\ *NAME* is "data", remove data_report.p and data_trace.p from
  the CWD.

ENVIRONMENT
-----------


* 
  ``DISPLAY``\ :
  Because the geopmplotter relies on Matplotlib backends for creating plots,
  you may have to manually set this variable for proper functionality even if you
  are not displaying the interactive plot with ``-s``.  E.g.:

  DISPLAY=:0 geopmplotter -v data

* 
  ``MPLBACKEND``\ :
  As an alternative to the above, you can set your backend to "Agg" using either:

  MPLBACKEND="Agg" geopmplotter -v data

  Or you may make the change permanently in your matplotlibrc file  as
  described here:\ :raw-html-m2r:`<br>`
  _https://matplotlib.org/faq/usage_faq.html#what-is-a-backend_.

  The plotter will try to use the default backend (TkAgg), and fall back to Agg automatically
  if the default is not supported.  Additional backends can be used by setting MPLBACKEND
  in the environment.

Using either of these options will prevent ``--show`` from working properly.

EXAMPLES
--------

  Once the geopmpy modules are installed, you can invoke this utility directly from
  the command line.  You can either run the utility from inside a directory containing
  data, or from a remote directory.  Some examples follow:

  Running the utility from the CWD plot the bar and box plots for the current dataset.
  By default, this assumes you have data utilizing the "static_policy" and "power_balancing"
  plugins.  The verbose flag is added to show progress when parsing or plotting.

.. code-block::

   geopmplotter -vp box,bar


  Plot the epoch loop time comparison, frequency, and power plots.  Note that this will
  parse all the trace files present in the CWD.  This could take a considerable amount
  of time for a large dataset.  Similar to the bar plot, the epoch plot assumes the
  data contains output from both the "static_policy" and "power_balancing" plugins.

.. code-block::

   geopmplotter -vp epoch,freq,power


  To plot a single plugin, use the ``--ref_plugin`` and ``--tgt_plugin`` options:

.. code-block::

   geopmplotter -vp epoch --ref_plugin static_policy --tgt_plugin static_policy


  Plot the trace based plots, though only parsing a subset of the trace files.  In this
  instance, the trace files are named based on the global power budget that was set
  at the beginning of the run (e.g. 160-...-trace-\ :raw-html-m2r:`<NODENAME>`\ ).  This can be significantly
  faster to parse for large datasets.

.. code-block::

   geopmplotter -vp epoch,freq,power -t "160*trace"


  The ``--cache`` flag can be used to save any parsed data into a Pickle file for expedited
  data loading on subsequent runs.  Since the plotter will only parse data based on the
  requested plots, to parse and cache all the report and trace data you must request a set
  of plots that require both types of files.  Presently the reports are needed for the bar
  and box plots while all other plots are based on the trace data.  The following requests
  the bar and frequency plots, and saves the parsed data in all_report.p and all_trace.p.

.. code-block::

   geopmplotter -vp bar,freq --cache all


  If all_report.p or all_trace.p exist, they will be loaded and used.  Otherwise, the data
  will be parsed as usual and then saved in these files.  Utilizing the cache will dramatically
  speed up the time needed to create plots.  If you need to re-parse your data, remove these
  files.

  Plot everything utilizing the cache.

.. code-block::

   geopmplotter -vp box,bar,epoch,freq,power --cache all


  Plot the speedup bar plot from 190W to 210W utilizing the cache.

.. code-block::

   geopmplotter -vp bar --speedup --min_drop 190 --max_drop 210 --cache all


  Use ``--analyze`` with the power plot to include the aggregate power and the power cap
  line.

.. code-block::

   geopmplotter -vp power --cache all --analyze


SEE ALSO
--------

`geopm(7) <geopm.7.html>`_\ ,
`geopmpy(7) <geopmpy.7.html>`_\ ,
**geopmanalysis(1)**\ ,
`geopmlaunch(1) <geopmlaunch.1.html>`_\ ,
