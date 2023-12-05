CREATE TABLE recordings
(
	id BIGSERIAL PRIMARY KEY,
	streaminterface_id BIGINT REFERENCES streaminterfaces(id) ON DELETE SET NULL,
	name TEXT,
	state TEXT, -- init, recording, done
	recordingstart TIMESTAMP, -- set when recorder starts
	recordingend TIMESTAMP, -- set when recorder stops
	sourceaddress TEXT, -- input address for recording
	sourceport INTEGER -- input port for recording
);


CREATE TABLE recorderstats
(
	id BIGSERIAL PRIMARY KEY,
	streaminterface_id BIGINT REFERENCES streaminterfaces(id) ON DELETE CASCADE,
	state TEXT, -- recording / pause
	startrecordat TIMESTAMP,
	startpos TIMESTAMP, -- the oldest time for material available
	lastpos TIMESTAMP, -- the newest time for material available
	currpos TIMESTAMP, -- playback from this time
	recordto TIMESTAMP
);


CREATE TABLE playercoms
(
	id BIGSERIAL PRIMARY KEY,
	streaminterface_id BIGINT REFERENCES streaminterfaces(id) ON DELETE CASCADE,
	pathnumber BIGINT,
	timestamp TIMESTAMP default now(),
	action TEXT, -- jump / play / pause / playto
	jumptopos TIMESTAMP,
	startplayat TIMESTAMP,
	playto TIMESTAMP
);

CREATE TABLE playerstats
(
	id BIGSERIAL PRIMARY KEY,
	streaminterface_id BIGINT REFERENCES streaminterfaces(id) ON DELETE CASCADE,
	pathnumber BIGINT,
	recording_id BIGINT REFERENCES recordings(id) ON DELETE CASCADE,
	state TEXT, -- play / pause
	startplayat TIMESTAMP,
	startpos TIMESTAMP, -- the oldest time for material available
	lastpos TIMESTAMP, -- the newest time for material available
	currpos TIMESTAMP, -- playback from this time
	playto TIMESTAMP
);

CREATE TABLE sysmons
(
	id BIGSERIAL PRIMARY KEY,
	timestamp TIMESTAMP DEFAULT now(),
	nrofcpus BIGINT,
	cpu1min REAL,
	cpu5min REAL,
	cpu15min REAL
);

ALTER TABLE streaminterfacesources ADD COLUMN recording_id BIGINT REFERENCES recordings(id) ON DELETE SET NULL;
ALTER TABLE streaminterfacesinks ADD COLUMN recording_id BIGINT REFERENCES recordings(id) ON DELETE SET NULL;


UPDATE systeminfo set dbversion=21;
