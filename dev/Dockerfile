# x86_64 sanity build container.
#   docker build -t probeme-dev -f dev/Dockerfile dev/
#   docker run --rm --platform linux/amd64 -v "$PWD":/src -w /src probeme-dev \
#       cmake --preset release && cmake --build --preset release && ctest --preset release
FROM ubuntu:24.04

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        build-essential \
        clang \
        cmake \
        pkg-config \
        git \
        ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
