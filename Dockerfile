# ============================================================
# Stage 1: Builder — build LLVM 22.1.4 and ANTLR4 4.13.2
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
        libedit-dev \
        libncurses-dev \
        libxml2-dev \
        libzstd-dev \
        python3-dev \
        swig \
    && rm -rf /var/lib/apt/lists/*

# ------------------------------------------------------------------
# LLVM 22.1.4 (llvm, clang, lld, lldb)
# ------------------------------------------------------------------
RUN curl -L -o llvm-project-22.1.4.src.tar.xz \
        https://github.com/llvm/llvm-project/releases/download/llvmorg-22.1.4/llvm-project-22.1.4.src.tar.xz \
    && tar -xf llvm-project-22.1.4.src.tar.xz \
    && rm llvm-project-22.1.4.src.tar.xz

RUN cmake -S llvm-project-22.1.4.src/llvm -B build-llvm \
        -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/opt/llvm \
        -DLLVM_ENABLE_PROJECTS="clang;lld;lldb" \
        -DLLVM_TARGETS_TO_BUILD="X86" \
        -DLLVM_INCLUDE_TESTS=OFF \
        -DLLVM_INCLUDE_EXAMPLES=OFF \
        -DLLVM_INCLUDE_DOCS=OFF \
        -DLLVM_INCLUDE_BENCHMARKS=OFF \
        -DCLANG_INCLUDE_TESTS=OFF \
        -DCLANG_INCLUDE_DOCS=OFF \
    && cmake --build build-llvm --target install -j"$(nproc)" \
    && rm -rf llvm-project-22.1.4.src build-llvm

# ------------------------------------------------------------------
# ANTLR4 4.13.2 — complete JAR (for code generation)
# ------------------------------------------------------------------
RUN mkdir -p /opt/antlr \
    && curl -L -o /opt/antlr/antlr-4.13.2-complete.jar \
        https://www.antlr.org/download/antlr-4.13.2-complete.jar

# ------------------------------------------------------------------
# ANTLR4 4.13.2 — C++ runtime (for linking generated parsers)
# ------------------------------------------------------------------
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
        default-jdk \
        libgtest-dev \
        libcurl4-openssl-dev \
        uuid-dev \
        curl \
        git \
        pkg-config \
        wget \
        zip \
        unzip \
        zsh \
        ca-certificates \
        python3 \
        libedit-dev \
        libncurses-dev \
        libxml2-dev \
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
