ALTER TABLE streaminterfaceconfigs ADD COLUMN backuptoprimary BOOLEAN DEFAULT 'f';

UPDATE systeminfo SET dbversion=8;