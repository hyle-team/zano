###########################################################################################
## zanod Dockerfile
###########################################################################################

#
# Usage:
# (make sure you have correct permission for /var/data/zano-data prior to run!)
#
#   docker run --restart=always -v /var/data/zano-data:/home/zano/.Zano -p 11121:11121 -p 11211:11211 --name=zanod -dit sowle/zano-full-node
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
#   docker build --build-arg OPENSSL_VERSION_DOT=1.1.1n --build-arg OPENSSL_HASH=40dceb51a4f6a5275bde0e6bf20ef4b91bfc32ed57c0552e2e8e15463372b17a -f utils/docker/Dockerfile .
#
# Available Build Args
#   - CMake Version:    CMAKE_VERSION_DOT, CMAKE_HASH
#   - Boost Version:    BOOST_VERSION, BOOST_VERSION_DOT, BOOST_HASH
#   - OpenSSL Version:  OPENSSL_VERSION_DOT, OPENSSL_HASH

#
# Build Zano
#

FROM ubuntu:22.04 as build-prep

ENV DEBIAN_FRONTEND noninteractive

RUN apt update && \
    apt install -y build-essential \
    g++ \
    curl \
    autotools-dev \
    libicu-dev \
    libbz2-dev \
    cmake \
    git \
    screen \
    checkinstall \
    zlib1g-dev \
    libssl-dev \
    bzip2

WORKDIR /root

# Lib Settings
# ARG CMAKE_VERSION_DOT=3.16.9
# ARG CMAKE_HASH=d71eda07d6ecf3964de65a0e36d0b171565e1aced56ba9f53ca3783406b5cacf

ARG BOOST_VERSION=1_84_0
ARG BOOST_VERSION_DOT=1.84.0
ARG BOOST_HASH=cc4b893acf645c9d4b698e9a0f08ca8846aa5d6c68275c14c3e7949c24109454

ARG OPENSSL_VERSION_DOT=1.1.1w
ARG OPENSSL_HASH=cf3098950cb4d853ad95c0841f1f9c6d3dc102dccfcacd521d93925208b76ac8

ARG ZANO_RELEASE_VERSION=2.1.5.397

# Environment Variables
ENV BOOST_ROOT /root/boost_${BOOST_VERSION}
ENV OPENSSL_ROOT_DIR=/root/openssl

##########################################################
# Split download & compile to use dockers caching layers #
##########################################################

# Download CMake
# RUN set -ex \
#     && curl https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION_DOT}/cmake-${CMAKE_VERSION_DOT}-Linux-x86_64.sh -OL\
#     && echo "${CMAKE_HASH}  cmake-${CMAKE_VERSION_DOT}-Linux-x86_64.sh" | sha256sum -c

# Download Boost
RUN set -ex \
    && curl -L -o  boost_${BOOST_VERSION}.tar.bz2 https://downloads.sourceforge.net/project/boost/boost/${BOOST_VERSION_DOT}/boost_${BOOST_VERSION}.tar.bz2 \
    && sha256sum boost_${BOOST_VERSION}.tar.bz2 \
    && echo "${BOOST_HASH}  boost_${BOOST_VERSION}.tar.bz2" | sha256sum -c\
    && tar -xvf boost_${BOOST_VERSION}.tar.bz2


# Download OpenSSL
RUN curl https://www.openssl.org/source/openssl-${OPENSSL_VERSION_DOT}.tar.gz -OL \
    &&  sha256sum openssl-${OPENSSL_VERSION_DOT}.tar.gz \
    && echo "${OPENSSL_HASH} openssl-${OPENSSL_VERSION_DOT}.tar.gz" | sha256sum -c


# Compile CMake
# RUN set -ex \
#     && mkdir /opt/cmake \
#     && sh cmake-3.16.9-Linux-x86_64.sh --prefix=/opt/cmake --skip-license\
#     && ln -s /opt/cmake/bin/cmake /usr/local/bin/cmake\
#     && cmake --version\
#     && rm cmake-3.16.9-Linux-x86_64.sh

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
    && ./config --prefix=/root/openssl --openssldir=/root/openssl shared zlib \
    && make \
    && make test \
    && make install \
    && cd .. \
    && rm -rf openssl-${OPENSSL_VERSION_DOT}

FROM build-prep as build

# Zano

RUN pwd && mem_avail_gb=$(( $(getconf _AVPHYS_PAGES) * $(getconf PAGE_SIZE) / (1024 * 1024 * 1024) )) &&\
    make_job_slots=$(( $mem_avail_gb < 4 ? 1 : $mem_avail_gb / 4)) &&\
    echo make_job_slots=$make_job_slots &&\
    set -x &&\
    git clone --single-branch --recursive https://github.com/hyle-team/zano.git &&\
    cd zano &&\
    git checkout ${ZANO_RELEASE_VERSION} && \
    mkdir build && cd build &&\
    cmake -D STATIC=TRUE .. &&\
    make -j $make_job_slots daemon simplewallet


#
# Run Zano
#

FROM ubuntu:22.04

RUN mkdir -p /home/zano/.Zano

WORKDIR /home/zano
COPY --from=build /root/zano/build/src/zanod /usr/local/bin
COPY --from=build /root/zano/build/src/simplewallet /usr/local/bin

# blockchain loaction
#VOLUME /home/zano/.Zano

EXPOSE 11121 11211
