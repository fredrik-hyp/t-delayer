ARG RUNTIME_IMAGE=t-delayer:runtime-base-4.1.2-ubuntu26.04-amd64
FROM ${RUNTIME_IMAGE}
USER root
# PostgreSQL is supervised by single-entrypoint.py, not systemd.
RUN printf '#!/bin/sh\nexit 101\n' > /usr/sbin/policy-rc.d \
    && chmod 755 /usr/sbin/policy-rc.d \
    && apt-get update \
    && apt-get install -y --no-install-recommends postgresql postgresql-client util-linux \
    && rm -rf /var/lib/apt/lists/* /var/lib/postgresql/*
COPY pov-database/installation/ /schema/
COPY docker-runtime/init-db.sh /opt/delayer/init-db.sh
COPY packaging/single-entrypoint.py /opt/delayer/single-entrypoint.py
ENV PGHOST=/run/postgresql PGPORT=5432 PGDATABASE=delayerdb PGUSER=delayer
ENTRYPOINT ["/usr/bin/tini", "--", "python3", "/opt/delayer/single-entrypoint.py"]
