ALTER TABLE virtualsources ALTER COLUMN revision TYPE BIGINT;
ALTER TABLE streaminterfacesources ALTER COLUMN vsourcerevision TYPE BIGINT;

UPDATE systeminfo SET dbversion=10;