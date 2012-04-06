#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <libvirt/libvirt.h>		// INCLUDING THE LIBVIRT LIBRARY
#include <pthread.h>
#include <assert.h>
#include <string.h>
#include "definitions.h"

struct connThreadStruct {
	pthread_t connThread;
	pthread_t domainThread [MaxNumDomains];
}cThread[MaxNumConnections];

struct connStruct {
	virConnectPtr conn;
	virDomainPtr dom [MaxNumDomains];
	int numGuestDomains;
}connection[MaxNumConnections];


int createDomain (int conNum) {
	int rc;
	int domainNum;
	domainNum = getNextDomainThreadNum (conNum);
	rc = pthread_create (&cThread[conNum].domainThread[domainNum], NULL, manageDomain, (void *)domainNum);
	assert (rc == 0);
	rc = pthread_join (cThread[conNum].domainThread[domainNum], NULL);
	assert (rc == 0);
	return 0;
}

int getNextDomain (int conNum) {
	return virConnectNumOfDomains(connection[conNum].conn)+1;
}

int getNextDomainThreadNum (int conNum) {
	int i;
	for (i=0; i < MaxNumDomains; i++) {
		if (connection[conNum].dom [i] == NULL) {
			return i;
		}
	}
	if (i == MaxNumDomains) {
		return -1;
	}
}

int getNextConnThread () {
	int i;
	for (i=0; i < MaxNumConnections; i++) {
		if (connection[i].conn == NULL) {
			return i;
		}
	}
	if (i == MaxNumConnections) {
		return -1;
	}
}

int isConnectionEstablished (char *hostname) {
	int i;
	for (i=0; i < MaxNumConnections; i++) {
		if (connection[i].conn != NULL) {
			printf ("host: %d\t%s\n",i, virConnectGetHostname (connection[i].conn));
			if (!strcmp(virConnectGetHostname (connection[i].conn), hostname)) {
				return i;
			}
		}
	}
	if (i == MaxNumConnections) {
		return -1;
	}
}

void *manageDomain (void *arg) {
	int domainNum, conNum, isret;
	
	domainNum = (int)arg;
	printf ("Domain num: %d\n\n", domainNum);
	char line[100];
	char configFileName[MaxFileName], xmlConfig [MaxConfigSize];
	printf ("Specify XML config file name: ");
	scanf ("%s", configFileName);
	FILE *fp;
	fp = fopen (configFileName, "r");
	while (fgets(line, 100, fp) != NULL) {
		strcat (xmlConfig, line);
	}
// 	printf ("\n%s\n", xmlConfig);
	printf ("Enter hostname: ");
	scanf ("%s", line);
	isret = isConnectionEstablished (line);
	if (isret < 0) {
		conNum = getNextConnThread();
		createConnection (conNum);
	}
	else {
		conNum = isret;
	}
	connection[conNum].dom[domainNum] = virDomainDefineXML (connection[conNum].conn, xmlConfig);
	if (!connection[conNum].dom[domainNum]) {
		fprintf(stderr, "Domain definition failed\n\n");
		return NULL;
	}
	if (virDomainCreate(connection[conNum].dom[domainNum]) < 0) {
		virDomainFree(connection[conNum].dom[domainNum]);
		fprintf(stderr, "Cannot boot guest\n\n");
		return NULL;
	}
	fprintf(stderr, "Guest has booted\n\n");
	virDomainFree(connection[conNum].dom[domainNum]);

	return NULL;
}

int assignNum (char *input) {
	if (!strcmp (input, "connect")) 
		return CONNECT;
	else if (!strcmp (input, "close"))
		return CLOSECON;
	else if (!strcmp (input, "dumpxml"))
		return DUMPXML;
	else if (!strcmp (input, "createdomain"))
		return CREATEDOM;
	else if (!strcmp (input, "suspend"))
		return SUSPEND;
	else if (!strcmp (input, "resume"))
		return RESUME;
	else if (!strcmp (input, "save"))
		return SAVE;
	else if (!strcmp (input, "restore"))
		return RESTORE;
	else if (!strcmp (input, "shutdown"))
		return SHUTDOWN;
	else if (!strcmp (input, "reboot"))
		return REBOOT;
	else if (!strcmp (input, "destroy"))
		return DESTROY;
	else if (!strcmp (input, "numofdomains"))
		return NUMDOMAIN;
}

int createConnection (int conNum) {
	int rc;
	rc = pthread_create (&cThread[conNum].connThread, NULL, manageConnections, (void *)conNum);
	printf ("conThread: %d\n", conNum);
	assert (rc == 0);
	rc = pthread_join (cThread[conNum].connThread, NULL);
	assert (rc == 0);
	return 0;
}

void *manageConnections (void *arg) {
	int conNum;
	char uri[50];
	printf ("Enter URI: ");
	scanf ("%s", uri);
	conNum = (int)arg;
	connection[conNum].conn = virConnectOpen (uri);
	if (connection[conNum].conn == NULL) {
		fprintf (stderr, "Failed to open connection to %s\n\n", uri);
		return NULL;
	}
	printf ("Connection to %s established\n\n", uri);
}

int handleInput (int input) {
	switch (input) {
		case CONNECT: {
			int conNum = getNextConnThread ();
			createConnection (conNum);
		}
		break;
		
		case CLOSECON: {
			int isret, conNum;
			char line [50];
			printf ("Enter hostname: ");
			scanf ("%s", line);
			isret = isConnectionEstablished (line);
			if (isret >= 0) {
				conNum = isret;
				strcpy (line, virConnectGetHostname (connection[conNum].conn));
			}
			else {
				fprintf (stderr, "Invalid connection\n");
				return -1;
			}
			if (virConnectClose (connection[conNum].conn) < 0) {
				fprintf (stderr, "Failed to close connection to %s\n", line);
				return -1;
			}
			printf ("Connection to %s closed\n\n", line);
		}
		break;
		
		case NUMDOMAIN: {
			int numDomains, isret, conNum;
			char hostname [50];
			printf ("Enter hostname: ");
			scanf ("%s", hostname);
			isret = isConnectionEstablished (hostname);
			if (isret >= 0) {
				conNum = isret;
				strcpy (hostname, virConnectGetHostname (connection[conNum].conn));
			}
			else {
				fprintf (stderr, "Invalid connection\n");
				return -1;
			}
			numDomains = virConnectNumOfDomains (connection[conNum].conn);
			printf ("Total number of domains in the host %s: %d\n\n",hostname, numDomains);
		}
		break;
		
		case CREATEDOM: {
			int isret, conNum;
			char hostname [50];
			printf ("Enter hostname: ");
			scanf ("%s", hostname);
			isret = isConnectionEstablished (hostname);
			if (isret >= 0) {
				conNum = isret;
// 				strcpy (hostname, virConnectGetHostname (connection[conNum].conn));
			}
			else {
				fprintf (stderr, "Invalid connection\n");
				return -1;
			}
			createDomain (conNum);
		}
		break;
		
		case SHUTDOWN: {
			printf ("Enter domain name: ");
			char domName [20];
			int ret, conNum;
			scanf ("%s", domName);
			virDomainPtr dom;
			dom = virDomainLookupByName (connection[conNum].conn, domName);
			ret = virDomainShutdown(dom);
			if (ret != 0) {
				fprintf (stderr, "Error: Cannot shutdown domain object\n\n");
				return -1;
			}
			if (virDomainFree (dom) != 0){
				fprintf (stderr, "Error: Cannot free domain datastructure\n\n");
				return -1;
			}
		}
		break;
		
		default:
			fprintf (stderr, "Error: Invalid input\n\n");
		break;
	}
}
