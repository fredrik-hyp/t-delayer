#!/bin/sh

su - postgres -c "psql poverix < /tmp/blacklabel_2_0.sql"

