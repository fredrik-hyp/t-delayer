ALTER TABLE streaminterfacesinks ADD COLUMN maxbps BIGINT;
ALTER TABLE streaminterfacebackupsinks ADD COLUMN maxbps BIGINT;

UPDATE systeminfo SET dbversion=14;