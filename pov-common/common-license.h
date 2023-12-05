#ifndef __COMMON_LICENSE_H__
#define __COMMON_LICENSE_H__

#include <openssl/sha.h>

void sha256_hash_string (unsigned char hash[SHA256_DIGEST_LENGTH], char outputBuffer[65]);
void sha256(char *string, char outputBuffer[256]);
int sha256_file(char *path, char outputBuffer[65]);
int getAvailableInterfaceAddresses(char **macaddresses);

#endif /* __COMMON_LICENSE_H__ */
