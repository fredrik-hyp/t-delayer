#!/bin/sh
# Runs only against a NEW postgres data volume, via the official entrypoint.
set -eu
psql -v ON_ERROR_STOP=1 --username "$POSTGRES_USER" --dbname "$POSTGRES_DB" -f /schema/database_1_0.sql
version=1
while [ "$version" -lt 31 ]; do
    next=$((version + 1))
    psql -v ON_ERROR_STOP=1 --username "$POSTGRES_USER" --dbname "$POSTGRES_DB" \
        -f "/schema/database_${version}_0-${next}_0.sql"
    version=$next
done
psql -v ON_ERROR_STOP=1 --username "$POSTGRES_USER" --dbname "$POSTGRES_DB" \
    -c "ALTER DATABASE delayerdb SET timezone TO 'UTC'"
