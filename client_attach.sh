#todo rename geopm_valgrind_client_attach?
#todo combine debug.sh with this and have param to specify unit or mpi test?
#todo detect when ia-gdb is to be used
#todo take binary of interest as param
#HOSTNAME=$SLURM_NODELIST gdb --command=.gdbinit.geopm.local /home/bbaker1/dev/geopm/.libs/geopmbench
#todo if GNU toolchain
#DEBUGGER=gdb
DEBUGGER=gdb-ia
HOSTNAME=mcfly $DEBUGGER --command=.gdbinit.geopm.local /home/bbaker1/dev/geopm/.libs/geopmbench
