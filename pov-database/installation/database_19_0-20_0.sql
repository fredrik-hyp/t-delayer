ALTER TABLE streaminterfacebackupsinks ADD COLUMN rawudp BOOLEAN default false;
ALTER TABLE streaminterfacebackupsinks ADD COLUMN ttl BIGINT default 1;

CREATE TABLE delayerconfigs
(
	id BIGSERIAL,
	streaminterface_id BIGINT NOT NULL  REFERENCES streaminterfaces(id) ON DELETE CASCADE,
	pathnumber BIGINT NOT NULL,
	playondelete BOOLEAN default 't' NOT NULL
);

UPDATE systeminfo set dbversion=20;