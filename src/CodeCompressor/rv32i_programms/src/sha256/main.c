#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "sha256.h"

char *SHA256(char * data)
{
    int strLen = strlen(data);
	SHA256_CTX ctx;
	unsigned char hash[32];
	char* hashStr = malloc(65);
	strcpy(hashStr, "");

	sha256_init(&ctx);
	sha256_update(&ctx, data, strLen);
	sha256_final(&ctx, hash);

	char s[3];
	for (int i = 0; i < 32; i++) {
		sprintf(s, "%02x", hash[i]);
		strcat(hashStr, s);
	}

	return hashStr;
}

int main()
{
    char data[] = "Hello World!";
    char* sha256 = SHA256(data);
    free(sha256);
    return 0;
}