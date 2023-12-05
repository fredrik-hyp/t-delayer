ALTER TABLE systeminfo ADD COLUMN statdb_chunksize BIGINT DEFAULT 20;

UPDATE systeminfo set dbversion=17;
