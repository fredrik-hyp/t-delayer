CREATE TABLE smbsetting
(
	mountfolder TEXT,
	ipaddress TEXT,
	sharename TEXT,
	username TEXT
);

ALTER TABLE streaminterfaceconfigs ADD COLUMN actasserver BOOLEAN DEFAULT 'f';

UPDATE systeminfo SET dbversion=25;
