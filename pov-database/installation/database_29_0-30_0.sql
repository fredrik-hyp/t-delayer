-- Backup outputs belong to the primary sink, rather than to an input.
ALTER TABLE streaminterfacebackupsinks
    DROP CONSTRAINT streaminterfacebackupsinks_streaminterfacesink_id_fkey;
ALTER TABLE streaminterfacebackupsinks
    ADD CONSTRAINT streaminterfacebackupsinks_streaminterfacesink_id_fkey
    FOREIGN KEY (streaminterfacesink_id) REFERENCES streaminterfacesinks(id)
    ON DELETE CASCADE NOT VALID;

-- Runtime statistics are keyed by streaminterface, like the C writer.
ALTER TABLE streaminterfacelivestatistics
    DROP CONSTRAINT streaminterfacelivestatistics_streaminterface_id_fkey;
ALTER TABLE streaminterfacelivestatistics
    ADD CONSTRAINT streaminterfacelivestatistics_streaminterface_id_fkey
    FOREIGN KEY (streaminterface_id) REFERENCES streaminterfaces(id)
    ON DELETE CASCADE NOT VALID;

UPDATE systeminfo SET dbversion=30;
