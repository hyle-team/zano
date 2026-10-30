# Mobile remote node

This image runs the checked-out Zano daemon behind nginx. Ubuntu 24.04 supplies
the compiler, Boost, nginx and security-maintained OpenSSL packages. Rebuild with
`--pull --no-cache` to pick up package updates. OpenSSL is linked into the daemon,
so updating a running container's packages alone does not update that binary.

## Build

Run from the repository root:

```sh
git submodule update --init --recursive contrib/miniupnp contrib/jwt-cpp
docker build --pull -f utils/docker/remote-node-mobile/Dockerfile \
  --build-arg BUILD_JOBS=2 -t zano-remote-node .
```

The build uses this checkout, rather than cloning an unrelated moving branch.
The Dockerfile-specific ignore file limits the build context to native source
and these container files. Only the stripped daemon is copied into the runtime.

## Run

A named volume receives the image's data-directory ownership (UID/GID 10001).
For an existing bind mount, give that user write access before starting.

```sh
docker run -d --name zano-remote-node --restart unless-stopped \
  --cap-drop=ALL --security-opt no-new-privileges:true \
  --read-only --tmpfs /tmp:rw,nosuid,nodev,size=64m \
  -v zano-data:/home/zano/.Zano \
  -p 11121:11121 -p 33340:33340 zano-remote-node
docker stop --time 60 zano-remote-node
```

Both services run as user 10001. Daemon RPC is fixed to container loopback on
11211. Publish 33340 for wallet requests and 11121 for peers; do not publish
11211. The entrypoint forwards shutdown signals and stops both services if
either exits. Additional arguments are daemon options, for example
`--no-predownload`. RPC binding, configuration-file and administrative-API
overrides are rejected.

Nginx accepts whole wallet endpoint paths, including the current
`getrandom_outs4.bin` endpoint. Unknown paths and suffixes return 404.
`set_maintainers_info.bin`, mining-control paths and `force_relay` are excluded.
The JSON-RPC endpoint remains available for wallet methods. The daemon's
administrative API remains disabled.

Requests are limited to 10 per second per client address, with a burst of 20,
30 concurrent requests and a 1 MiB request body. Rate and connection rejection
return 429. Backend reads may take up to 120 seconds. The healthcheck tests RPC
availability, not whether blockchain synchronization has completed.

## Reverse proxy or load balancer

Terminate HTTPS at your own reverse proxy or load balancer. Restrict access to
33340 to that proxy if the service should not be reachable directly.

By default, forwarded IP headers are ignored for client identity and rate
limits. When a trusted proxy supplies X-Forwarded-For, mount a configuration
file into `/etc/nginx/trusted-proxies/`:

```nginx
# Replace this with the immediate proxy's actual address or narrow network.
set_real_ip_from 10.20.30.5/32;
```

For example, add
`-v /path/trusted-proxy.conf:/etc/nginx/trusted-proxies/lb.conf:ro` to docker run.
Trust only addresses controlled by your proxy operator. Broad trust ranges let
clients forge the address used for limits. The forwarded header sent onward to
the daemon is replaced with nginx's resolved client address.

To serve the mobile configuration, mount its directory read-only at
`/home/mobile_app_config/general_info`; the wallet requests
`/general_info/config.json`. The legacy exact `/general_info` file response
is also allowed. Other descendants and files are not served.

## Local regression checks

```sh
python3 utils/docker/remote-node-mobile/test_proxy.py
```

The test builds only the runtime-base stage, uses a disposable loopback backend,
and checks routing, body limits, spoofed forwarded headers and rate limiting.
It does not start a blockchain node or contact a public node.
