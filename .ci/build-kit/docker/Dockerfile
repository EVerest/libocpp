# syntax=docker/dockerfile:1
ARG BASE_IMAGE_TAG=latest
FROM ghcr.io/everest/everest-ci/build-kit-base:${BASE_IMAGE_TAG}

# Check if github.com is already in known_hosts, if not, add it
RUN mkdir -p /root/.ssh && \
    grep -q "^github.com " /root/.ssh/known_hosts || \
    ssh-keyscan github.com >> /root/.ssh/known_hosts
