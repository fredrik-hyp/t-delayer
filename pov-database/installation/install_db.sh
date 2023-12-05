#!/bin/sh

#cp database_* /tmp
#cp populate_delayer.oh.sql /tmp

su - postgres -c "/usr/bin/dropdb delayerdb"
su - postgres -c "/usr/bin/createdb delayerdb"
su - postgres -c "/usr/bin/psql delayerdb < /tmp/database_1_0.sql"
su - postgres -c "/usr/bin/psql delayerdb < /tmp/database_1_0-2_0.sql"
su - postgres -c "/usr/bin/psql delayerdb < /tmp/database_2_0-3_0.sql"
su - postgres -c "/usr/bin/psql delayerdb < /tmp/database_3_0-4_0.sql"
su - postgres -c "/usr/bin/psql delayerdb < /tmp/database_4_0-5_0.sql"
su - postgres -c "/usr/bin/psql delayerdb < /tmp/database_5_0-6_0.sql"
su - postgres -c "/usr/bin/psql delayerdb < /tmp/database_6_0-7_0.sql"
su - postgres -c "/usr/bin/psql delayerdb < /tmp/database_7_0-8_0.sql"
su - postgres -c "/usr/bin/psql delayerdb < /tmp/database_8_0-9_0.sql"
su - postgres -c "/usr/bin/psql delayerdb < /tmp/database_9_0-10_0.sql"
su - postgres -c "/usr/bin/psql delayerdb < /tmp/database_10_0-11_0.sql"
su - postgres -c "/usr/bin/psql delayerdb < /tmp/database_11_0-12_0.sql"
su - postgres -c "/usr/bin/psql delayerdb < /tmp/database_12_0-13_0.sql"
su - postgres -c "/usr/bin/psql delayerdb < /tmp/database_13_0-14_0.sql"
su - postgres -c "/usr/bin/psql delayerdb < /tmp/database_14_0-15_0.sql"
su - postgres -c "/usr/bin/psql delayerdb < /tmp/database_15_0-16_0.sql"
su - postgres -c "/usr/bin/psql delayerdb < /tmp/database_16_0-17_0.sql"
su - postgres -c "/usr/bin/psql delayerdb < /tmp/database_17_0-18_0.sql"
su - postgres -c "/usr/bin/psql delayerdb < /tmp/database_18_0-19_0.sql"
su - postgres -c "/usr/bin/psql delayerdb < /tmp/database_19_0-20_0.sql"
su - postgres -c "/usr/bin/psql delayerdb < /tmp/database_20_0-21_0.sql"
su - postgres -c "/usr/bin/psql delayerdb < /tmp/database_21_0-22_0.sql"
su - postgres -c "/usr/bin/psql delayerdb < /tmp/database_22_0-23_0.sql"
su - postgres -c "/usr/bin/psql delayerdb < /tmp/database_23_0-24_0.sql"
su - postgres -c "/usr/bin/psql delayerdb < /tmp/database_24_0-25_0.sql"
su - postgres -c "/usr/bin/psql delayerdb < /tmp/database_25_0-26_0.sql"
su - postgres -c "/usr/bin/psql delayerdb < /tmp/database_26_0-27_0.sql"
su - postgres -c "/usr/bin/psql delayerdb < /tmp/database_27_0-28_0.sql"

su - postgres -c "/usr/bin/psql delayerdb < /tmp/populate_delayer.oh.sql"

#populate database, rows are checked for existence before inserting 
#su - postgres -c "psql delayerdb < /tmp/populate_streaminterfaces_rudp_1.0.sql"
