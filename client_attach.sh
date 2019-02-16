#todo detect when ia-gdb is to be used
HOSTNAME=$SLURM_NODELIST gdb --command=.gdbinit.geopm.local /home/bbaker1/dev/geopm/.libs/geopmbench
