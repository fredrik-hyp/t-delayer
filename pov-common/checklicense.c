#include "checklicense.h"
#include "common-license.h"
#include "common.h"
#include "scrambled.h"
#include <sys/time.h>

#define DEBUG_CHECKLICENSE 0
#define SERIAL_FILENAME "/etc/serialnumber"

int fixedstrs = 0;

void fixstrs()
{
	int i;
	
	if (fixedstrs)
	{
		return;
	}
	
	for (i = 0; i < PUBKEYARR_LEN; i++)
	{
		pubkeyarr[i] = pubkeyarr[i] + 65;
	}
	pubkeyarr[i] = '\0';
	
	for (i = 0; i < TMPPUBFILE_LEN; i++)
	{
		tmppubfile[i] = tmppubfile[i]+65;
	}
	tmppubfile[i] = '\0';
	
	for (i = 0; i < IMPORTCMD_LEN; i++)
	{
		importcmd[i] = importcmd[i]+65;
	}
	importcmd[i] = '\0';
	
	for (i = 0; i < CRYPTBIN_LEN; i++)
	{
		cryptbin[i] = cryptbin[i] + 65;
	}
	cryptbin[i] = '\0';
	
	for (i = 0; i < READANDVERIFYCMD_LEN; i++)
	{
		readandverifycmd[i]=readandverifycmd[i]+65;
	}
	readandverifycmd[i] = '\0';
	
	for (i = 0; i < CRYPTBINSUMMA_LEN; i++)
	{
		cryptbinsumma[i] = cryptbinsumma[i]+65;
	}
	cryptbinsumma[i] = '\0';
	
	for (i = 0; i < VERIFYSTR_LEN; i++)
	{
		verifystr[i] = verifystr[i]+65;
	}
	verifystr[i] = '\0';

	for (i = 0; i < VERIFYSIGN_LEN; i++)
	{
		verifysign[i] = verifysign[i]+65;
	}
	verifysign[i] = '\0';
	
	for (i = 0; i < REDIRECTSTDERR_LEN; i++)
	{
		redirectstderr[i] = redirectstderr[i]+65;
	}
	redirectstderr[i] = '\0';
	
	for (i = 0; i < CLIENTEMAIL_LEN; i++)
	{
		clientemail[i] = clientemail[i]+65;
	}
	clientemail[i] = '\0';
	
	for (i = 0; i < STREAMTRANSPORTERSTR_LEN; i++)
	{
		streamtransporterstr[i] = streamtransporterstr[i] +65;
	}
	streamtransporterstr[i] = '\0';

	for (i = 0; i < LICENSE_BELONG_TO_OTHER_DEVICE_STR_LEN; i++)
	{
		license_belong_to_other_device_str[i] = license_belong_to_other_device_str[i] + 65;
	}
	license_belong_to_other_device_str[i] = '\0';

	fixedstrs=1;
}

uint64_t readSerialnumberFromFile()
{
	uint64_t serialnumber=0;
	FILE *fd;
	char input[128];

	memset(input, 0, 128);
	fd = fopen(SERIAL_FILENAME, "r");
	if (fd == NULL)
	{
		LOGMSG(LOG_NOTICE, "readSerialnumberFromFile: Cant open %s",
			SERIAL_FILENAME);
		return 0;
	}
	fgets(input, 128, fd);
	serialnumber = atol(input);
	fclose(fd);

	return serialnumber;
}

// RETURN 1 = FAILED 0 = FOUND A MATCH
int matchMacaddresses(licensefilecontents *license)
{
	char *macaddresses[25];
	int nrofmacs;
	int i,j,found;

	memset(macaddresses,0,sizeof(macaddresses));
	nrofmacs = getAvailableInterfaceAddresses(macaddresses);
	if (nrofmacs < 1)
	{
#ifdef NODAEMON_DEBUG
		LOGMSG(LOG_NOTICE, "LICENSE: Cant find any interface (%d)",
			nrofmacs);
#endif
		return 1;
	}
	
	if (nrofmacs != license->nrofmacaddresses)
	{
#ifdef NODAEMON_DEBUG
		LOGMSG(LOG_NOTICE, "LICENSE: Number of MAC in licensefile is not the same as in the box (%d %d)",
			nrofmacs, license->nrofmacaddresses);
#endif
		return 1;
	}

	found = 0;
	for (i = 0; i < nrofmacs && found == 0; i++)
	{
		for (j = 0; j < nrofmacs; j++)
		{
			if(strcmp(license->macaddresses[i], macaddresses[j]) == 0)
			{
				found = 1;
				break;
			}
		}
	}
	
	for (i = 0; i < nrofmacs; i++)
	{
		free(macaddresses[i]);
	}

	if (!found)
	{
		return 1;
	}

	return 0;
}

/* expects that readfile have been validated before called*/
int checkpubkey(uint64_t streaminterfaceid)
{
	FILE *fd_pubfile;
	FILE *fd_pipe;
	char buf[10240];
	char line[256];
	char cmd[1024];
	char tmppubfileuniq[TMPPUBFILE_LEN+1024];

	memset(buf, 0, 10240);
	memset(line, 0, 256);
	memset(cmd, 0, 1024);
	memset(tmppubfileuniq, 0, TMPPUBFILE_LEN+1024);
	
	int uniqlen = snprintf(tmppubfileuniq, TMPPUBFILE_LEN+1024, "%s%" PRIu64, tmppubfile, streaminterfaceid);
	if (uniqlen < TMPPUBFILE_LEN)
	{
		LOGMSG(LOG_WARNING, "st%" PRIu64 ": checkpubkey: snprintf failed",
			streaminterfaceid);
		return 1;
	}
	tmppubfileuniq[uniqlen] = '\0';
	
	
	if (access(tmppubfileuniq, F_OK) == 0)
	{
		unlink(tmppubfileuniq);
	}

	fd_pubfile = fopen(tmppubfileuniq, "w");
	if (fd_pubfile == NULL)
	{
		LOGMSG(LOG_NOTICE, "st%" PRIu64 ": checkpubkey: cant open uniq file",
			streaminterfaceid);
		return 1;
	}
	fwrite((char*)pubkeyarr, 1, PUBKEYARR_LEN, fd_pubfile);
	fclose(fd_pubfile);
	usleep(5000);

	sprintf(cmd, "%s %s %s", importcmd, tmppubfileuniq, redirectstderr);
	fd_pipe = popen(cmd, "r");
	if (fd_pipe == NULL)
	{
		unlink(tmppubfileuniq);
#ifdef NODAEMON_DEBUG
		LOGMSG(LOG_NOTICE, "st%" PRIu64 ": checkpubkey: license pipe is null",
			streaminterfaceid);
#endif
		return 1;
	}

	ssize_t readBytes = fread(buf, 1, 10240, fd_pipe);
	pclose(fd_pipe);
	unlink(tmppubfileuniq);

	int startLine = 0;
	for (int counter = 0; counter < readBytes; counter++)
	{
		if (buf[counter] == '\n')
		{
			int linelength = counter - startLine + 1;
			buf[counter] = '\0';

			if (linelength == VERIFYSTR_LEN &&
				strncmp(verifystr, buf+startLine, VERIFYSTR_LEN-1) == 0)
			{
				return 0;
			}
			startLine = counter+1;
		}
	}
	return 1;
}

// returns number of streamtransporters licensed
uint16_t checklic(uint64_t streaminterfaceid)
{
	char footer[300];
	char buf[10240];
	char result[65];
	FILE *fd;
	licensefilecontents *contents;
	povlicense *licenses;
	int counter = 0, newline = 0;
	int startpos = 0;

	// === Zero strings ===
	memset(footer, 0, sizeof(footer));
	memset(buf, 0, sizeof(buf));
	memset(result, 0, sizeof(result));
	
	// === DESCRABLE STRINGS ===
	fixstrs();
	
	// === CHECK INTEGRITY FILE ===
	int sha256_file_crypt = sha256_file(cryptbin, result);
	if (sha256_file_crypt || strcmp(result,cryptbinsumma))
	{
#ifdef NODAEMON_DEBUG
		LOGMSG(LOG_NOTICE, "st%" PRIu64 ": LICENSE: sha256 sum of application failed %d",
			streaminterfaceid, errno);
#else
		LOGMSG(LOG_NOTICE, "st%" PRIu64 ": #1 License error %d - %d",
			streaminterfaceid, sha256_file_crypt, errno);
#endif
		return 0;
	}

	// === CHECK PUBLIC KEY ===
	if (checkpubkey(streaminterfaceid))
	{
#ifdef NODAEMON_DEBUG
		LOGMSG(LOG_NOTICE, "st%" PRIu64 ": LICENSE: check of public key failed. %d",
			streaminterfaceid, errno);
#else
		LOGMSG(LOG_NOTICE, "st%" PRIu64 ": #2 License error %d",
			streaminterfaceid, errno);
#endif
		return 0;
	}

	// === DECRYPT LICENSE FILE ===
	fd = popen(readandverifycmd, "r");

	if (fd == NULL)
	{
#ifdef NODAEMON_DEBUG
		LOGMSG(LOG_NOTICE, "st%" PRIu64 ": LICENSE: Cant execute application. %d",
			streaminterfaceid, errno);
#else
		LOGMSG(LOG_NOTICE, "st%" PRIu64 ": #3 License error %d",
			streaminterfaceid, errno);
#endif
		return 0;
	}

	// ==== READ EVERYTHING FROM STDOUT TO A BUFFER - CLOSE PIPE ====
	ssize_t readBytes = fread(buf, 1, 10240, fd);
	pclose(fd);

	// === PARSE FILE ===
#if DEBUG_CHECKLICENSE && defined(NODAEMON_DEBUG)
	LOGMSG(LOG_NOTICE, "read %zd bytes", readBytes);
	LOGMSG(LOG_NOTICE, "sizeof(licensefilecontents): %zd", sizeof(licensefilecontents));
	LOGMSG(LOG_NOTICE, "sizeof(povlicense): %zd", sizeof(povlicense));
#endif

	// === GRAB THE HEADER OUTPUT FROM GPG ===
	for (counter = 0; counter < 400 && counter < readBytes; counter++)
	{
		if (buf[counter] == '\n')
		{
			if (newline == 0 &&
				strncmp(buf, "gpg: WARNING: standard input reopened", counter-1) == 0)
			{
				startpos = counter+1;
				continue;
			}

			// === IF WE CANT DECRYPT FILE - THIS FILE BELONGS TO OTHER DEVICE OR KEYS ARE MISSING ===
			if (strncmp(buf+startpos, license_belong_to_other_device_str, counter-startpos) == 0)
			{
#ifdef NODAEMON_DEBUG
				LOGMSG(LOG_NOTICE, "st%" PRIu64 ": LICENSE: License file belongs to other hardware. %d",
					streaminterfaceid, errno);
#else
				LOGMSG(LOG_NOTICE, "st%" PRIu64 ": #4 License error %d",
					streaminterfaceid, errno);
#endif
				return 0;
			}

			newline++;
			if (newline == 2)
			{
				break;
			}
			startpos = counter+1;
		}
	}
	buf[counter] = '\0';

#if DEBUG_CHECKLICENSE && defined(NODAEMON_DEBUG)
	LOGMSG(LOG_NOTICE, "header: %s", buf);
	LOGMSG(LOG_NOTICE, "license_belong_to_other_device_str: %s", license_belong_to_other_device_str);
#endif

	// === ASSUME THAT THE DECRYPTION WAS SUCCESSFUL - APPLY licensefilecontents STRUCTURE ON THE MEMORY ===
	contents = (licensefilecontents *)(buf+counter+1);

#if DEBUG_CHECKLICENSE && defined(NODAEMON_DEBUG)
	LOGMSG(LOG_NOTICE, "serialnumber: %s", contents->serialnumber);
	LOGMSG(LOG_NOTICE, "serialnumberlength: %d", contents->serialnumberlength);
	LOGMSG(LOG_NOTICE, "nrofmacaddresses: %d", contents->nrofmacaddresses);
	for (int i = 0; i < contents->nrofmacaddresses; i++)
	{
		LOGMSG(LOG_NOTICE, "macadresses: %s", contents->macaddresses[i]);
	}
	LOGMSG(LOG_NOTICE, "numberofinterfacetypes: %d", contents->numberofinterfacetypes);
#endif

	// === CHECK THAT SERIAL NUMBER ON THE DEVICE MATCHES LICENSEFILE ===
	uint64_t localSerialNumber = readSerialnumberFromFile();
	
#if DEBUG_CHECKLICENSE && defined(NODAEMON_DEBUG)
	LOGMSG(LOG_NOTICE, "%" PRIu64 " (%s)", localSerialNumber, contents->serialnumber);
#endif
	if (localSerialNumber != atol(contents->serialnumber))
	{
#ifdef NODAEMON_DEBUG
		LOGMSG(LOG_NOTICE, "st%" PRIu64 ": LICENSE: serial number doesnt match the one in licensefile. %d",
			streaminterfaceid, errno);
#else
		LOGMSG(LOG_NOTICE, "st%" PRIu64 ": #5 License error %d",
			streaminterfaceid, errno);
#endif
		return 0;
	}

	// === CHECK THAT MAC ADDRESSES ON THE INTERFACE MATCH ON THE SYSTEM ===
	if (matchMacaddresses(contents))
	{
#ifdef NODAEMON_DEBUG
		LOGMSG(LOG_NOTICE, "st%" PRIu64 ": LICENSE: MAC addresses doesnt match the one in licensefile. %d",
			streaminterfaceid, errno);
#else
		LOGMSG(LOG_NOTICE, "st%" PRIu64 ": #6 License error %d",
			streaminterfaceid, errno);
#endif
		return 0;
	}

	// === IF THE HEADER CHECKED OUT OK LET INSPECT povlicense ===
	licenses = (povlicense *)(buf+counter+1+sizeof(licensefilecontents));

#if DEBUG_CHECKLICENSE && defined(NODAEMON_DEBUG)
	LOGMSG(LOG_NOTICE, "interfacetypelength: %d", licenses->interfacetypelength);
	LOGMSG(LOG_NOTICE, "interfacetype: %s", licenses->interfacetype);
	LOGMSG(LOG_NOTICE, "numberofinterfaces: %d", licenses->numberofinterfaces);
	LOGMSG(LOG_NOTICE, "validuntil: %zd", licenses->validuntil);
	LOGMSG(LOG_NOTICE, "extlength: %d", licenses->extlength);
	LOGMSG(LOG_NOTICE, "%s", (char *)(buf+counter+1+sizeof(licensefilecontents)+sizeof(povlicense)+1));
#endif

	if (licenses->extlength > 0)
	{
#if DEBUG_CHECKLICENSE && defined(NODAEMON_DEBUG)
		LOGMSG(LOG_NOTICE, "extlength: %d", licenses->extlength);
#endif
		licenses->ext = (char *)(buf+counter+1+sizeof(licensefilecontents)+sizeof(povlicense));
	}

	// === CHECK LICENSE TYPE ===
	if(strcmp(licenses->interfacetype, streamtransporterstr))
	{
#ifdef NODAEMON_DEBUG
		LOGMSG(LOG_NOTICE, "st%" PRIu64 ": LICENSE: interface type doesnt match. %d",
			streaminterfaceid, errno);
#else
		LOGMSG(LOG_NOTICE, "st%" PRIu64 ": #7 License error %d",
			streaminterfaceid, errno);
#endif
		return 0;
	}

	// === CHECK LICENSE STILL VALID ===
	struct timeval tv;
	gettimeofday(&tv,NULL);
	
	if (tv.tv_sec > licenses->validuntil && licenses->validuntil != 0)
	{
#ifdef NODAEMON_DEBUG
		LOGMSG(LOG_NOTICE, "st%" PRIu64 ": License has expired! %d",
			streaminterfaceid, errno);
#else
		LOGMSG(LOG_NOTICE, "st%" PRIu64 ": #8 License error %d",
			streaminterfaceid, errno);
#endif
		return 0;
	}

	// === FOOTER ===
	int footer_offset = counter+1+sizeof(licensefilecontents)+sizeof(povlicense)+1;
	int footer_length =  readBytes - footer_offset;
	int footer_counter = 0;

	strncpy(footer, (char *)(buf+footer_offset), footer_length); 

	for (footer_counter = 0; footer_counter < footer_length; footer_counter++)
	{
		if (footer[footer_counter] == '\n')
		{
			footer_counter++;
			break;
		}
	}

	// === COMPARE TO GOOD SIGNATURE ===
	if (((footer_length - footer_counter) != VERIFYSIGN_LEN) || strncmp(footer+footer_counter, verifysign, VERIFYSIGN_LEN) != 0)
	{
#ifdef NODAEMON_DEBUG
		LOGMSG(LOG_NOTICE, "st%" PRIu64 ": Cant find Good signature %d",
			streaminterfaceid, errno);
#else
		LOGMSG(LOG_NOTICE, "st%" PRIu64 ": #9 License error %d",
			streaminterfaceid, errno);
#endif
		return 0;
	}

#if DEBUG_CHECKLICENSE && defined(NODAEMON_DEBUG)
	LOGMSG(LOG_NOTICE, "licenses->numberofinterfaces: %d", licenses->numberofinterfaces);
#endif

	// === WELL SEEMS TO BE FINE - RETURN NUMBER OF INTERFACES ===
	return licenses->numberofinterfaces;
}
