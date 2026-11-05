FROM ubuntu:26.04 AS build
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends build-essential libpq-dev libncurses-dev \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /src
COPY pov-common/ pov-common/
COPY pov-database/src/c/ pov-database/src/c/
COPY pov-delayer2/ pov-delayer2/
RUN cd pov-delayer2 && ./make.sh -d && test -x delayer && ./make.sh -f \
    && cd tools/read_chunk_file && ./make.sh && test -x read_chunk_file

FROM ubuntu:26.04
ENV DEBIAN_FRONTEND=noninteractive PYTHONUNBUFFERED=1 PYTHONDONTWRITEBYTECODE=1 \
    PGHOST=database PGPORT=5432 PGDATABASE=delayerdb PGUSER=delayer PGCONNECT_TIMEOUT=5 PGTZ=UTC \
    WEB_PORT=8080 WEB_USERNAME=admin DELAYER_CONFIG=/etc/delayer/instances.json
RUN apt-get update && apt-get install -y --no-install-recommends \
    libpq5 nginx python3 python3-psycopg2 openssl tini ca-certificates \
    && rm -rf /var/lib/apt/lists/* /etc/nginx/sites-enabled/default \
    && mkdir -p /run/delayerd /run/delayer /home/video
COPY --from=build /src/pov-delayer2/delayer /usr/local/bin/delayer
COPY --from=build /src/pov-delayer2/tools/read_chunk_file/read_chunk_file /usr/local/bin/read_chunk_file
COPY docker-runtime/control.py docker-runtime/entrypoint.py /opt/delayer/
COPY docker-runtime/nginx.conf.template /opt/delayer/
COPY docker-runtime/web/ /opt/delayer/web/
EXPOSE 8080
STOPSIGNAL SIGTERM
HEALTHCHECK --interval=15s --timeout=8s --start-period=30s --retries=3 \
    CMD python3 /opt/delayer/control.py --healthcheck
ENTRYPOINT ["/usr/bin/tini", "--", "python3", "/opt/delayer/entrypoint.py"]
