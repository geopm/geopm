
export GEOPM_SOURCE="${GEOPM_SOURCE?-Set GEOPM_SOURCE to full path of file with initialization parameters}"
export PYTHONPATH=${GEOPM_SOURCE}/:${GEOPM_SOURCE}/geopmpy/:${GEOPM_SOURCE}/geopmdpy/:${HOME}/.local/lib/python3.6/site-packages/:$PYTHONPATH


## Choose your system setup (please comment the section that doesn't apply)

    # Option-1: Using a default GEOPM installation that can be loaded as a module
    module load geopm-runtime
    
    ## Option-2: Using a local GEOPM installation on the system 
    #export GEOPM_INSTALL="${GEOPM_INSTALL?-Set full path to geopm installation directory}"
    #export PATH=${GEOPM_INSTALL}/bin:$PATH
    #export LD_LIBRARY_PATH=${GEOPM_INSTALL}/lib:$LD_LIBRARY_PATH

