ALTER TABLE systeminfo ADD COLUMN statdb_remote_url TEXT;
ALTER TABLE systeminfo ADD COLUMN statdb_sleeptime INTEGER;

UPDATE systeminfo SET dbversion=15;