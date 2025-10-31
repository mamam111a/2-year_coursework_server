FROM ubuntu:24.04

ARG UID
ARG GID
ARG TZ

RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        g++ \
        make \
        libssl-dev \
        ca-certificates \
        tzdata \
        locales-all && \
    groupadd -g ${GID} tester && \
    useradd -m -u ${UID} -g ${GID} -s /bin/bash tester && \
    rm -rf /var/lib/apt/lists/*

RUN mkdir -p /app && \
    chown -R ${UID}:${GID} /app && \
    chmod -R u+rwx /app

WORKDIR /app 
COPY --chown=${UID}:${GID} ./server/ /app

RUN g++ -fdiagnostics-color=always -g \
        server.cpp \
        authorization.cpp \
        condition.cpp \
        condition_additional.cpp \
        crud.cpp \
        workingCSV.cpp \
        DBMSbody.cpp \
        server_additional.cpp \
        filelocks.cpp \
        -o server \
        -lssl -lcrypto


ENV LANG=ru_RU.UTF-8
ENV LANGUAGE=ru_RU:ru
ENV LC_ALL=ru_RU.UTF-8

ENV TZ=${TZ}

USER tester
VOLUME ["/app/config"]
CMD ["./server"]
