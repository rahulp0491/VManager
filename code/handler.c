#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <libvirt/libvirt.h>		// INCLUDING THE LIBVIRT LIBRARY
#include <pthread.h>
#include <assert.h>
#include <string.h>
#include "definitions.h"

virConnectPtr conn;
pthread_t domainThread [MaxNumThreads];
virDomainPtr dom [MaxNumThreads];

int createDomain (int domainNum) {
	int rc;
	rc = pthread_create (&domainThread[domainNum], NULL, manageDomain, (void *)domainNum);
	assert (rc == 0);
	rc = pthread_join (domainThread[domainNum], NULL);
	assert (rc == 0);
	return 0;
}

int getNextDomain () {
	return virConnectNumOfDomains(conn)+1;
}

int getNextThreadNum () {
	int i;
	for (i=0; i < MaxNumThreads; i++) {
		if (dom [i] == NULL) {
			return i;
		}
	}
	if (i == MaxNumThreads) {
		return -1;
	}
}


void *manageDomain (void *arg) {
	int domainNum;
	
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
	dom [domainNum] = virDomainDefineXML (conn, xmlConfig);
	if (!dom [domainNum]) {
		fprintf(stderr, "Domain definition failed\n\n");
		return NULL;
	}
	if (virDomainCreate(dom [domainNum]) < 0) {
		virDomainFree(dom [domainNum]);
		fprintf(stderr, "Cannot boot guest\n\n");
		return NULL;
	}
	fprintf(stderr, "Guest has booted\n\n");
	virDomainFree(dom [domainNum]);

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

int handleInput (int input) {
	switch (input) {
		case CONNECT: {
			char uri[50];
			printf ("Enter URI: ");
			scanf ("%s", uri);
			conn = virConnectOpen ("qemu:///session");
			if (conn == NULL) {
				fprintf (stderr, "Failed to open connection to qemu:///session\n\n");
				return -1;
			}
			printf ("Connection to qemu:///session established\n\n");
		}
		break;
		
		case CLOSECON: {
			if (virConnectClose (conn) < 0) {
				fprintf (stderr, "Failed to close connection to qemu:///session\n");
				return -1;
			}
			printf ("Connection to qemu:///session closed\n\n");
		}
		break;
		
		case NUMDOMAIN: {
			int numDomains;
			numDomains = virConnectNumOfDomains (conn);
			printf ("Total number of domains in the host: %d\n\n", numDomains);
		}
		break;
		
		case CREATEDOM: {
			int nextdomnum;
			nextdomnum = getNextDomain ();
			createDomain (nextdomnum);
		}
		break;
		
		case SHUTDOWN: {
			printf ("Enter domain name: ");
			char domName [20];
			int ret;
			scanf ("%s", domName);
			virDomainPtr dom;
			dom = virDomainLookupByName (conn, domName);
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
