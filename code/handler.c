/*
 * handler.c
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <libvirt/libvirt.h>		// INCLUDING THE LIBVIRT LIBRARY
#include <pthread.h>
#include <assert.h>
#include <string.h>
#include "definitions.h"

int globalConHandler;

char inputOptions[][NumOfInputOptions]={"connect", "close", "dumpxml", "createdom", "suspend", "resume", "save", "restore", "shutdown", "reboot", "destroy", "numdomain", "nodeinfo","nodelist", "nodecap"};

struct connThreadStruct {
	pthread_t connThread;
	pthread_t domainThread [MaxNumDomains];
}cThread[MaxNumConnections];

struct connStruct {
	virConnectPtr conn;
	int isconnected;
	virDomainPtr dom [MaxNumDomains];
	int numGuestDomains;
}connection[MaxNumConnections];


void printNodeList () {
	int i;
	for (i=0; i < MaxNumConnections; i++) {
		if (connection[i].isconnected != 0) {
			fprintf (stdout, "%s\n", virConnectGetHostname (connection[i].conn));
		}
	}
	printf ("\n");
}

int createDomain (int conNum) {
	int rc;
	int domainNum;
	domainNum = getNextDomainThreadNum (conNum);
	globalConHandler = conNum;
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
	return -1;
}

int getNextConnThread () {
	int i;
	for (i=0; i < MaxNumConnections; i++) {
		if (connection[i].conn == NULL) {
			return i;
		}
	}
	return -1;
}

int isConnectionEstablished (char *hostname) {
	int i;
	for (i=0; i < MaxNumConnections; i++) {
		if (connection[i].isconnected == 1) {
			if (!strcmp(virConnectGetHostname (connection[i].conn), hostname)) {
				return i;
			}
		}
	}
	return -1;
}

void *manageDomain (void *arg) {
	int domainNum, conNum;
	conNum = globalConHandler;
	domainNum = (int)arg;
	printf ("Domain num: %d\n\n", domainNum);
	char line[100];
	char configFileName[MaxFileName], xmlConfig [MaxConfigSize];
	printf ("XML config file name: ");
	scanf ("%s", configFileName);
	FILE *fp;
	fp = fopen (configFileName, "r");
	while (fgets(line, 100, fp) != NULL) {
		strcat (xmlConfig, line);
	}
	connection[conNum].dom[domainNum] = virDomainDefineXML (connection[conNum].conn, xmlConfig);
	virConnectRef (connection[conNum].conn);
	if (!connection[conNum].dom[domainNum]) {
		fprintf(stderr, "Error: Domain definition failed\n\n");
		return NULL;
	}
	if (virDomainCreate(connection[conNum].dom[domainNum]) < 0) {
		virDomainFree(connection[conNum].dom[domainNum]);
		fprintf(stderr, "Error: Cannot boot guest\n\n");
		return NULL;
	}
	fprintf(stdout, "Guest has booted\n\n");
	virDomainFree(connection[conNum].dom[domainNum]);

	return NULL;
}

int assignNum (char *input) {
	int i=0;
	while(i < NumOfInputOptions)
	 	if (!strcmp(input, inputOptions[i++]))
	 		return i;
	return -1;
} 

int createConnection (int conNum) {
	int rc;
	rc = pthread_create (&cThread[conNum].connThread, NULL, manageConnections, (void *)conNum);
	assert (rc == 0);
	rc = pthread_join (cThread[conNum].connThread, NULL);
	assert (rc == 0);
	return 0;
}

void *manageConnections (void *arg) {
	int conNum;
	char uri[50];
	fprintf (stdout, "Enter URI: ");
	scanf ("%s", uri);
	conNum = (int)arg;
	connection[conNum].conn = virConnectOpen (uri);
	if (connection[conNum].conn == NULL) {
		fprintf (stderr, "Error: Failed to open connection to %s\n\n", uri);
		return NULL;
	}
	connection[conNum].isconnected = 1;
	fprintf (stdout, "Connection to %s established\n\n", uri);
	return NULL;
}

void printNodeInfo (virNodeInfo nodeinfo) {
	fprintf (stdout, "Model: %s\n", nodeinfo.model);
	fprintf (stdout, "Memory size: %lu kb\n", nodeinfo.memory);
	fprintf (stdout, "Number of CPUs: %u\n", nodeinfo.cpus);
	fprintf (stdout, "MHz of CPUs: %u\n", nodeinfo.mhz);
	fprintf (stdout, "Number of NUMA nodes: %u\n", nodeinfo.nodes);
	fprintf (stdout, "Number of CPU sockets: %u\n", nodeinfo.sockets);
	fprintf (stdout, "Number of CPU cores per socket: %u\n", nodeinfo.cores);
	fprintf (stdout, "Number of CPU threads per socket: %u\n\n", nodeinfo.threads);
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
			fprintf (stdout, "Enter hostname: ");
			scanf ("%s", line);
			isret = isConnectionEstablished (line);
			if (isret >= 0) {
				conNum = isret;
				strcpy (line, virConnectGetHostname (connection[conNum].conn));
			}
			else {
				fprintf (stderr, "Error: Invalid connection\n\n");
				return -1;
			}
			if (virConnectClose (connection[conNum].conn) < 0) {
				fprintf (stderr, "Error: Failed to close connection to %s\n\n", line);
				return -1;
			}
			connection[conNum].isconnected = 0;
			fprintf (stdout, "Connection to %s closed\n\n", line);
		}
		break;
		
		case NUMDOMAIN: {
			int numDomains, isret, conNum;
			char hostname [50];
			fprintf (stdout, "Enter hostname: ");
			scanf ("%s", hostname);
			isret = isConnectionEstablished (hostname);
			if (isret >= 0) {
				conNum = isret;
				strcpy (hostname, virConnectGetHostname (connection[conNum].conn));
			}
			else {
				fprintf (stderr, "Error: Invalid connection\n\n");
				return -1;
			}
			numDomains = virConnectNumOfDomains (connection[conNum].conn);
			fprintf (stdout, "Total number of domains in the host %s: %d\n\n",hostname, numDomains);
		}
		break;
		
		case CREATEDOM: {
			int isret, conNum;
			char hostname [50];
			fprintf (stdout, "Enter hostname: ");
			scanf ("%s", hostname);
			isret = isConnectionEstablished (hostname);
			if (isret >= 0) {
				conNum = isret;
			}
			else {
				fprintf (stderr, "Error: Invalid connection\n\n");
				return -1;
			}
			createDomain (conNum);
		}
		break;
		
		case SHUTDOWN: {
			fprintf (stdout, "Enter domain name: ");
			char domName [20];
			int isret, conNum;
			scanf ("%s", domName);
			char hostname [50];
			fprintf (stdout, "Enter hostname: ");
			scanf ("%s", hostname);
			isret = isConnectionEstablished (hostname);
			if (isret >= 0) {
				conNum = isret;
			}
			else {
				fprintf (stderr, "Error: Invalid connection\n\n");
				return -1;
			}
			virDomainPtr dom;
			dom = virDomainLookupByName (connection[conNum].conn, domName);
			isret = virDomainShutdown(dom);
			virConnectClose (connection[conNum].conn);
			if (isret != 0) {
				fprintf (stderr, "Error: Cannot shutdown domain object\n\n");
				return -1;
			}
			if (virDomainFree (dom) != 0){
				fprintf (stderr, "Error: Cannot free domain datastructure\n\n");
				return -1;
			}
		}
		break;
		
		case NODEINFO: {
			int isret, conNum;
			char hostname [50];
			fprintf (stdout, "Enter hostname: ");
			scanf ("%s", hostname);
			isret = isConnectionEstablished (hostname);
			if (isret >= 0) {
				conNum = isret;
			}
			else {
				fprintf (stderr, "Error: Invalid connection\n\n");
				return -1;
			}
			virNodeInfo nodeinfo;
			isret = virNodeGetInfo(connection[conNum].conn, &nodeinfo);
			if (isret != 0) {
				fprintf (stderr, "Error: Cannot get node information of %s\n\n", hostname);
				return -1;
			}
			printNodeInfo (nodeinfo);
		}
		break;
		
		case NODELIST: {
			printNodeList();
		}
		break;
		
		case NODECAP: {
			int isret, conNum;
			char hostname [50], *caps;
			fprintf (stdout, "Enter hostname: ");
			scanf ("%s", hostname);
			isret = isConnectionEstablished (hostname);
			if (isret >= 0) {
				conNum = isret;
			}
			else {
				fprintf (stderr, "Error: Invalid connection\n\n");
				return -1;
			}
			caps = virConnectGetCapabilities (connection[conNum].conn);
			fprintf (stdout, "%s\n\n", caps);
		}
		break;
		
		case DUMPXML: {
			int isret, conNum;
			char name [50], *xmldump;
			fprintf (stdout, "Enter hostname: ");
			scanf ("%s", name);
			isret = isConnectionEstablished (name);
			if (isret >= 0) {
				conNum = isret;
			}
			else {
				fprintf (stderr, "Error: Invalid connection\n\n");
				return -1;
			}
			fprintf (stdout, "Enter Domain name: ");
			scanf ("%s", name);
			virDomainPtr dom;
			dom = virDomainLookupByName (connection[conNum].conn, name);
			if (dom == NULL) {
				fprintf (stdout, "Error: Invalid domain\n\n");
				return -1;
			}
 			xmldump = virDomainGetXMLDesc (dom, VIR_DOMAIN_XML_SECURE | VIR_DOMAIN_XML_INACTIVE | VIR_DOMAIN_XML_UPDATE_CPU);
			fprintf (stdout, "%s\n\n", xmldump);
		}
		break;
		
		default:
			fprintf (stderr, "Error: Invalid input\n\n");
		break;
	}
	return 0;
}
