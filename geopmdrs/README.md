# GEOPM Rust Implementation

To get started with rust, install the toolchain via rustup.  More info can be
found here: https://rustup.rs/

To build the release version of `geopmd-proxy` run:

```
cargo build -r
```

The `build.sh` script builds the debug binary, release binary, and debian
package. Note: Running the `build.sh` script will also install the cargo-deb
package into your Rust environment if it is not already installed.

## geopmd-proxy

Directory contains the implementation for the geopmd gRPC UDS proxy server.  This
proxy server is required to work around the lack of support for UDS credentials
in the C++/Python implementation for grpc:

https://github.com/grpc/grpc/issues/28755

The work-around that we are using for this problem is to create a proxy grpc
server in Rust where UDS credentials are supported. The proxy server runs on an
open permissions UDS file, and then forwards the credentials with the
request/result to/from a root only r/w UDS file monitored by the real geopmd
service process (written in python).
