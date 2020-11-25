# What is Pennant?

From https://asc.llnl.gov/coral-2-benchmarks:

    PENNANT is a mini-app for hydrodynamics on general unstructured
    meshes in 2D (arbitrary polygons). It makes heavy use of indirect
    addressing and irregular memory access patterns.

A detailed description of the algorithm can be found in the Pennant
repository (downloaded to integration/apps/pennant/PENNANT by the
build.sh script) under doc/pennantdoc.pdf.

# Number of Cores Reserved for Pennant

Pennant AppConfig file (pennant.py) reserves N physical cores per node
where N is the largest even number less than the total number of physical
cores in the node. This way, at least one physical core is reserved for
the OS and GEOPM.

# GEOPM Epoch Selection

Based on the problem size we choose a different build of pennant.
The difference between these builds is how GEOPM epochs are marked:

    epoch100: One epoch every 100 outer loop iterations.
    epoch1: One epoch every outer loop iteration.
    default: No epoch markup.

There is a known issue that epochs occuring too often causes GEOPM to hang due to
epoch handling overwhelming GEOPM control loop. Larger problem sizes are expected to
be able to handle a smaller number of outer loops per epoch since outer loop time
seems to increase with problem size.

The map of which dataset is run with which build is kept in the AppConfig file for
Pennant (pennant.py).