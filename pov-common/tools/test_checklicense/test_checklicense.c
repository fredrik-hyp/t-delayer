#include "t_checklicense.h"

int main (int argc, char **argv)
{
	if(argc < 2)
	{
		printf("Need license file as input!\n");
		exit(1);
	}
	
	if(access(argv[1], R_OK)!= 0)
	{
		printf("Can't access file \"%s\"!\n", argv[1]);
		exit(2);
	}
	
	int ret = checklic(1, argv[1]);
	printf("checklic ret: %d\n", ret);
	return 0;
}
