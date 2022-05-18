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

FROM ubuntu:18.04 as build-prep

RUN apt update && \
    apt install -y build-essential \
    libicu-dev \
    curl \
    g++ \
    git


WORKDIR /root

# Lib Settings
ARG CMAKE_VERSION_DOT=3.15.5
ARG CMAKE_HASH=62e3e7d134a257e13521e306a9d3d1181ab99af8fcae66699c8f98754fc02dda

ARG BOOST_VERSION=1_70_0
ARG BOOST_VERSION_DOT=1.70.0
ARG BOOST_HASH=430ae8354789de4fd19ee52f3b1f739e1fba576f0aded0897c3c2bc00fb38778

ARG OPENSSL_VERSION_DOT=1.1.1n
ARG OPENSSL_HASH=40dceb51a4f6a5275bde0e6bf20ef4b91bfc32ed57c0552e2e8e15463372b17a

# Environment Variables
ENV BOOST_ROOT /root/boost_${BOOST_VERSION}
ENV OPENSSL_ROOT_DIR=/root/openssl

##########################################################
# Split download & compile to use dockers caching layers #
##########################################################

# Download CMake
RUN set -ex \
    && curl https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION_DOT}/cmake-${CMAKE_VERSION_DOT}-Linux-x86_64.sh -OL\
    && echo "${CMAKE_HASH}  cmake-${CMAKE_VERSION_DOT}-Linux-x86_64.sh" | sha256sum -c

# Download Boost
RUN set -ex \
    && curl -L -o  boost_${BOOST_VERSION}.tar.bz2 https://downloads.sourceforge.net/project/boost/boost/${BOOST_VERSION_DOT}/boost_${BOOST_VERSION}.tar.bz2 \
    &&  sha256sum boost_${BOOST_VERSION}.tar.bz2 \
    && echo "${BOOST_HASH}  boost_${BOOST_VERSION}.tar.bz2" | sha256sum -c\
    && tar -xvf boost_${BOOST_VERSION}.tar.bz2


# Download OpenSSL
RUN curl https://www.openssl.org/source/openssl-${OPENSSL_VERSION_DOT}.tar.gz -OL \
    &&  sha256sum openssl-${OPENSSL_VERSION_DOT}.tar.gz \
    && echo "${OPENSSL_HASH} openssl-${OPENSSL_VERSION_DOT}.tar.gz" | sha256sum -c


# Compile CMake
RUN set -ex \
    && mkdir /opt/cmake \
    && sh cmake-3.15.5-Linux-x86_64.sh --prefix=/opt/cmake --skip-license\
    && ln -s /opt/cmake/bin/cmake /usr/local/bin/cmake\
    && cmake --version\
    && rm cmake-3.15.5-Linux-x86_64.sh

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
    mkdir build && cd build &&\
    cmake -D STATIC=TRUE .. &&\
    make -j $make_job_slots daemon simplewallet


#
# Run Zano
#

FROM ubuntu:18.04

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
