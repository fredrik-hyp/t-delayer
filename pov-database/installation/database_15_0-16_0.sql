ALTER TABLE streaminterfacesinks ADD COLUMN rawudp BOOLEAN DEFAULT 'f';

UPDATE systeminfo set dbversion=16;