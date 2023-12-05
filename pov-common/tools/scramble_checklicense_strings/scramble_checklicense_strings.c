#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include "clear_input_to_scramble.h"

void printenc(FILE *fd, char *variable, char *str)
{
	int count=0, i;
	fprintf(fd, "char %s[] = {\n", variable);
	for (i=0; str[i] != '\0'; i++,count++)
	{
		fprintf(fd, "%d", str[i]-65);
		if(str[i+1] != '\0')
		{
			fprintf(fd, ", ");
		}
		if ((i+1) % 17 == 0)
		{
			fprintf(fd, "\n");
		}
	}
	fprintf(fd, ", %d", rand()%50);
	fprintf(fd, "};\n");
	fprintf(fd, "#define ");
	for (i = 0; variable[i] != '\0'; i++)
	{
		fprintf(fd, "%c", toupper(variable[i]));
	}
	fprintf(fd, "_LEN\t%d\n\n", count);
}

int main(int argc, char **argv)
{
	FILE *scrambledfd = fopen("./scrambled.h", "w");
	if (scrambledfd == NULL)
	{
		fprintf(stderr, "cant open scrambled.h for writing %d - %s\n\n",
			errno, strerror(errno));
		return 1;
	}

	fprintf(scrambledfd, "/**************************************************************************/\n");
	fprintf(scrambledfd, "/* THIS FILE IS GENERATED WITH scramble_checklicense_strings APPLICATION! */\n");
	fprintf(scrambledfd, "/* PLEASE DONT EDIT THIS FILE!                                            */\n");
	fprintf(scrambledfd, "/* PLEASE EDIT clear_input_to_scramble.h!                                 */\n");
	fprintf(scrambledfd, "/**************************************************************************/\n");
	fprintf(scrambledfd, "\n");
	fprintf(scrambledfd, "#ifndef __SCRAMBLED_H__\n");
	fprintf(scrambledfd, "#define __SCRAMBLED_H__\n");

	printenc(scrambledfd, "pubkeyarr\0", pubkeyarr);

	printenc(scrambledfd, "tmppubfile\0", tmppubfile);
	
	printenc(scrambledfd, "importcmd\0", importcmd);

	printenc(scrambledfd, "cryptbin\0", cryptbin);

	printenc(scrambledfd, "readandverifycmd\0", readandverifycmd);

	printenc(scrambledfd, "cryptbinsumma\0", cryptbinsumma);
	printenc(scrambledfd, "verifystr\0", verifystr);
	printenc(scrambledfd, "verifysign\0", verifysign);
	printenc(scrambledfd, "redirectstderr\0", redirectstderr);
	printenc(scrambledfd, "clientemail\0", clientemail);
	printenc(scrambledfd, "streamtransporterstr\0", streamtransporterstr);
	printenc(scrambledfd, "license_belong_to_other_device_str\0", license_belong_to_other_device_str);

	fprintf(scrambledfd, "#endif /* __SCRAMBLED_H__ */\n");
	
	fclose(scrambledfd);

	return 0;
}
