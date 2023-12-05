ALTER TABLE systeminfo ADD COLUMN systemname TEXT;

UPDATE systeminfo SET dbversion=9;