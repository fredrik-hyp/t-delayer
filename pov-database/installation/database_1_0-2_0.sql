CREATE TABLE streaminterfaceruninfos(
	id BIGSERIAL PRIMARY KEY,
	streaminterface_id BIGINT REFERENCES streaminterfaces(id) ON DELETE CASCADE,
	pid BIGINT,
	lastchanged TIMESTAMP NOT NULL DEFAULT now(),
	startstopinfo TEXT --autostarted, userstarted, userstopped
);

ALTER TABLE streaminterfacesinks ADD COLUMN ttl INTEGER DEFAULT 7;

ALTER TABLE streaminterfacelivestatistics ADD COLUMN rtt INTEGER;
ALTER TABLE streaminterfacelivestatistics ADD COLUMN timestamp TIMESTAMP DEFAULT now();
ALTER TABLE streaminterfacelivestatistics ADD COLUMN source  TEXT;
ALTER TABLE streaminterfacelivestatistics ADD COLUMN destination TEXT;
ALTER TABLE streaminterfacelivestatistics ADD COLUMN udpdrops TEXT;

UPDATE systeminfo set dbversion=2;
