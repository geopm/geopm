# Containers

Reaching a host's GEOPM Access Service from inside a container.

> **Status: architecture documented, recipe unvalidated.** The pieces described
> here exist in the repository and in the packages, but this path is not covered
> by GEOPM's `docs/source` documentation, and we have not verified an end-to-end
> container client against a host daemon. Treat the commands below as a starting
> point to test, not as a known-good procedure. If you validate or correct them,
> update this page.

## What is and is not possible

A container cannot run its own Access Service usefully. `geopmd` needs
privileged access to MSRs and sysfs power interfaces, and running a second
privileged daemon alongside the host's would mean two processes writing the same
hardware with independent save/restore state. **Run one daemon, on the host, and
treat the container as a client.**

So the goal is narrow: let client tools inside a container talk to the host's
`geopmd`.

## Two transports

`geopmd` can be reached two ways, and which one your deployment uses determines
the approach.

### D-Bus

The default. `geopmaccess` and the service integration in `libgeopmd` use the
system bus. For a container to reach it, the host's system bus socket has to be
visible inside:

```bash
docker run --rm -it \
  -v /run/dbus/system_bus_socket:/run/dbus/system_bus_socket \
  your-image geopmread CPU_POWER board 0
```

Untested. Expect complications: the container needs a matching UID for the
service to attribute the session correctly, and D-Bus policy in
`io.github.geopm.conf` governs which users may call which methods.

### gRPC over a Unix domain socket

Intended for exactly this case. The relevant component is
[geopmdrs](../../../../geopmdrs), a Rust proxy whose own README explains why it
exists:

> This proxy server is required to work around the lack of support for UDS
> credentials in the C++/Python implementation for grpc. The work-around is to
> create a proxy grpc server in Rust where UDS credentials are supported. The
> proxy server runs on an open permissions UDS file, and then forwards the
> credentials with the request/result to/from a root only r/w UDS file monitored
> by the real geopmd service process.

So the shape is:

```
container client  ->  open-permission UDS  ->  geopmd-proxy (Rust)
                                                    |
                                         root-only UDS  ->  geopmd (Python)
```

The service contract is
[geopm_service.proto](../../../../geopm_service.proto).

On Debian and Ubuntu the proxy ships as the `geopmd-proxy` package, providing
`/usr/bin/geopmd-proxy`.

### Confirming which transport is live

`geopmd-proxy` has no `--help`; it attempts to connect immediately. On a host
where the gRPC path is not enabled it exits with a transport error naming a
missing socket:

```
Error: tonic::transport::Error(Transport, hyper::Error(Connect,
Os { code: 2, kind: NotFound, message: "No such file or directory" }))
```

That is the signature of a D-Bus-only deployment. Note also that the packages
install no `geopmd-proxy` systemd unit — only `geopm.service` — so the proxy
must be started deliberately.

## Suggested procedure

Unverified; validate each step.

1. **Confirm the host is healthy first.** Run the probe and verification
   scripts on the host, not in the container. A container adds a layer of
   failure modes, and diagnosing them on top of a broken host install wastes
   time.
2. **Determine the transport.** If `geopmd-proxy` is not running and no gRPC
   socket exists, you are on D-Bus.
3. **Expose the socket** into the container, read-only where possible.
4. **Match the UID.** Sessions and access lists are attributed per user, so a
   container running as a different UID will get that UID's access, typically
   nothing. `docker run --user "$(id -u):$(id -g)"`.
5. **Install client tools in the container.** The same virtual environment
   recipe applies: [client-venv.md](client-venv.md). The container needs
   `libgeopmd` at a compatible version — see the ABI note in
   [source-build.md](source-build.md).
6. **Verify from inside** with `geopm-verify-install.sh`.

## Kubernetes

Same constraints, more restrictions. A pod reaching a host daemon needs a
`hostPath` volume for the socket, which many clusters forbid, and pod security
policy usually blocks it outright.

Also consider whether it is meaningful. A tuning campaign changes package-wide
or board-wide hardware settings, which affect **every workload on the node**,
not just the pod that requested them. Running `geopmopt` from a pod on a shared
node will distort its neighbours and produce measurements distorted by them in
turn. If you go down this path, use a dedicated node.

## Recommendation

Unless containerization is a hard requirement, run the client tools directly on
the host. The virtual environment already provides the isolation that usually
motivates a container, without any of the socket, UID, and ABI complications,
and it is the configuration this skill has actually validated.
