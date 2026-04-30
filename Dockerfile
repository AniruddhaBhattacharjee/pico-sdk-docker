# Base image with build tools
FROM ubuntu:24.04

# Install dependencies
RUN apt-get update && apt-get install -y \
    cmake \
    git \
    build-essential \
    python3 \
    python3-pip \
    python3-setuptools \
    python3-wheel \
    libnewlib-arm-none-eabi \
    gcc-arm-none-eabi \
    libstdc++-arm-none-eabi-newlib \
    && rm -rf /var/lib/apt/lists/*

# Set up Pico SDK
WORKDIR /opt
RUN git clone -b master https://github.com/raspberrypi/pico-sdk.git \
    && git -C pico-sdk submodule update --init
RUN git clone -b main https://github.com/raspberrypi/FreeRTOS-Kernel.git

# Environment variables
ENV PICO_SDK_PATH=/opt/pico-sdk
ENV FREERTOS_KERNEL_PATH=/opt/FreeRTOS-Kernel

# Default work directory for projects
WORKDIR /workspace
