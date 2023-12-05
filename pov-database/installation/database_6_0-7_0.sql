CREATE TABLE virtualsourcegroups(
    id BIGSERIAL PRIMARY KEY,
    virtualsourcegroup_id BIGINT REFERENCES virtualsourcegroups(id) ON DELETE CASCADE,
    name TEXT
);

ALTER TABLE virtualsources ADD COLUMN virtualsourcegroup_id BIGINT REFERENCES virtualsourcegroups(id) ON DELETE CASCADE;

--  add backup sources and sinks
ALTER TABLE streaminterfacesources ADD COLUMN maxretrytime INTEGER;
ALTER TABLE streaminterfacesources ADD COLUMN maxretries INTEGER;
ALTER TABLE streaminterfacesources ADD COLUMN streammissfailovertimeout INTEGER default '0'; -- seconds to wait before failiing over if no input rudp datastream, 0 = disable
ALTER TABLE streaminterfacesinks ADD COLUMN priority INTEGER;
ALTER TABLE streaminterfacesinks ADD COLUMN maxretrytime INTEGER;
ALTER TABLE streaminterfacesinks ADD COLUMN maxretries INTEGER;
ALTER TABLE streaminterfacesinks ADD COLUMN streammissfailovertimeout INTEGER default '0'; -- seconds to wait before failiing over if

ALTER TABLE streamstats ADD COLUMN bitrate_in BIGINT;
ALTER TABLE streamstats ADD COLUMN bitrate_out BIGINT;
ALTER TABLE streamstats ADD COLUMN resendcounter BIGINT;
ALTER TABLE streamstats ADD COLUMN rudprtt INTEGER;

CREATE TABLE streaminterfacebackupsources(
    id BIGSERIAL PRIMARY KEY,
    streaminterfacesource_id BIGINT REFERENCES streaminterfacesources(id) ON DELETE CASCADE,
    name TEXT,
    priority INTEGER, -- lower means higher priority, same priority
    maxretrytime INTEGER, -- seconds to try before moving on to next source 0 = no timeout given
--    maxretries INTEGER, -- number of times to try to connect before moving on 0 means 1
    streammissfailovertimeout INTEGER default '0',
    sourcetype TEXT,
    host TEXT,
    port INTEGER,
    talkbackhost TEXT,
    talkbackport INTEGER,
    hostclientcertificate_id BIGINT REFERENCES clientcertificates(id) ON DELETE SET NULL,
    talkbackclientcertificate_id BIGINT REFERENCES clientcertificates(id) ON DELETE SET NULL
);

CREATE TABLE streaminterfacebackupsinks(
    id BIGSERIAL PRIMARY KEY,
    streaminterfacesink_id BIGINT REFERENCES streaminterfacesources(id) ON DELETE CASCADE,
    name TEXT,
    priority INTEGER, -- lower means higher priority, same priority
    maxretrytime INTEGER, -- seconds to try before moving on to next source 0 = no timeout given
--   maxretries INTEGER, -- number of times to try to connect before moving on 0 means 1
    streammissfailovertimeout INTEGER default '0',
    sinktype TEXT,
    host TEXT,
    port INTEGER,
    talkbackhost TEXT,
    talkbackport INTEGER,
    hostclientcertificate_id BIGINT REFERENCES clientcertificates(id) ON DELETE SET NULL,
    talkbackclientcertificate_id BIGINT REFERENCES clientcertificates(id) ON DELETE SET NULL
);

UPDATE systeminfo SET dbversion=7;
