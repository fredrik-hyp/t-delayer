#!/bin/sh

rm -rf delayer-4.0.0
rm delayer-4.0.0.deb

mkdir delayer-4.0.0
cd delayer-4.0.0
mkdir -p usr/local/bin
mkdir -p var/run/delayerd/
mkdir -p etc/systemd/system
mkdir -p tmp
mkdir -p DEBIAN
chmod 755 DEBIAN

# --- DEBIAN/control ---
echo 'Package: delayer' > DEBIAN/control
echo 'Version: 4.0.0' >> DEBIAN/control
echo 'Section: base' >> DEBIAN/control
echo 'Priority: optional' >> DEBIAN/control
echo 'Architecture: amd64' >> DEBIAN/control
echo 'Depends: libpq5, postgresql-14' >> DEBIAN/control
echo 'Maintainer: Christoffer Branzell<christoffer@polylegio.se>' >> DEBIAN/control
echo 'Description: Polylegio Delayerd' >> DEBIAN/control

# --- DEBIAN/postinst create /etc/rc.local ---
echo '#!/bin/sh'              > DEBIAN/postinst 
echo '' >> DEBIAN/postinit
echo 'set -e' >> DEBIAN/postinit
echo '' >> DEBIAN/postinit
echo 'FILE=/etc/rc.local'    >> DEBIAN/postinst 
echo 'if [ -f "$FILE" ]; then'  >> DEBIAN/postinst
echo 'if grep -q delayerd "$FILE"; then'  >> DEBIAN/postinst
echo 'echo "exists"'  >> DEBIAN/postinst
echo 'else'  >> DEBIAN/postinst
echo "sed '/^exit 0.*/i mkdir -p /var/run/delayerd' /etc/rc.local"  >> DEBIAN/postinst
echo "sed '/^exit 0.*/i chmod a+w /var/run/delayerd' /etc/rc.local"  >> DEBIAN/postinst
echo "sed '/^exit 0.*/i chmod a+r /var/run/delayerd' /etc/rc.local"  >> DEBIAN/postinst
echo "sed '/^exit 0.*/i chmod a+x /var/run/delayerd' /etc/rc.local"  >> DEBIAN/postinst
echo 'fi'  >> DEBIAN/postinst
echo 'else '                    >> DEBIAN/postinst 
echo 'echo "#!/bin/sh -e" >> /etc/rc.local'>> DEBIAN/postinst 
echo 'echo "#" >> /etc/rc.local'>> DEBIAN/postinst 
echo 'echo "# rc.local ">> /etc/rc.local'>> DEBIAN/postinst 
echo 'echo "#" >> /etc/rc.local'>> DEBIAN/postinst 
echo 'echo "# This script is executed at the end of each multiuser runlevel." >> /etc/rc.local'>> DEBIAN/postinst 
echo 'echo "# Make sure that the script will "exit 0" on success or any other" >> /etc/rc.local'>> DEBIAN/postinst 
echo 'echo "# value on error." >> /etc/rc.local'>> DEBIAN/postinst 
echo 'echo "#" >> /etc/rc.local'>> DEBIAN/postinst 
echo 'echo "# In order to enable or disable this script just change the execution" >> /etc/rc.local'>> DEBIAN/postinst 
echo 'echo "# bits." >> /etc/rc.local'>> DEBIAN/postinst 
echo 'echo "#" >> /etc/rc.local'>> DEBIAN/postinst 
echo 'echo "# By default this script does nothing." >> /etc/rc.local'>> DEBIAN/postinst 
echo 'echo "mkdir -p /var/run/delayerd" >> /etc/rc.local' >> DEBIAN/postinst 
echo 'echo "chmod a+w /var/run/delayerd" >> /etc/rc.local' >> DEBIAN/postinst 
echo 'echo "chmod a+r /var/run/delayerd" >> /etc/rc.local' >> DEBIAN/postinst 
echo 'echo "chmod a+x /var/run/delayerd" >> /etc/rc.local' >> DEBIAN/postinst 
echo 'echo "exit 0" >> /etc/rc.local' >> DEBIAN/postinst 
echo 'chmod +x /etc/rc.local' >> DEBIAN/postinst 
echo 'echo "Default the delayer use /home/video as the storage point for the delayed streams."' >> DEBIAN/postinit
echo 'echo "If you want other location please configure that.... in the database"' >> DEBIAN/postinit

echo 'fi' >> DEBIAN/postinst

# --- POSTINIT - create /var/run/delayerd ---
echo 'mkdir -p /var/run/delayerd' >> DEBIAN/postinst 
echo 'chmod a+w /var/run/delayerd' >> DEBIAN/postinst 
echo 'chmod a+r /var/run/delayerd' >> DEBIAN/postinst 
echo 'chmod a+x /var/run/delayerd' >> DEBIAN/postinst 

echo 'adduser --disabled-password  --quiet --system \
      --home /var/run/delayerd --no-create-home \
      --gecos "delayer daemon" --group nogroup' >> DEBIAN/postinit

# --- POSTINIT - fix up postgres config ---
# - OBS hardcorded path!! ---
# - OBS hardcorded path!! ---
# - OBS hardcorded path!! ---
# - OBS hardcorded path!! ---
# - OBS hardcorded path!! ---
echo "sed -i  '/^local   all             postgres                                peer/ s/peer/trust/' /etc/postgresql/14/main/pg_hba.conf" >> DEBIAN/postinst
echo "sed -i  '/^host    all             all             127\.0\.0.1\/32            md5/ s/md5/trust/' /etc/postgresql/14/main/pg_hba.conf" >> DEBIAN/postinst
echo "sed -i  '/^local   all             all                                     peer/ s/peer/trust/' /etc/postgresql/14/main/pg_hba.conf" >> DEBIAN/postinst
echo "sed -i  '/^host    all             all             ::1\/128                 scram-sha-256/ s/scram-sha-256/trust/' /etc/postgresql/14/main/pg_hba.conf" >> DEBIAN/postinst

echo 'service postgresql restart' >> DEBIAN/postinst
echo 'sh /tmp/install_db.sh'>> DEBIAN/postinst
echo 'rm /tmp/install_db.sh' >> DEBIAN/postinst
echo 'rm /tmp/database*' >> DEBIAN/postinst
####### TO BE REMOVED ##############
echo 'rm /tmp/populate_delayer.oh.sql' >> DEBIAN/postinst
####### TO BE REMOVED ##############
echo 'service postgresql restart' >> DEBIAN/postinst

#echo 'chmod a+x /usr/local/bin/*.sh' >> DEBIAN/postinst

echo 'systemctl daemon-reload' >> DEBIAN/postinst
#echo 'systemctl enable delayerd@.service' >> DEBIAN/postinst
#echo 'systemctl start delayerd@.service' >> DEBIAN/postinst

chmod 755 DEBIAN/postinst

# --- postrm ---
echo '#!/bin/sh'              > DEBIAN/postrm
echo '' >> DEBIAN/postrm
echo 'set -e' >> DEBIAN/postrm
echo '' >> DEBIAN/postrm
# REMOVE START SCRIPT: Start script seems to be deleted by the apt-get remove-magic.
#echo '/usr/bin/rm /etc/systemd/system/delayerd@.service' >> DEBIAN/postrm
echo 'su - postgres -c "/usr/bin/pg_dump delayerdb > /tmp/backup_delayer_db.sql"' >> DEBIAN/postrm
echo 'su - postgres -c "/usr/bin/dropdb delayerdb"' >> DEBIAN/postrm
# TODO: remove videofiles in other locations...
echo 'if [ -f /home/video/st*.bin ]; then' >> DEBIAN/postrm
echo '   rm /home/video/st*.bin' >> DEBIAN/postrm
echo 'fi' >> DEBIAN/postrm
chmod 755 DEBIAN/postrm

# --- Make delayerd and COPY FILES ---
cd ../../..
sh make.sh -p

cd ..
cp pov-database/installation/database* pov-delayer2/tools/create_package/delayer-4.0.0/tmp
cp pov-database/installation/populate_delayer.oh.sql pov-delayer2/tools/create_package/delayer-4.0.0/tmp
cp pov-database/installation/install_db.sh pov-delayer2/tools/create_package/delayer-4.0.0/tmp

cd pov-delayer2/tools/
cp systemd/delayerd@.service create_package/delayer-4.0.0/etc/systemd/system/

cd create_package

dpkg-deb --build delayer-4.0.0
