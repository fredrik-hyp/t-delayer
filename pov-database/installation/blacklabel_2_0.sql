CREATE TABLE defaultsettings(
	id BIGSERIAL PRIMARY KEY,
	imstunnelconnectport BIGINT,
	imstunnelconnecthost TEXT
);

INSERT INTO defaultsettings (imstunnelconnecthost, imstunnelconnectport) VALUES ('127.0.0.1', 22);

CREATE TABLE organisations(
	id BIGSERIAL PRIMARY KEY,
	name TEXT,
	address1 TEXT,
	address2 TEXT,
	zipcode TEXT,
	city TEXT,
	country TEXT,
	contactperson TEXT,
	telephone TEXT,
	email TEXT,
	comments TEXT
);

CREATE TABLE organisationanduser(
	organisation_id BIGINT REFERENCES organisations(id) ON DELETE CASCADE,
	user_id BIGINT REFERENCES users(id) ON DELETE CASCADE 
);


CREATE TABLE devices(
	id BIGSERIAL PRIMARY KEY,
	approved BOOLEAN DEFAULT 'f',
	hide BOOLEAN DEFAULT 'f',
	newborn BOOLEAN DEFAULT 't',
	name TEXT,
	serialnumber BIGINT,
	comments TEXT,
	last_heard_of TIMESTAMP DEFAULT now(),
	imscliversion TEXT,
	publickey TEXT,
	public_ip TEXT,
	private_ip1 TEXT,
	private_ip2 TEXT,
	private_ip3 TEXT,
	private_ip4 TEXT
);

CREATE TABLE useranddevice(
	user_id BIGINT REFERENCES users(id) ON DELETE CASCADE,
	device_id BIGINT REFERENCES devices(id) ON DELETE CASCADE
);

CREATE TABLE organisationanddevice(
	organisation_id BIGINT REFERENCES organisations(id) ON DELETE CASCADE,
	device_id BIGINT REFERENCES devices(id) ON DELETE CASCADE
);

CREATE TABLE deviceactions(
	id BIGSERIAL PRIMARY KEY,
	device_id BIGINT NOT NULL REFERENCES devices(id) ON DELETE CASCADE,
	action TEXT,
	connectport BIGINT
);
