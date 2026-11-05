CREATE TABLE remotecontrolclients
(
	id BIGSERIAL PRIMARY KEY,
	name TEXT NOT NULL,
	token_hash TEXT,
	allowed_ip TEXT,
	active BOOLEAN DEFAULT 't',
	created_at TIMESTAMP DEFAULT now(),
	last_used TIMESTAMP,
	CHECK (token_hash IS NOT NULL OR allowed_ip IS NOT NULL)
);

CREATE TABLE outputreservations
(
	id BIGSERIAL PRIMARY KEY,
	streaminterfacesink_id BIGINT REFERENCES streaminterfacesinks(id) ON DELETE SET NULL,
	streaminterface_id BIGINT REFERENCES streaminterfaces(id) ON DELETE SET NULL,
	host TEXT NOT NULL,
	port INTEGER NOT NULL,
	reserved_from TIMESTAMP NOT NULL,
	reserved_until TIMESTAMP NOT NULL,
	owner TEXT,
	request_id TEXT,
	state TEXT DEFAULT 'active',
	created_at TIMESTAMP DEFAULT now(),
	updated_at TIMESTAMP DEFAULT now(),
	CHECK (reserved_until > reserved_from),
	CHECK (state IN ('active', 'released', 'used', 'expired'))
);

CREATE UNIQUE INDEX outputreservations_request_id_uniq
	ON outputreservations(request_id)
	WHERE request_id IS NOT NULL;

CREATE INDEX outputreservations_host_port_state_time_idx
	ON outputreservations(host, port, state, reserved_from, reserved_until);

UPDATE systeminfo SET dbversion=29;
