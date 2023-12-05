ALTER TABLE streamstats ADD COLUMN multipath BOOLEAN DEFAULT 'f';
ALTER TABLE streamstats ADD COLUMN mploadbalance BOOLEAN DEFAULT 'f';

CREATE TABLE delayercoms
(
	id BIGSERIAL PRIMARY KEY,
	streaminterface_id BIGINT REFERENCES streaminterfaces(id) ON DELETE CASCADE,
	action TEXT, -- jump / play /pause / playto
	jumptopos TIMESTAMP,
	startplayat TIMESTAMP,
	playto TIMESTAMP
);

CREATE TABLE delayerstats
(
	id BIGSERIAL PRIMARY KEY,
	streaminterface_id BIGINT REFERENCES streaminterfaces(id) ON DELETE CASCADE,
	state TEXT, -- play / pause
	startplayat TIMESTAMP,
	startpos TIMESTAMP, -- the oldest time for material available
	lastpos TIMESTAMP, -- the newest time for material available
	currpos TIMESTAMP, -- playback from this time
	playto TIMESTAMP
);

 -- state # play / pause
 --startpos  # the oldest time for material available
 --lastpost # the newest time for material available
 -- currpos # playback from this time

UPDATE systeminfo set dbversion=18;
