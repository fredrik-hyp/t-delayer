ALTER TABLE rootcertificates ADD COLUMN notbefore TIMESTAMP;
ALTER TABLE rootcertificates ADD COLUMN notafter TIMESTAMP;
ALTER TABLE clientcertificates ADD COLUMN notbefore TIMESTAMP;
ALTER TABLE clientcertificates ADD COLUMN notafter TIMESTAMP;

UPDATE systeminfo SET dbversion=11;