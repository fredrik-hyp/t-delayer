#include "common.h"
#include "common-license.h"
#include <openssl/sha.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <stdint.h>
#include <sys/types.h>
#include <dirent.h>

void sha256_hash_string (unsigned char hash[SHA256_DIGEST_LENGTH], char outputBuffer[65])
{
	int i = 0;

	for (i = 0; i < SHA256_DIGEST_LENGTH; i++)
	{
		sprintf(outputBuffer + (i * 2), "%02x", hash[i]);
	}
	outputBuffer[64] = 0;
}

void sha256(char *string, char outputBuffer[256])
{
	unsigned char hash[SHA256_DIGEST_LENGTH];
	SHA256_CTX sha256;
	SHA256_Init(&sha256);
	SHA256_Update(&sha256, string, strlen(string));
	SHA256_Final(hash, &sha256);
	int i = 0;
	for (i = 0; i < SHA256_DIGEST_LENGTH; i++)
	{
		sprintf(outputBuffer + (i * 2), "%02x", hash[i]);
	}
	outputBuffer[64] = 0;
}

int sha256_file(char *path, char outputBuffer[65])
{
	FILE *file = fopen(path, "rb");
	if (!file)
	{
		LOGMSG(LOG_NOTICE, "fopen: %s",
			strerror(errno));
		return errno;
	}

	uint8_t hash[SHA256_DIGEST_LENGTH];
	SHA256_CTX sha256;
	SHA256_Init(&sha256);
	const int bufSize = 32768;
	uint8_t *buffer = malloc(bufSize);
	int bytesRead = 0;
	if (!buffer)
	{
		return ENOMEM;
	}
	while ((bytesRead = fread(buffer, 1, bufSize, file)))
	{
		SHA256_Update(&sha256, buffer, bytesRead);
	}
	SHA256_Final(hash, &sha256);

	sha256_hash_string(hash, outputBuffer);
	fclose(file);
	free(buffer);
	return 0;
}

int readaddress(char *name, char *output, int size)
{
	char filename[1024];
	FILE *fd;

	sprintf(filename, "/sys/class/net/%s/address", name);
	fd = fopen(filename, "r");
	if (fd == NULL)
	{
		return -1;
	}

	fgets(output, size, fd);
	fclose(fd);
	return 0;

}

int getAvailableInterfaceAddresses(char **macaddresses)
{
	DIR *dir;
	struct dirent *ent;
	char mac[24];
	int i=0;

	if ((dir = opendir ("/sys/class/net/")) != NULL)
	{
		/* print all the files and directories within directory */
		while ((ent = readdir (dir)) != NULL)
		{
			if(!strcmp(ent->d_name,"lo"))
			{
				continue;
			}
			else if((!strncmp(ent->d_name, "eth",3) && strlen(ent->d_name) == 4) ||
					(!strncmp(ent->d_name, "em", 2) && strlen(ent->d_name) == 3))
			{
				if (!readaddress(ent->d_name,mac,24))
				{
					mac[17] = 0;
//					fprintf(stderr, "mac is %s\n",mac);
					macaddresses[i] = calloc(20,1);
					strncpy(macaddresses[i],mac,17);
					i++;
				}
			}
		}
		closedir (dir);
	}
	else
	{
		/* could not open directory */
		//  perror ("");
		return -1;
	}
	return i;
}
