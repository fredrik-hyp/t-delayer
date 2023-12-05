ALTER TABLE systeminfo ADD COLUMN video_path TEXT DEFAULT '/home/video';

UPDATE systeminfo SET dbversion=28;
