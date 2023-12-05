ALTER TABLE streaminterfaceconfigs ADD column nosessionid BOOLEAN default 't';

UPDATE systeminfo SET dbversion=4;

