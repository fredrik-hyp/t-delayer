ALTER TABLE streaminterfaceconfigs ADD COLUMN multipath BOOLEAN DEFAULT 'f';
ALTER TABLE streaminterfaceconfigs ADD COLUMN mploadbalance BOOLEAN DEFAULT 'f';

ALTER TABLE streaminterfacelivestatistics ADD COLUMN pathnumber BIGINT;
ALTER TABLE streaminterfacelivestatistics ADD COLUMN pathname TEXT;

ALTER TABLE streamstats ADD COLUMN pathnumber BIGINT;
ALTER TABLE streamstats ADD COLUMN pathname TEXT;
ALTER TABLE streamstats ADD COLUMN linkdrops BIGINT;

ALTER TABLE streaminterfaceconfigs ADD COLUMN  mcinusecheck BOOLEAN DEFAULT 'f';

ALTER DATABASE postgres SET timezone TO 'UTC';

UPDATE systeminfo SET dbversion=12;

