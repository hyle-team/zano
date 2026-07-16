###########################################################################################
## zanod Dockerfile
###########################################################################################

#
# Usage:
# (make sure you have correct permission for /var/data/zano-data prior to run!)
#
#   docker run --restart=always -v /var/data/zano-data:/home/zano/.Zano -p 0.0.0.0:11121:11121 -p 127.0.0.1:11211:11211 --name=zanod -dit zanoproject/zano-full-node
#
# (11121 - is the default P2P port, better to be accessible from outside; 11211 - is the default RPC port, should be protected, thus the binding addresses above)
#
#
# To get into container and interact with the daemon:
#   docker attach zanod
#
# To detach from container and left it running:
#   Ctrl+P, Ctrl+Q
#
# To stop container:
#   docker stop zanod
#
# To build with different lib versions, pass through --build-arg's
#   docker build --build-arg BRANCH=release --build-arg OPENSSL_VERSION_DOT=3.5.7 --build-arg OPENSSL_HASH=a8c0d28a529ca480f9f36cf5792e2cd21984552a3c8e4aa11a24aa31aeac98e8 -f utils/docker/Dockerfile .
#
# Available Build Args
#   - branch to use:    BRANCH
#   - CMake Version:    CMAKE_VERSION_DOT, CMAKE_HASH
#   - Boost Version:    BOOST_VERSION, BOOST_VERSION_DOT, BOOST_HASH
#   - OpenSSL Version:  OPENSSL_VERSION_DOT, OPENSSL_HASH

#
# Build Zano
#

FROM ubuntu:20.04 AS build-prep

ENV DEBIAN_FRONTEND=noninteractive

RUN apt update && \
    apt install -y build-essential \
    libicu-dev \
    libz-dev \
    curl \
    gcc-9 \
    g++-9 \
    git

WORKDIR /root

# Lib Settings
ARG CMAKE_VERSION_DOT=3.26.3
ARG CMAKE_HASH=8ec0ef24375a1d0e78de2f790b4545d0718acc55fd7e2322ecb8e135696c77fe

ARG BOOST_VERSION=1_84_0
ARG BOOST_VERSION_DOT=1.84.0
ARG BOOST_HASH=cc4b893acf645c9d4b698e9a0f08ca8846aa5d6c68275c14c3e7949c24109454

ARG OPENSSL_VERSION_DOT=3.5.7
ARG OPENSSL_HASH=a8c0d28a529ca480f9f36cf5792e2cd21984552a3c8e4aa11a24aa31aeac98e8

# Environment Variables
ENV BOOST_ROOT=/root/boost_${BOOST_VERSION}
ENV OPENSSL_ROOT_DIR=/root/openssl

##########################################################
# Split download & compile to use dockers caching layers #
##########################################################

# Download CMake
RUN set -ex \
    && curl https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION_DOT}/cmake-${CMAKE_VERSION_DOT}-linux-x86_64.sh -OL\
    && echo "${CMAKE_HASH}  cmake-${CMAKE_VERSION_DOT}-linux-x86_64.sh" | sha256sum -c

# Download Boost
RUN set -ex \
    && curl -L -o  boost_${BOOST_VERSION}.tar.bz2 https://downloads.sourceforge.net/project/boost/boost/${BOOST_VERSION_DOT}/boost_${BOOST_VERSION}.tar.bz2 \
    && sha256sum boost_${BOOST_VERSION}.tar.bz2 \
    && echo "${BOOST_HASH}  boost_${BOOST_VERSION}.tar.bz2" | sha256sum -c\
    && tar -xvf boost_${BOOST_VERSION}.tar.bz2


# Download OpenSSL
RUN curl https://github.com/openssl/openssl/releases/download/openssl-${OPENSSL_VERSION_DOT}/openssl-${OPENSSL_VERSION_DOT}.tar.gz -OL \
    &&  sha256sum openssl-${OPENSSL_VERSION_DOT}.tar.gz \
    && echo "${OPENSSL_HASH} openssl-${OPENSSL_VERSION_DOT}.tar.gz" | sha256sum -c


# Compile CMake
RUN set -ex \
    && mkdir /opt/cmake \
    && sh cmake-${CMAKE_VERSION_DOT}-linux-x86_64.sh --prefix=/opt/cmake --skip-license\
    && ln -s /opt/cmake/bin/cmake /usr/local/bin/cmake\
    && cmake --version\
    && rm cmake-${CMAKE_VERSION_DOT}-linux-x86_64.sh

# Compile Boost
RUN set -ex \
    && cd boost_${BOOST_VERSION} \
    && ./bootstrap.sh --with-libraries=system,filesystem,thread,date_time,chrono,regex,serialization,atomic,program_options,locale,timer,log \
    && ./b2

# Compile OpenSSL
RUN set -ex \
    && tar xaf openssl-${OPENSSL_VERSION_DOT}.tar.gz \
    && rm openssl-${OPENSSL_VERSION_DOT}.tar.gz \
    && cd openssl-${OPENSSL_VERSION_DOT} \
    && ./config --prefix=/root/openssl --openssldir=/root/openssl --libdir=lib no-comp shared \
    && make \
    && make install_sw install_ssldirs \
    && cd .. \
    && rm -rf openssl-${OPENSSL_VERSION_DOT}

FROM build-prep AS build

# Zano

ARG BRANCH=master

# CACHEBUST does nothing, just to enforce zano layer to rebuild each time, regardless of the cached settings / other layers
ARG CACHEBUST=0

RUN echo "cachebust=${CACHEBUST}" && pwd && mem_avail_gb=$(( $(getconf _AVPHYS_PAGES) * $(getconf PAGE_SIZE) / (1024 * 1024 * 1024) )) &&\
    make_job_slots=$(( $mem_avail_gb < 4 ? 1 : $mem_avail_gb / 4)) &&\
    echo make_job_slots=$make_job_slots &&\
    set -x &&\
    git clone --single-branch --branch=${BRANCH} --recursive https://github.com/hyle-team/zano.git &&\
    cd zano &&\
    mkdir build && cd build &&\
    cmake -D STATIC=TRUE .. &&\
    make -j $make_job_slots daemon simplewallet


#
# Run Zano
#

FROM ubuntu:20.04

RUN useradd -ms /bin/bash zano &&\
    mkdir -p /home/zano/.Zano &&\
    chown -R zano:zano /home/zano/.Zano

USER zano:zano

WORKDIR /home/zano
COPY --chown=zano:zano --from=build /root/zano/build/src/zanod .
COPY --chown=zano:zano --from=build /root/zano/build/src/simplewallet .

# blockchain loaction
VOLUME /home/zano/.Zano

EXPOSE 11121 11211

ENTRYPOINT ["./zanod"]
CMD ["--disable-upnp", "--rpc-bind-ip=0.0.0.0", "--log-level=0"]
