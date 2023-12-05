CREATE TABLE virtualsources(
	id BIGSERIAL PRIMARY KEY,
	revision INT,
	name TEXT,
	httpurl TEXT,
	url TEXT,
	address TEXT,
	port INT
);

ALTER TABLE streaminterfacesources ADD COLUMN virtualsource_id BIGINT REFERENCES virtualsources(id) ON DELETE SET NULL;
ALTER TABLE streaminterfacesources ADD COLUMN vsourcerevision INT;

UPDATE systeminfo set dbversion=3;
