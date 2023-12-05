ALTER TABLE delayercoms ADD COLUMN timestamp TIMESTAMP default now();
ALTER TABLE delayercoms ADD COLUMN pathnumber BIGINT;

ALTER TABLE delayerstats ADD COLUMN pathnumber BIGINT;

ALTER TABLE streaminterfacesinks ADD COLUMN outnic TEXT;
  
UPDATE systeminfo set dbversion=19;