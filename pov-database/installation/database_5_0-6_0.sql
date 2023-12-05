CREATE INDEX streaminterfacelogs_timestamp_idx ON streaminterfacelogs(timestamp);

UPDATE systeminfo SET dbversion=6;
