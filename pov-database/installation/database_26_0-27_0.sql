ALTER TABLE systeminfo ADD COLUMN labelapipassw TEXT DEFAULT '';

ALTER TABLE sysmons ADD COLUMN sda0realloc BIGINT;
ALTER TABLE sysmons ADD COLUMN sda1realloc BIGINT;
ALTER TABLE sysmons ADD COLUMN sda2realloc BIGINT;
ALTER TABLE sysmons ADD COLUMN sda3realloc BIGINT;
ALTER TABLE sysmons ADD COLUMN sda4realloc BIGINT;
ALTER TABLE sysmons ADD COLUMN sda5realloc BIGINT;
ALTER TABLE sysmons ADD COLUMN sda6realloc BIGINT;
ALTER TABLE sysmons ADD COLUMN sda7realloc BIGINT;
ALTER TABLE sysmons ADD COLUMN sda8realloc BIGINT;
ALTER TABLE sysmons ADD COLUMN sda9realloc BIGINT;

UPDATE systeminfo SET dbversion=27;
