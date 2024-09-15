# Use an appropriate base image
FROM ubuntu:20.04

# Set environment variables
ENV DEBIAN_FRONTEND=noninteractive

# Install dependencies
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    python3 \
    automake \
    autotools-dev \
    bsdmainutils \
    build-essential \
    ca-certificates \
    ccache \
    cmake \
    curl \
    git \
    libtool \
    pkg-config \
    gperf && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*