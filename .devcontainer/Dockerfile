FROM ubuntu:24.04 AS buildenv

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        build-essential \
        ca-certificates \
        clang \
        clang-tools \
        cppcheck \
        curl \
        faketime \
        gdb \
        git \
        gnupg \
        protobuf-c-compiler \
        zip \
    && mkdir -p /etc/apt/keyrings \
    && curl -fsSL https://deb.nodesource.com/gpgkey/nodesource-repo.gpg.key \
        | gpg --dearmor -o /etc/apt/keyrings/nodesource.gpg \
    && echo "deb [signed-by=/etc/apt/keyrings/nodesource.gpg] https://deb.nodesource.com/node_22.x nodistro main" \
        > /etc/apt/sources.list.d/nodesource.list \
    && apt-get update \
    && apt-get install -y --no-install-recommends nodejs \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

#COPY . /buildenv
#WORKDIR /buildenv
#
#RUN curl -f https://raw.githubusercontent.com/toniebox-reverse-engineering/tonies-json/release/tonies.json -o /buildenv/install/pre/config/tonies.json || true
#RUN make zip
