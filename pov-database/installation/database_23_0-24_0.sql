CREATE TABLE files
(
   id BIGSERIAL PRIMARY KEY,
   streaminterface_id BIGINT REFERENCES streaminterfaces(id) ON DELETE CASCADE,
   filename TEXT,
   size BIGINT,
   priority BIGINT, -- 0 = normal 10 high priority
   queuedtime TIMESTAMP DEFAULT now(),
   startsendtime TIMESTAMP,
   stopsendtime TIMESTAMP, 
   transferredbytes BIGINT,
   status TEXT, -- queued, intransit, done
   comment TEXT -- any other 
);

UPDATE systeminfo SET dbversion=24;