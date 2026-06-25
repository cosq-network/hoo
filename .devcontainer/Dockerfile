FROM mcr.microsoft.com/devcontainers/cpp:1-ubuntu-24.04

ARG LLVM_VERSION=22.1.4
ARG ANTLR_VERSION=4.13.2

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        ca-certificates \
        cmake \
        curl \
        default-jre-headless \
        libcurl4-openssl-dev \
        libedit-dev \
        libgtest-dev \
        libncurses-dev \
        libssl-dev \
        libxml2-dev \
        libzstd-dev \
        ninja-build \
        unzip \
        uuid-dev \
        xz-utils \
        zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*

RUN curl -L -o /tmp/llvm.tar.xz \
        "https://github.com/llvm/llvm-project/releases/download/llvmorg-${LLVM_VERSION}/LLVM-${LLVM_VERSION}-Linux-X64.tar.xz" \
    && mkdir -p /opt/llvm \
    && tar -xf /tmp/llvm.tar.xz -C /opt/llvm --strip-components=1 \
    && rm /tmp/llvm.tar.xz

RUN mkdir -p /opt/antlr \
    && curl -L -o "/opt/antlr/antlr-${ANTLR_VERSION}-complete.jar" \
        "https://www.antlr.org/download/antlr-${ANTLR_VERSION}-complete.jar" \
    && curl -L -o /tmp/antlr4-cpp-runtime.zip \
        "https://www.antlr.org/download/antlr4-cpp-runtime-${ANTLR_VERSION}-source.zip" \
    && unzip -q /tmp/antlr4-cpp-runtime.zip -d /tmp/antlr4-cpp-runtime \
    && cmake -S /tmp/antlr4-cpp-runtime -B /tmp/antlr4-build \
        -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/opt/antlr \
        -DANTLR_BUILD_SHARED=OFF \
        -DANTLR_BUILD_CPP_TESTS=OFF \
    && cmake --build /tmp/antlr4-build --target install -j"$(nproc)" \
    && rm -rf /tmp/antlr4-cpp-runtime /tmp/antlr4-build /tmp/antlr4-cpp-runtime.zip

ENV LLVM_DIR=/opt/llvm/lib/cmake/llvm
ENV ANTLR4_ROOT=/opt/antlr
ENV ANTLR4_JAR_PATH=/opt/antlr/antlr-4.13.2-complete.jar
ENV CMAKE_PREFIX_PATH=/opt/llvm:/opt/antlr
ENV PATH=/opt/llvm/bin:${PATH}
ENV LD_LIBRARY_PATH=/opt/llvm/lib:/opt/antlr/lib
