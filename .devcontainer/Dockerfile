FROM mcr.microsoft.com/vscode/devcontainers/python:3

RUN dpkg --add-architecture i386 && \
    apt-get update && \
    apt-get install -y libc6-i386 python3-pip && \
    pip3 install commitizen && \
    rm -rf /var/lib/apt/lists/*
