ALTER TABLE streaminterfacelivestatistics ADD COLUMN multipath BOOLEAN DEFAULT 'f';
ALTER TABLE streaminterfacelivestatistics ADD COLUMN mploadbalance BOOLEAN DEFAULT 'f';
ALTER TABLE streaminterfacelivestatistics ADD COLUMN ttl BIGINT DEFAULT 7;
ALTER TABLE streaminterfacelivestatistics ADD COLUMN hostclientcertificate_id BIGINT REFERENCES clientcertificates(id) ON DELETE RESTRICT;
ALTER TABLE streaminterfacelivestatistics ADD COLUMN talkbackclientcertificate_id BIGINT REFERENCES clientcertificates(id) ON DELETE RESTRICT;


UPDATE systeminfo SET dbversion=13;
