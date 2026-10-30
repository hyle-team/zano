# Build from the repository root after initializing the native submodules.
# See README.md beside this file for build and deployment commands.
FROM ubuntu:24.04 AS build-deps
ENV DEBIAN_FRONTEND=noninteractive ZANO_USE_SYSTEM_BOOST=1
RUN apt-get update \
    && apt-get install -y --no-install-recommends \
       build-essential ca-certificates cmake git pkg-config libssl-dev \
       libicu-dev zlib1g-dev libboost-system-dev libboost-filesystem-dev \
       libboost-locale-dev libboost-thread-dev libboost-timer-dev \
       libboost-date-time-dev libboost-chrono-dev libboost-regex-dev \
       libboost-serialization-dev libboost-atomic-dev \
       libboost-program-options-dev libboost-log-dev \
    && rm -rf /var/lib/apt/lists/*

FROM build-deps AS build
WORKDIR /src
COPY CMakeLists.txt ./
COPY contrib/ contrib/
COPY src/ src/
COPY utils/test-static-assert.c utils/test-static-assert.c
ARG BUILD_JOBS=2
RUN cmake -S . -B build -DSTATIC=ON -DBUILD_GUI=OFF \
      -DBUILD_TESTS=OFF -DCOMMIT_ID_IN_VERSION=OFF -DBoost_NO_BOOST_CMAKE=ON -DGIT:FILEPATH= \
    && cmake --build build --target daemon --parallel "$BUILD_JOBS" \
    && strip build/src/zanod

FROM ubuntu:24.04 AS runtime-base
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update \
    && apt-get install -y --no-install-recommends \
       ca-certificates curl nginx tini \
    && rm -rf /var/lib/apt/lists/* \
    && useradd --uid 10001 --create-home --shell /usr/sbin/nologin zano \
    && mkdir -p /home/zano/.Zano /home/mobile_app_config /etc/nginx/trusted-proxies \
    && chown zano:zano /home/zano/.Zano
COPY utils/docker/remote-node-mobile/nginx.conf /etc/nginx/nginx.conf
COPY --chmod=755 utils/docker/remote-node-mobile/entrypoint.sh /usr/local/bin/zano-remote-node
WORKDIR /home/zano
USER 10001:10001
VOLUME /home/zano/.Zano
# The daemon RPC listener stays on container loopback. Only nginx is public.
EXPOSE 11121 33340
ENTRYPOINT ["tini", "-g", "--", "/usr/local/bin/zano-remote-node"]

FROM runtime-base AS runtime
COPY --from=build /src/build/src/zanod /usr/local/bin/zanod
HEALTHCHECK --interval=30s --timeout=5s --start-period=60s \
  CMD curl --fail --silent --output /dev/null --header "Content-Type: application/json" \
      --data "{}" http://127.0.0.1:33340/getheight || exit 1
