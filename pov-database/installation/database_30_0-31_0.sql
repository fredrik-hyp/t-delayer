-- Persist instances created/configured through the container web UI.
CREATE TABLE delayerwebinstances
(
    streaminterface_id BIGINT PRIMARY KEY REFERENCES streaminterfaces(id) ON DELETE CASCADE,
    controller_id TEXT NOT NULL,
    autostart BOOLEAN NOT NULL DEFAULT false
);
CREATE INDEX delayerwebinstances_controller_idx ON delayerwebinstances(controller_id);

UPDATE systeminfo SET dbversion=31;
