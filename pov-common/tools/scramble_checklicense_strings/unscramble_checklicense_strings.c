#include <stdio.h>
#include <errno.h>
#include "scrambled.h"

void fixstrs()
{
	int i;
	
	for (i = 0; i < PUBKEYARR_LEN; i++)
	{
		pubkeyarr[i] = pubkeyarr[i] + 65;
	}
	pubkeyarr[i] = '\0';
	fprintf(stderr, "pubkeyarr=%s\n", pubkeyarr);
	
	for (i = 0; i < TMPPUBFILE_LEN; i++)
	{
		tmppubfile[i] = tmppubfile[i]+65;
	}
	tmppubfile[i] = '\0';
	
	fprintf(stderr, "tmppubfile=\"%s\"\n", tmppubfile);

	for(i = 0; i < IMPORTCMD_LEN; i++)
	{
		importcmd[i] = importcmd[i]+65;
	}
	importcmd[i] = '\0';
	fprintf(stderr, "importcmd=\"%s\"\n", importcmd);
	
	for (i = 0; i < CRYPTBIN_LEN; i++)
	{
		cryptbin[i] = cryptbin[i] + 65;
	}
	cryptbin[i] = '\0';
	fprintf(stderr, "cryptbin=\"%s\"\n", cryptbin);
	
	for (i = 0; i < READANDVERIFYCMD_LEN; i++)
	{
		readandverifycmd[i]=readandverifycmd[i]+65;
	}
	readandverifycmd[i] = '\0';
	fprintf(stderr, "readandverifycmd=\"%s\"\n", readandverifycmd);
	
	for (i = 0; i < CRYPTBINSUMMA_LEN; i++)
	{
		cryptbinsumma[i] = cryptbinsumma[i]+65;
	}
	cryptbinsumma[i] = '\0';
	fprintf(stderr, "cryptbinsumma=\"%s\"\n", cryptbinsumma);
	
	for (i = 0; i < VERIFYSTR_LEN; i++)
	{
		verifystr[i] = verifystr[i]+65;
	}
	verifystr[i] = '\0';
	fprintf(stderr, "verifystr=\"%s\"\n", verifystr);

	for (i = 0; i < VERIFYSIGN_LEN; i++)
	{
		verifysign[i] = verifysign[i]+65;
	}
	verifysign[i] = '\0';
	fprintf(stderr, "verifysign=\"%s\"\n", verifysign);

	for (i = 0; i < REDIRECTSTDERR_LEN; i++)
	{
		redirectstderr[i] = redirectstderr[i]+65;
	}
	redirectstderr[i] = '\0';
	fprintf(stderr, "redirectstderr=\"%s\"\n", redirectstderr);
	
	for (i = 0; i < CLIENTEMAIL_LEN; i++)
	{
		clientemail[i] = clientemail[i]+65;
	}
	clientemail[i] = '\0';
	fprintf(stderr, "clientemail=\"%s\"\n", clientemail);
	
	for (i = 0; i < STREAMTRANSPORTERSTR_LEN; i++)
	{
		streamtransporterstr[i] = streamtransporterstr[i] +65;
	}
	streamtransporterstr[i] = '\0';
	fprintf(stderr, "streamtransporterstr=\"%s\"\n", streamtransporterstr);

	for (i = 0; i < LICENSE_BELONG_TO_OTHER_DEVICE_STR_LEN; i++)
	{
		license_belong_to_other_device_str[i] = license_belong_to_other_device_str[i] + 65;
	}
	license_belong_to_other_device_str[i] = '\0';
	fprintf(stderr, "license_belong_to_other_device_str=\"%s\"\n", license_belong_to_other_device_str);
}

int main(int argc, char **argv)
{
	fixstrs();
	return 0;
}
