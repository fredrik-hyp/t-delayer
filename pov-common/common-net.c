#include "common-net.h"

// returns the bits in a netmask byte
// returns error if additional set bits are found after 0
int maskbitcount(uint8_t mask)
{
	int res = 0;
    uint8_t i;

    for(i = 128; i > 0; i = i >> 1 )
    {
		if(i != (mask & i))
        {
        	i = i >> 1;
            break;
        }
        res++;
    }

    if(i > 0)
    {
    	for(i=i ; i > 0; i = i >> 1)
        {
        	if(i == (mask & i))
            {
                return -1;
            }
		}
	}
	return res;
}

int cidr2str(int cidr, char *res, int len)
{
	uint32_t ipv4Netmask;
	
	if(cidr < 1 || cidr > 32 )
	{
		return 1;
	}
	
	memset(res,0,len);
	ipv4Netmask = 0xffffffff;
	ipv4Netmask <<= 32 - cidr;
	snprintf(res, len, "%d.%d.%d.%d",
			((ipv4Netmask & 0xFF000000) >> 24),
			((ipv4Netmask & 0x00FF0000) >> 16),
			((ipv4Netmask & 0x0000FF00) >> 8),
			((ipv4Netmask & 0x000000FF)));
	
	return 0;
}

// returns 0 on success
int str2cidr(char *str, int *cidr)
{
	int i = 0;
	char *mask;
	int mask_i;
	char *posstr;
	int res;

	*cidr = 0;
	while( (mask = strtok_r(str, ".", &posstr)) != NULL )
	{
		if(i > 4)
		{
			return 2; // invalid input mask
		}
	 	mask_i = atoi(mask);
		
		if(mask_i > 255 || mask_i < 0)
		{
		  return 2;
	  	}
		
		res = maskbitcount(mask_i);
		if(res < 0)
		{
			return 3;
		}
		
		*cidr += res;
	  	i++;
	}
	return 0;
}

int saveDNStoDB(PGconn **dbconn, char *dns)
{
	int ret;
	dbnetservice netservice;
	
	memset(&netservice, 0, sizeof(dbnetservice));

	ret = dbdeletenetservicesbytype(dbconn, "dns");
	if(ret != 0)
	{
		return 1;
	}
	
	if( strlen(dns) > 0 )
	{
		snprintf(netservice.address, DB_MAX_HOSTNAME_LENGTH, "%s", dns);
		ret = dbaddnetservice(dbconn, &netservice);
		if(ret != 0)
		{
			return 2;
		}
	}
	return 0;
}

int constructInterfacesFileFromDB(PGconn **dbconn, char *resultstr, uint64_t len)
{
//	return 1;
	int res, i,j, k, cidr;
	dbnetnic netnics[16];
	uint64_t nrofnetnics;
	dbnetvlan *netvlans;
	uint64_t nrofnetvlans;
	dbnetroute netroutes[2000];
	uint64_t nrofnetroutes;
	dbnetservice netservice;
	uint64_t nrofnetservices;
	char *routestr;
	int routestrlen=10000;
	char tmpstr[10000];
	int tmpstrlen=10000;
	
	// add loopback	*/
	res = snprintf(resultstr, 8192, "source /etc/network/interfaces.d/*\n\nauto lo\niface lo inet loopback\n\n");
	memset(netnics, 0, sizeof(dbnetnic)*16);
	memset(&netservice, 0, sizeof(dbnetservice));
	

	
	// add enabled nics
	res = dbfetchnetnics(dbconn, netnics, 16, &nrofnetnics);
	if(res != 0)
	{
		return 1;
	}
	
	routestr = malloc(sizeof(char) * routestrlen);
	memset(routestr, 0, sizeof(char) * routestrlen);
	for(i = 0 ; i < 16; i++)
	{
		if(netnics[i].inuse)
		{
		
			snprintf(tmpstr, tmpstrlen, "%sallow-hotplug %s\n", resultstr, netnics[i].physicalname);
			snprintf(resultstr, len, "%s", tmpstr);
			if(netnics[i].dhcp)
			{
				snprintf(tmpstr, tmpstrlen, "%siface %s inet dhcp", resultstr, netnics[i].physicalname);
				snprintf(resultstr, len,"%s", tmpstr);
			}
			else
			{
				snprintf(tmpstr, tmpstrlen, "%siface %s inet static\n\taddress %s\n\tnetmask %s\n\n",
									resultstr, netnics[i].physicalname, netnics[i].address, netnics[i].netmask);
				snprintf(resultstr, len, "%s", tmpstr);
			}
				
			// fetch the routes for this interface
			res = dbfetchnetroutesbynetnicid(dbconn, netnics[i].id , netroutes, 2000 , &nrofnetroutes);
			if(res == 0 )
			{
				for(j=0; j<nrofnetroutes; j++)
				{
					printf("netmask: %s\n", routestr);
					res = str2cidr(netroutes[i].netmask, &cidr);
					if(res != 0)
					{
						free(routestr);
						return 2;
					}
					res = snprintf(routestr, routestrlen, "%sip route add %s/%d dev %s#%s\n", routestr, netroutes[j].network, res,  netnics[i].physicalname, netroutes[j].description);
					if(res >= routestrlen)
					{
						free(routestr);
						return 3;
					}
				}
			}
			else
			{
				syslog(LOG_WARNING, "Failed to fetch netroute for netnics.");
			}
			
		}
		
		// fetch vlans attached to netnics
		netvlans = malloc(sizeof(dbnetvlan)*4096);
		memset(netvlans, 0, sizeof(dbnetvlan)*4096);
		res = dbfetchnetvlansbynetnicid(dbconn, netnics[i].id, netvlans, 4096, &nrofnetvlans);
		if(res != 0)
		{
			syslog(LOG_WARNING, "Failed to fetch netvlans for netnic.");
			free(netvlans);
			continue;		
		}
		
		for(j = 0; j < nrofnetvlans; j++)
		{
			if(!netvlans[j].inuse)
			{
				continue;
			}
			
			snprintf(resultstr, len, "%sallow-hotplug %s.%d\n", resultstr, netnics[i].physicalname, netvlans[j].vlanid);
		
			if(netvlans[j].dhcp)
			{
				snprintf(resultstr, len, "%siface %s.%d inet dhcp", resultstr, netnics[i].physicalname, netvlans[j].vlanid);
			}
			else
			{
				snprintf(resultstr, len, "%siface %s.%d inet static\n\taddress %s\n\tnetmask %s\n\n",
									resultstr, netnics[i].physicalname, netvlans[j].vlanid, netvlans[j].address, netvlans[j].netmask);
			}
		}
		// fetch the routes for this interface
		res = dbfetchnetroutesbynetnicidandvlanid(dbconn, netnics[i].id , netvlans[j].id, netroutes, 2000 , &nrofnetroutes);
		if(res == 0 )
		{
			for(k=0; k<nrofnetroutes; k++)
			{
				printf("netmask: %s\n", netroutes[k].netmask);
				res = str2cidr(netroutes[k].netmask, &cidr);
				if(res != 0)
				{
					free(netvlans);
					free(routestr);
					return 4;
				}
				res = snprintf(routestr, 10000, "%sip route add %s/%d dev %s.%d #%s\n",
												routestr, netroutes[k].network, res,
												netnics[i].physicalname, netvlans[j].vlanid, netroutes[k].description);
				if(res >= 10000)
				{
					free(netvlans);
					free(routestr);
					return 5;
				}
			}
		}
		else
		{
			syslog(LOG_WARNING, "Failed to fetch netroute for netnics.");
		}
		
		free(netvlans);
		
	}
	
	
	// add default gateway
	// ip route add default via "11.1.1.1
	res = dbfetchnetservicesbytype(dbconn, "defgw", &netservice, 1, &nrofnetservices);
	if( res != 0)
	{
		return 6;
	}
		
	if(netservice.id)
	{
		res = snprintf(routestr, 10000, "up ip route add default via %s\n%s", netservice.address, routestr);
		if(res >= 10000)
		{
			free(routestr);
			return 7;
		} 
	}
		
	// add static routes including multicast route
	printf("resultstr: %s\n",resultstr);
	res = snprintf(tmpstr, tmpstrlen, "%s\n%s", resultstr, routestr);
	snprintf(resultstr, len, "%s", tmpstr);
	printf("resultstr: %s\n",resultstr);
	if(res >= len)
	{
		free(routestr);
		return 8;
	}
	free(routestr);
	return 0;
}