CREATE TABLE systeminfo(
	id BIGSERIAL PRIMARY KEY,
	dbversion BIGINT
);

-- INSERT INTO systeminfo (dbversion) values (1) ;
INSERT INTO systeminfo (dbversion) select 1 where not exists (select * from systeminfo);
	
CREATE TABLE users(
	id BIGSERIAL PRIMARY KEY,
	username TEXT NOT NULL UNIQUE,
	password TEXT NOT NULL,
	name TEXT,
	surname TEXT,
	email TEXT,
	admin BOOLEAN DEFAULT false
);

CREATE TABLE usersessions(
	id BIGSERIAL PRIMARY KEY,
	user_id BIGINT NOT NULL REFERENCES users(id) ON DELETE CASCADE,
	started_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
	last_heard_of TIMESTAMP,
	sessionid TEXT
);

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

CREATE TABLE rootcertificates(
	id BIGSERIAL PRIMARY KEY,
	created TIMESTAMP DEFAULT now(),
	expiration_days BIGINT,
	organisation_id BIGINT REFERENCES organisations(id) ON DELETE CASCADE,
	name TEXT,
	public_part TEXT,
	private_part TEXT
);

CREATE TABLE clientcertificates(
	id BIGSERIAL PRIMARY KEY,
	created TIMESTAMP DEFAULT now(),
	expiration_days BIGINT,
	rootcertificate_id BIGINT REFERENCES rootcertificates(id) ON DELETE CASCADE,
	name TEXT,
	public_part TEXT,
	private_part TEXT
);

CREATE TABLE streaminterfaces(
	id BIGSERIAL PRIMARY KEY,
	interfacetype TEXT,  --streamtransporter
	name TEXT UNIQUE,
	usertext TEXT
);

CREATE TABLE streaminterfacelogs(
	id BIGSERIAL PRIMARY KEY,
	streaminterface_id BIGINT REFERENCES streaminterfaces(id) ON DELETE CASCADE,
	timestamp TIMESTAMP NOT NULL DEFAULT now(),
	level INTEGER, -- 1=error, 2=warning, 3=info
	text TEXT 
);

CREATE TABLE streaminterfaceconfigs(
	id BIGSERIAL PRIMARY KEY,
	streaminterface_id BIGINT REFERENCES streaminterfaces(id) ON DELETE CASCADE,
	typeofconfig TEXT,  --running or stored
	autostart BOOLEAN default 'f',
	msbuffer INTEGER,
	resendmode TEXT,
	mode TEXT --transmitter receiver
);

CREATE TABLE streaminterfacesources(
	id BIGSERIAL PRIMARY KEY,
	streaminterfaceconfig_id BIGINT REFERENCES streaminterfaceconfigs(id) ON DELETE CASCADE,
	name TEXT,
	priority integer, --for cases when we have fallback sources
	sourcetype TEXT, --rudp, tcp, udp
	host TEXT,
	port INTEGER,
	talkbackhost TEXT,
	talkbackport INTEGER
);

CREATE TABLE streaminterfacesinks(
	id BIGSERIAL PRIMARY KEY,
	streaminterfaceconfig_id BIGINT REFERENCES streaminterfaceconfigs(id) ON DELETE CASCADE,
	name TEXT,
	sinktype TEXT,
	host TEXT,
	port INTEGER,
	talkbackhost TEXT,
	talkbackport INTEGER
);

CREATE TABLE streaminterfacelivestatistics(
	id BIGSERIAL PRIMARY KEY,
	streaminterface_id BIGINT REFERENCES streaminterfaceconfigs(id) ON DELETE CASCADE,
	mode TEXT,
	state TEXT,
	bitratein BIGINT,
	bitrateout BIGINT,
	bufferlevel BIGINT
);
