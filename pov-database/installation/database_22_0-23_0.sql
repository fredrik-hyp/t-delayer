CREATE TABLE netnics
(
	id BIGSERIAL PRIMARY KEY,
	physicalname TEXT UNIQUE NOT NULL,
	name TEXT,
	inuse BOOLEAN DEFAULT false,
	description TEXT,
	dhcp BOOLEAN DEFAULT false,
	address TEXT,
	netmask TEXT
);

CREATE TABLE netbridgenics
(
	id BIGSERIAL PRIMARY KEY,
	netnic1_id	BIGINT REFERENCES netnics(id) ON DELETE SET NULL,
	netnic2_id	BIGINT REFERENCES netnics(id) ON DELETE SET NULL,
	netnic3_id	BIGINT REFERENCES netnics(id) ON DELETE SET NULL,
	netnic4_id	BIGINT REFERENCES netnics(id) ON DELETE SET NULL,
	netnic5_id	BIGINT REFERENCES netnics(id) ON DELETE SET NULL,
	netnic6_id	BIGINT REFERENCES netnics(id) ON DELETE SET NULL,
	netnic7_id	BIGINT REFERENCES netnics(id) ON DELETE SET NULL,
	netnic8_id	BIGINT REFERENCES netnics(id) ON DELETE SET NULL,
	name TEXT,
	inuse BOOLEAN DEFAULT false,
	description TEXT,
	dhcp BOOLEAN DEFAULT false,
	address TEXT,
	netmask TEXT
);

CREATE TABLE netvlans
(
	id BIGSERIAL PRIMARY KEY,
	netnic_id BIGINT REFERENCES netnics(id) ON DELETE CASCADE,
	netbridgenic_id BIGINT REFERENCES netbridgenics(id) ON DELETE CASCADE,
	vlanid BIGINT NOT NULL,
	inuse BOOLEAN DEFAULT false,
	description TEXT,
	dhcp BOOLEAN DEFAULT false,
	address TEXT,
	netmask TEXT
);

CREATE TABLE netroutes
(
	id BIGSERIAL PRIMARY KEY,
	netnic_id BIGINT REFERENCES netnics(id) ON DELETE CASCADE,
	netvlan_id BIGINT REFERENCES netvlans(id) ON DELETE CASCADE,
	network TEXT,
	netmask TEXT,
	gateway TEXT,
	inuse BOOLEAN DEFAULT false,
	description TEXT
);

CREATE TYPE netservicetype AS ENUM
(
	'dns',
	'ntp',
	'defgw',
	'cinrelay'	
);

CREATE TABLE netservices
(
	id BIGSERIAL PRIMARY KEY,
	type netservicetype NOT NULL,
	address TEXT,
	options TEXT
);

UPDATE systeminfo SET dbversion=23;