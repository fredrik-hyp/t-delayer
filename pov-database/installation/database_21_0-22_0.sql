ALTER TABLE streaminterfacesources ADD COLUMN addrtp BOOLEAN default 'f';

UPDATE systeminfo SET dbversion=22;