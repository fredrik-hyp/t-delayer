CREATE TABLE streamstats(
    id BIGSERIAL PRIMARY KEY,
    streaminterface_id BIGINT REFERENCES streaminterfaces(id) ON DELETE CASCADE,
    timestamp TIMESTAMP DEFAULT now(),
    streammode TEXT, -- transmitter, receiver
    source_address TEXT, --if receiver use talkback address
    source_port INTEGER,
    sink_address TEXT, -- if transmitter use destination address
    sink_port INTEGER,
    drops INTEGER,
    bitrate BIGINT
);


CREATE INDEX streamstats_timestamp_idx ON streamstats(timestamp);
ALTER TABLE streaminterfaceconfigs ADD COLUMN reordersize INTEGER default 32;

ALTER TABLE streaminterfacesources ADD COLUMN hostclientcertificate_id BIGINT REFERENCES clientcertificates(id) ON DELETE SET NULL;
ALTER TABLE streaminterfacesources ADD COLUMN talkbackclientcertificate_id BIGINT REFERENCES clientcertificates(id) ON DELETE SET NULL;

ALTER TABLE streaminterfacesinks ADD COLUMN hostclientcertificate_id BIGINT REFERENCES clientcertificates(id) ON DELETE SET NULL;
ALTER TABLE streaminterfacesinks ADD COLUMN talkbackclientcertificate_id BIGINT REFERENCES clientcertificates(id) ON DELETE SET NULL;

ALTER TABLE clientcertificates ADD COLUMN server BOOLEAN DEFAULT 'f';

ALTER TABLE streaminterfacelivestatistics ADD COLUMN acc_rtp_drops BIGINT;  
UPDATE systeminfo SET dbversion=5;