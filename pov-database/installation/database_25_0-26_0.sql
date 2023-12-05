ALTER TABLE netnics ADD COLUMN blocktcp BOOLEAN DEFAULT false;
ALTER TABLE netvlans ADD COLUMN blocktcp BOOLEAN DEFAULT false;
ALTER TABLE netbridgenics ADD COLUMN blocktcp BOOLEAN DEFAULT false;

UPDATE systeminfo SET dbversion=26;