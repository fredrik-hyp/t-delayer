#include "thread_traps.h"
#include "common.h"
#include "common-db.h"
#include "states.h"

int mallocTrap;
int freeTrap;

extern s_interface streaminterface;
// inits a root element that not holds any data just a pointer next to the first dataelement
void init_trap_queue()
{
	mallocTrap = 0;
	freeTrap = 0;
	pthread_mutex_lock(&mutex_traps);
	trapqueue = malloc(sizeof(trapelement));
	mallocTrap++;
	trapqueue->next = NULL;
	pthread_mutex_unlock(&mutex_traps);
}

// removes everytingh in the queue including the above root element
void deinit_trap_queue()
{
	trapelement *current, *next;

	if (trapqueue == NULL)
	{
		return;
	}

	pthread_mutex_lock(&mutex_traps);
	current = trapqueue;
	while (current != NULL)
	{
		next = current->next;
		free(current);
		freeTrap++;
		current = next;
	}
	trapqueue = NULL;
	pthread_mutex_unlock(&mutex_traps);
	LOGMSG(LOG_WARNING, "%s: mallocTrap %d freeTrap %d",
		   streaminterface.name, mallocTrap, freeTrap);
}

// adds a trapelement last to the queue
void add_trap_to_queue(int i_oid, char *msg)
{
	trapelement *current, *newtrap;

	if (trapqueue == NULL)
	{
		return;
	}

	pthread_mutex_lock(&mutex_traps);
	current = trapqueue;
	while (current->next != NULL)
	{
		current = current -> next;
	}
	newtrap = malloc(sizeof(trapelement));
	mallocTrap++;
	newtrap->i_oid = i_oid;
	strncpy(newtrap->msg, msg, TRAP_MSG_LENGTH);
	newtrap->next = NULL;
	current->next = newtrap;
	pthread_mutex_unlock(&mutex_traps);
}

// returns the first trap element and removes it from the queue
trapelement *get_trap_from_queue()
{
	trapelement *result;

	pthread_mutex_lock(&mutex_traps);
	if (trapqueue->next == NULL)
	{
		pthread_mutex_unlock(&mutex_traps);
		return NULL;
	}


	result = trapqueue->next;
	trapqueue->next = result->next;
	pthread_mutex_unlock(&mutex_traps);

	result->next = NULL;
	return result;
}

int trap_to_blacklabel(int trap, char *oid, int oidlen)
{
	oid[0] = '\0';
	switch (trap)
	{
	case TRAP_TRANSMITTER_START:
		strncpy(oid, BLACKLABEL_TRAP_TRANSMITTER_START, oidlen);
		break;
	case TRAP_TRANSMITTER_STOP:
		strncpy(oid, BLACKLABEL_TRAP_TRANSMITTER_STOP, oidlen);
		break;
	case TRAP_TRANSMITTER_FAILED_DNSLOOKUP:
		strncpy(oid, BLACKLABEL_TRAP_TRANSMITTER_FAILED_DNSLOOKUP, oidlen);
		break;
	case TRAP_TRANSMITTER_FAILED_SETUP:
		strncpy(oid, BLACKLABEL_TRAP_TRANSMITTER_FAILED_SETUP, oidlen);
		break;
	case TRAP_TRANSMITTER_FAILED_TO_CONNECT:
		strncpy(oid, BLACKLABEL_TRAP_TRANSMITTER_FAILED_TO_CONNECT, oidlen);
		break;
	case TRAP_TRANSMITTER_LOST_OUTPUT:
		strncpy(oid, BLACKLABEL_TRAP_TRANSMITTER_LOST_OUTPUT, oidlen);
		break;
	case TRAP_TRANSMITTER_FAULTY_INPUT:
		strncpy(oid, BLACKLABEL_TRAP_TRANSMITTER_FAULTY_INPUT, oidlen);
		break;
	case TRAP_TRANSMITTER_LOST_INPUT:
		strncpy(oid, BLACKLABEL_TRAP_TRANSMITTER_LOST_INPUT, oidlen);
		break;
	case TRAP_TRANSMITTER_RESTORED_INPUT:
		strncpy(oid, BLACKLABEL_TRAP_TRANSMITTER_RESTORED_INPUT, oidlen);
		break;

	case TRAP_RECEIVER_START:
		strncpy(oid, BLACKLABEL_TRAP_RECEIVER_START, oidlen);
		break;
	case TRAP_RECEIVER_STOP:
		strncpy(oid, BLACKLABEL_TRAP_RECEIVER_STOP, oidlen);
		break;
	case TRAP_RECEIVER_FAILED_DNSLOOKUP:
		strncpy(oid, BLACKLABEL_TRAP_RECEIVER_FAILED_DNSLOOKUP, oidlen);
		break;
	case TRAP_RECEIVER_FAILED_SETUP:
		strncpy(oid, BLACKLABEL_TRAP_RECEIVER_FAILED_SETUP, oidlen);
		break;
	case TRAP_RECEIVER_FAILED_TO_CONNECT:
		strncpy(oid, BLACKLABEL_TRAP_RECEIVER_FAILED_TO_CONNECT, oidlen);
		break;
	case TRAP_RECEIVER_LOST_OUTPUT:
		strncpy(oid, BLACKLABEL_TRAP_RECEIVER_LOST_INPUT, oidlen);
		break;
	case TRAP_RECEIVER_RESTORED_OUTPUT:
		strncpy(oid, BLACKLABEL_TRAP_RECEIVER_RESTORED_OUTPUT, oidlen);
		break;
	case TRAP_RECEIVER_LOST_INPUT:
		strncpy(oid, BLACKLABEL_TRAP_RECEIVER_LOST_OUTPUT, oidlen);
		break;
	case TRAP_RECEIVER_FAULTY_OUTPUT:
		strncpy(oid, BLACKLABEL_TRAP_RECEIVER_FAULTY_OUTPUT, oidlen);
		break;

	case TRAP_FORWARDER_START:
		strncpy(oid, BLACKLABEL_TRAP_FORWARDER_START, oidlen);
		break;
	case TRAP_FORWARDER_STOP:
		strncpy(oid, BLACKLABEL_TRAP_FORWARDER_STOP, oidlen);
		break;
	case TRAP_FORWARDER_FAILED_SETUP:
		strncpy(oid, BLACKLABEL_TRAP_FORWARDER_FAILED_SETUP, oidlen);
		break;
	case TRAP_FORWARDER_LOST_INPUT:
		strncpy(oid, BLACKLABEL_TRAP_FORWARDER_LOST_INPUT, oidlen);
		break;
	case TRAP_FORWARDER_FAULTY_INPUT:
		strncpy(oid, BLACKLABEL_TRAP_FORWARDER_FAULTY_INPUT, oidlen);
		break;
	case TRAP_FORWARDER_RESTORED_INPUT:
		strncpy(oid, BLACKLABEL_TRAP_FORWARDER_RESTORED_INPUT, oidlen);
		break;
	case TRAP_FORWARDER_LOST_OUTPUT:
		strncpy(oid, BLACKLABEL_TRAP_FORWARDER_LOST_OUTPUT, oidlen);
		break;
	case TRAP_FORWARDER_RESTORED_OUTPUT:
		strncpy(oid, BLACKLABEL_TRAP_FORWARDER_RESTORED_OUTPUT, oidlen);
		break;
		


	case TRAP_DELAYER_START:
		strncpy(oid, BLACKLABEL_TRAP_DELAYER_START, oidlen);
		break;
	case TRAP_DELAYER_STOP:
		strncpy(oid, BLACKLABEL_TRAP_DELAYER_STOP, oidlen);
		break;
	case TRAP_DELAYER_FAILED_SETUP:
		strncpy(oid, BLACKLABEL_TRAP_DELAYER_FAILED_SETUP, oidlen);
		break;
	case TRAP_DELAYER_LOST_INPUT:
		strncpy(oid, BLACKLABEL_TRAP_DELAYER_LOST_INPUT, oidlen);
		break;
	case TRAP_DELAYER_FAULTY_INPUT:
		strncpy(oid, BLACKLABEL_TRAP_DELAYER_FAULTY_INPUT, oidlen);
		break;
	case TRAP_DELAYER_RESTORED_INPUT:
		strncpy(oid, BLACKLABEL_TRAP_DELAYER_RESTORED_INPUT, oidlen);
		break;

	case TRAP_RECORDER_START:
		strncpy(oid, BLACKLABEL_TRAP_RECORDER_START, oidlen);
		break;
	case TRAP_RECORDER_STOP:
		strncpy(oid, BLACKLABEL_TRAP_RECORDER_STOP, oidlen);
		break;
	case TRAP_RECORDER_FAILED_SETUP:
		strncpy(oid, BLACKLABEL_TRAP_RECORDER_FAILED_SETUP, oidlen);
		break;
	case TRAP_RECORDER_LOST_INPUT:
		strncpy(oid, BLACKLABEL_TRAP_RECORDER_LOST_INPUT, oidlen);
		break;
	case TRAP_RECORDER_FAULTY_INPUT:
		strncpy(oid, BLACKLABEL_TRAP_RECORDER_FAULTY_INPUT, oidlen);
		break;
	case TRAP_RECORDER_RESTORED_INPUT:
		strncpy(oid, BLACKLABEL_TRAP_RECORDER_RESTORED_INPUT, oidlen);
		break;

	case TRAP_PLAYER_START:
		strncpy(oid, BLACKLABEL_TRAP_PLAYER_START, oidlen);
		break;
	case TRAP_PLAYER_STOP:
		strncpy(oid, BLACKLABEL_TRAP_PLAYER_STOP, oidlen);
		break;
	case TRAP_PLAYER_FAILED_SETUP:
		strncpy(oid, BLACKLABEL_TRAP_PLAYER_FAILED_SETUP, oidlen);
		break;
	case TRAP_PLAYER_LOST_INPUT:
		strncpy(oid, BLACKLABEL_TRAP_PLAYER_LOST_INPUT, oidlen);
		break;
	case TRAP_PLAYER_FAULTY_INPUT:
		strncpy(oid, BLACKLABEL_TRAP_PLAYER_FAULTY_INPUT, oidlen);
		break;
	case TRAP_PLAYER_RESTORED_INPUT:
		strncpy(oid, BLACKLABEL_TRAP_PLAYER_RESTORED_INPUT, oidlen);
		break;
	default:
		return 1;
	}
	return 0;

}

int trap_to_bluelabel(int trap, char *oid, int oidlen)
{
	oid[0] = '\0';
	switch (trap)
	{
	case TRAP_TRANSMITTER_START:
		strncpy(oid, BLUELABEL_TRAP_TRANSMITTER_START, oidlen);
		break;
	case TRAP_TRANSMITTER_STOP:
		strncpy(oid, BLUELABEL_TRAP_TRANSMITTER_STOP, oidlen);
		break;
	case TRAP_TRANSMITTER_FAILED_DNSLOOKUP:
		strncpy(oid, BLUELABEL_TRAP_TRANSMITTER_FAILED_DNSLOOKUP, oidlen);
		break;
	case TRAP_TRANSMITTER_FAILED_SETUP:
		strncpy(oid, BLUELABEL_TRAP_TRANSMITTER_FAILED_SETUP, oidlen);
		break;
	case TRAP_TRANSMITTER_FAILED_TO_CONNECT:
		strncpy(oid, BLUELABEL_TRAP_TRANSMITTER_FAILED_TO_CONNECT, oidlen);
		break;
	case TRAP_TRANSMITTER_LOST_OUTPUT:
		strncpy(oid, BLUELABEL_TRAP_TRANSMITTER_LOST_OUTPUT, oidlen);
		break;
	case TRAP_TRANSMITTER_RESTORED_OUTPUT:
		strncpy(oid, BLUELABEL_TRAP_TRANSMITTER_RESTORED_OUTPUT, oidlen);
		break;
	case TRAP_TRANSMITTER_FAULTY_INPUT:
		strncpy(oid, BLUELABEL_TRAP_TRANSMITTER_FAULTY_INPUT, oidlen);
		break;
	case TRAP_TRANSMITTER_LOST_INPUT:
		strncpy(oid, BLUELABEL_TRAP_TRANSMITTER_LOST_INPUT, oidlen);
		break;
	case TRAP_TRANSMITTER_RESTORED_INPUT:
		strncpy(oid, BLUELABEL_TRAP_TRANSMITTER_RESTORED_INPUT, oidlen);
		break;

	case TRAP_RECEIVER_START:
		strncpy(oid, BLUELABEL_TRAP_RECEIVER_START, oidlen);
		break;
	case TRAP_RECEIVER_STOP:
		strncpy(oid, BLUELABEL_TRAP_RECEIVER_STOP, oidlen);
		break;
	case TRAP_RECEIVER_FAILED_DNSLOOKUP:
		strncpy(oid, BLUELABEL_TRAP_RECEIVER_FAILED_DNSLOOKUP, oidlen);
		break;
	case TRAP_RECEIVER_FAILED_SETUP:
		strncpy(oid, BLUELABEL_TRAP_RECEIVER_FAILED_SETUP, oidlen);
		break;
	case TRAP_RECEIVER_FAILED_TO_CONNECT:
		strncpy(oid, BLUELABEL_TRAP_RECEIVER_FAILED_TO_CONNECT, oidlen);
		break;
	case TRAP_RECEIVER_LOST_OUTPUT:
		strncpy(oid, BLUELABEL_TRAP_RECEIVER_LOST_INPUT, oidlen);
		break;
	case TRAP_RECEIVER_RESTORED_OUTPUT:
		strncpy(oid, BLUELABEL_TRAP_RECEIVER_RESTORED_OUTPUT, oidlen);
		break;
	case TRAP_RECEIVER_LOST_INPUT:
		strncpy(oid, BLUELABEL_TRAP_RECEIVER_LOST_OUTPUT, oidlen);
		break;
	case TRAP_RECEIVER_RESTORED_INPUT:
		strncpy(oid, BLUELABEL_TRAP_RECEIVER_RESTORED_INPUT, oidlen);
		break;
	case TRAP_RECEIVER_FAULTY_OUTPUT:
		strncpy(oid, BLUELABEL_TRAP_RECEIVER_FAULTY_OUTPUT, oidlen);
		break;
		
	case TRAP_FORWARDER_START:
		strncpy(oid, BLUELABEL_TRAP_FORWARDER_START, oidlen);
		break;
	case TRAP_FORWARDER_STOP:
		strncpy(oid, BLUELABEL_TRAP_FORWARDER_STOP, oidlen);
		break;
	case TRAP_FORWARDER_FAILED_SETUP:
		strncpy(oid, BLUELABEL_TRAP_FORWARDER_FAILED_SETUP, oidlen);
		break;
	case TRAP_FORWARDER_LOST_INPUT:
		strncpy(oid, BLUELABEL_TRAP_FORWARDER_LOST_INPUT, oidlen);
		break;
	case TRAP_FORWARDER_FAULTY_INPUT:
		strncpy(oid, BLUELABEL_TRAP_FORWARDER_FAULTY_INPUT, oidlen);
		break;
	case TRAP_FORWARDER_RESTORED_INPUT:
		strncpy(oid, BLUELABEL_TRAP_FORWARDER_RESTORED_INPUT, oidlen);
		break;
	case TRAP_FORWARDER_LOST_OUTPUT:
		strncpy(oid, BLUELABEL_TRAP_FORWARDER_LOST_OUTPUT, oidlen);
		break;
	case TRAP_FORWARDER_RESTORED_OUTPUT:
		strncpy(oid, BLUELABEL_TRAP_FORWARDER_RESTORED_OUTPUT, oidlen);
		break;

	case TRAP_DELAYER_START:
		strncpy(oid, BLUELABEL_TRAP_DELAYER_START, oidlen);
		break;
	case TRAP_DELAYER_STOP:
		strncpy(oid, BLUELABEL_TRAP_DELAYER_STOP, oidlen);
		break;
	case TRAP_DELAYER_FAILED_SETUP:
		strncpy(oid, BLUELABEL_TRAP_DELAYER_FAILED_SETUP, oidlen);
		break;
	case TRAP_DELAYER_LOST_INPUT:
		strncpy(oid, BLUELABEL_TRAP_DELAYER_LOST_INPUT, oidlen);
		break;
	case TRAP_DELAYER_FAULTY_INPUT:
		strncpy(oid, BLUELABEL_TRAP_DELAYER_FAULTY_INPUT, oidlen);
		break;
	case TRAP_DELAYER_RESTORED_INPUT:
		strncpy(oid, BLUELABEL_TRAP_DELAYER_RESTORED_INPUT, oidlen);
		break;

	case TRAP_RECORDER_START:
		strncpy(oid, BLUELABEL_TRAP_RECORDER_START, oidlen);
		break;
	case TRAP_RECORDER_STOP:
		strncpy(oid, BLUELABEL_TRAP_RECORDER_STOP, oidlen);
		break;
	case TRAP_RECORDER_FAILED_SETUP:
		strncpy(oid, BLUELABEL_TRAP_RECORDER_FAILED_SETUP, oidlen);
		break;
	case TRAP_RECORDER_LOST_INPUT:
		strncpy(oid, BLUELABEL_TRAP_RECORDER_LOST_INPUT, oidlen);
		break;
	case TRAP_RECORDER_FAULTY_INPUT:
		strncpy(oid, BLUELABEL_TRAP_RECORDER_FAULTY_INPUT, oidlen);
		break;
	case TRAP_RECORDER_RESTORED_INPUT:
		strncpy(oid, BLUELABEL_TRAP_RECORDER_RESTORED_INPUT, oidlen);
		break;

	case TRAP_PLAYER_START:
		strncpy(oid, BLUELABEL_TRAP_PLAYER_START, oidlen);
		break;
	case TRAP_PLAYER_STOP:
		strncpy(oid, BLUELABEL_TRAP_PLAYER_STOP, oidlen);
		break;
	case TRAP_PLAYER_FAILED_SETUP:
		strncpy(oid, BLUELABEL_TRAP_PLAYER_FAILED_SETUP, oidlen);
		break;
	case TRAP_PLAYER_LOST_INPUT:
		strncpy(oid, BLUELABEL_TRAP_PLAYER_LOST_INPUT, oidlen);
		break;
	case TRAP_PLAYER_FAULTY_INPUT:
		strncpy(oid, BLUELABEL_TRAP_PLAYER_FAULTY_INPUT, oidlen);
		break;
	case TRAP_PLAYER_RESTORED_INPUT:
		strncpy(oid, BLUELABEL_TRAP_PLAYER_RESTORED_INPUT, oidlen);
		break;
	default:
		return 1;
	}
	return 0;
}

void *thread_traps(void *arg)
{
	trapelement *trap;
	char oid[OID_LENGTH];
	char cmd[CMD_LENGTH];
	char destination[DESTINATION_LENGTH], newdestination[DESTINATION_LENGTH];
	char community[COMMUNITY_LENGTH], newcommunity[COMMUNITY_LENGTH];
	struct timeval lastconfreadtime, tv;
	int product;
	char trap_stub[TRAP_STUB_LENGTH];

	if (access("/usr/bin/snmptrap", R_OK | X_OK) == -1)
	{
		LOGMSG(LOG_WARNING, "%s: trap program not found, will not send any snmp traps!",
			   streaminterface.name);
		deinit_trap_queue();
		return NULL;
	}

	// Clear buffers
	memset(oid, 0, OID_LENGTH);
	memset(cmd, 0, CMD_LENGTH);
	memset(destination, 0, DESTINATION_LENGTH);
	memset(newdestination, 0, DESTINATION_LENGTH);
	memset(community, 0, COMMUNITY_LENGTH);
	memset(newcommunity, 0, COMMUNITY_LENGTH);
	memset(trap_stub, 0, TRAP_STUB_LENGTH);

	product = get_product();
	if (product == PRODUCT_UNKNOWN)
	{
		LOGMSG(LOG_WARNING, "%s: Could not determine product type, will not send any snmp traps!",
			   streaminterface.name);
		deinit_trap_queue();
		return NULL;
	}
	else if (product == PRODUCT_BLUELABEL)
	{
		snprintf(trap_stub, TRAP_STUB_LENGTH, "%s", BLUELABEL_TRAP_STUB);
	}
	else if (product == PRODUCT_BLACKLABEL)
	{
		snprintf(trap_stub, TRAP_STUB_LENGTH, "%s", BLACKLABEL_TRAP_STUB);
	}

	destination[0] = '\0';
	snprintf(community, COMMUNITY_LENGTH, "%s", "public");

	get_trap_sink(newdestination, 256, newcommunity, 256);
	gettimeofday(&lastconfreadtime, NULL);

	if (newdestination[0] !=  '\0')
	{
		snprintf(destination, DESTINATION_LENGTH, "%s", newdestination);
	}

	if (newcommunity[0] != '\0')
	{
		snprintf(community, COMMUNITY_LENGTH, "%s", newcommunity);
	}

	while (pstate != STATE_SHUTDOWN)
	{
		usleep(500000);
		trap = get_trap_from_queue();
		if (trap != NULL)
		{
			if (destination[0] == '\0')
			{
				LOGMSG(LOG_NOTICE, "%s: No destination given, don't know where to send trap",
					   streaminterface.name);
				free(trap);
				freeTrap++;
				continue;
			}
			// snmptrap -v 2c -c public 192.168.15.26  1.3.6.1.4.1.40562.3.1.4.1  1.3.6.1.4.1.40562.3.1.4.1.1.2.0 s "just here",
			if (product == PRODUCT_BLACKLABEL)
			{
				trap_to_blacklabel(trap->i_oid, oid, 1024);
			}
			else if (product == PRODUCT_BLUELABEL)
			{
				trap_to_bluelabel(trap->i_oid, oid, 1024);
			}
			else
			{
				LOGMSG(LOG_WARNING, "%s: Unknown product!",
					streaminterface.name);
				cmd[0] = '\0';
			}

			if (oid[0] != '\0')
			{
				snprintf(cmd, CMD_LENGTH, "/usr/bin/snmptrap -v 2c -c %s %s \"\" %s %s s \"%s: %s\"",
						 community, destination, trap_stub, oid, streaminterface.name, trap->msg);
#if NODEAMON_DEBUG
				fprintf(stderr, "Executing: %s", cmd);
#endif
				system(cmd);
			}
			else
			{
				LOGMSG(LOG_NOTICE, "%s: Could not extract valid oid!",
					streaminterface.name);
			}
			free(trap);
			freeTrap++;
		}

		gettimeofday(&tv, NULL);
		if (tv.tv_sec - lastconfreadtime.tv_sec  >= 60)
		{
			get_trap_sink(newdestination, 256, newcommunity, 256);
			if (newdestination[0] != '\0')
			{
				snprintf(destination, DESTINATION_LENGTH, "%s", newdestination);
			}
			if (newcommunity[0] != '\0')
			{
				snprintf(community, COMMUNITY_LENGTH, "%s", newcommunity);
			}
			lastconfreadtime = tv;
		}
	}

	LOGMSG(LOG_NOTICE, "%s: EXIT thread_traps",
		streaminterface.name);
	return NULL;
}

void trap_send_exit(int i_oid, const char *msg)
{
	char oid[OID_LENGTH];
	char cmd[CMD_LENGTH];
	int product;
	char destination[DESTINATION_LENGTH], newdestination[DESTINATION_LENGTH];
	char community[COMMUNITY_LENGTH], newcommunity[COMMUNITY_LENGTH];
	char trap_stub[TRAP_STUB_LENGTH];

	if (access("/usr/bin/snmptrap", R_OK | X_OK) == -1)
	{
		LOGMSG(LOG_WARNING, "%s: trap program not found, will not send any snmp trap!",
			   streaminterface.name);
		return;
	}

	// Clear buffers
	memset(oid, 0, OID_LENGTH);
	memset(cmd, 0, CMD_LENGTH);
	memset(destination, 0, DESTINATION_LENGTH);
	memset(newdestination, 0, DESTINATION_LENGTH);
	memset(community, 0, COMMUNITY_LENGTH);
	memset(newcommunity, 0, COMMUNITY_LENGTH);
	memset(trap_stub, 0, TRAP_STUB_LENGTH);

	product = get_product();
	if (product == PRODUCT_UNKNOWN)
	{
		LOGMSG(LOG_WARNING, "%s: Could not determine product type, will not send snmp trap!",
			   streaminterface.name);
		return;
	}
	else if (product == PRODUCT_BLUELABEL)
	{
		snprintf(trap_stub, TRAP_STUB_LENGTH, "%s", BLUELABEL_TRAP_STUB);
		trap_to_bluelabel(i_oid, oid, 1024);
	}
	else if (product == PRODUCT_BLACKLABEL)
	{
		snprintf(trap_stub, TRAP_STUB_LENGTH, "%s", BLACKLABEL_TRAP_STUB);
		trap_to_blacklabel(i_oid, oid, 1024);
	}

	destination[0] = '\0';
	snprintf(community, COMMUNITY_LENGTH, "%s", "public");

	get_trap_sink(newdestination, 256, newcommunity, 256);

	if (newdestination[0] !=  '\0')
	{
		snprintf(destination, DESTINATION_LENGTH, "%s", newdestination);
	}

	if (newcommunity[0] != '\0')
	{
		snprintf(community, COMMUNITY_LENGTH, "%s", newcommunity);
	}

	if (destination[0] == '\0')
	{
		LOGMSG(LOG_NOTICE, "%s: No destination given, don't know where to send trap",
			   streaminterface.name);
		return;
	}

	if (oid[0] != '\0')
	{
		snprintf(cmd, CMD_LENGTH, "/usr/bin/snmptrap -v 2c -c %s %s \"\" %s %s s \"%s: %s\"",
				 community, destination, trap_stub, oid, streaminterface.name, msg);
#if NODEAMON_DEBUG
		LOGMSG(LOG_NOTICE, "Executing: %s",
			cmd);
#endif
		system(cmd);
	}
	else
	{
		LOGMSG(LOG_NOTICE, "%s: Could not extract valid oid!", streaminterface.name);
	}

	return;
}
