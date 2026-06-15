# ============================================================
# Stage 1: Builder — download pre-built LLVM, build ANTLR4
# ============================================================
FROM ubuntu:24.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        build-essential \
        ninja-build \
        cmake \
        curl \
        ca-certificates \
        xz-utils \
        unzip \
    && rm -rf /var/lib/apt/lists/*

# Pre-built LLVM 22.1.4 (includes clang, lld, lldb, cmake configs)
RUN curl -L -o llvm.tar.xz \
        https://github.com/llvm/llvm-project/releases/download/llvmorg-22.1.4/LLVM-22.1.4-Linux-X64.tar.xz \
    && mkdir -p /opt/llvm \
    && tar -xf llvm.tar.xz -C /opt/llvm --strip-components=1 \
    && rm llvm.tar.xz

# ANTLR4 4.13.2 complete JAR (parser generator)
RUN mkdir -p /opt/antlr \
    && curl -L -o /opt/antlr/antlr-4.13.2-complete.jar \
        https://www.antlr.org/download/antlr-4.13.2-complete.jar

# ANTLR4 C++ runtime from source (must match JAR version)
RUN curl -L -o antlr4-cpp-runtime-4.13.2-source.zip \
        https://www.antlr.org/download/antlr4-cpp-runtime-4.13.2-source.zip \
    && unzip -q antlr4-cpp-runtime-4.13.2-source.zip -d antlr4-cpp-runtime \
    && rm antlr4-cpp-runtime-4.13.2-source.zip

RUN cmake -S antlr4-cpp-runtime -B build-antlr4 \
        -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/opt/antlr \
    && cmake --build build-antlr4 --target install -j"$(nproc)" \
    && rm -rf antlr4-cpp-runtime build-antlr4

# ============================================================
# Stage 2: Final — development environment
# ============================================================
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        build-essential \
        cmake \
        ninja-build \
        default-jre-headless \
        libgtest-dev \
        libcurl4-openssl-dev \
        uuid-dev \
        curl \
        git \
        unzip \
        ca-certificates \
        libedit-dev \
        libncurses-dev \
        libxml2-dev \
        libzstd-dev \
        libssl-dev \
        zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /opt/llvm /opt/llvm
COPY --from=builder /opt/antlr /opt/antlr

ENV LLVM_DIR=/opt/llvm/lib/cmake/llvm
ENV ANTLR4_ROOT=/opt/antlr
ENV ANTLR4_JAR_PATH=/opt/antlr/antlr-4.13.2-complete.jar
ENV CMAKE_PREFIX_PATH=/opt/llvm:/opt/antlr
ENV PATH=/opt/llvm/bin:${PATH}
ENV LD_LIBRARY_PATH=/opt/llvm/lib:/opt/antlr/lib

WORKDIR /workspace
