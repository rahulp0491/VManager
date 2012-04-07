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
#include <unistd.h>
#include "definitions.h"

int globalConHandler;

char inputOptions[][NumOfInputOptions]={"connect", "close", "dumpxml", "createdom", "suspend", "resume", "save", "restore", "shutdown", "reboot", "dominfo", "numdomain", "nodeinfo", "nodelist", "nodecap", "load", "domlist"};

struct connThreadStruct {
	pthread_t domainThread [MaxNumDomains];
}cThread[MaxNumConnections];

struct domStruct {
	virDomainPtr dom;
	int iscreated;
};

struct connStruct {
	virConnectPtr conn;
	int isconnected;
	struct domStruct domain [MaxNumDomains];
	int numGuestDomains;
}connection[MaxNumConnections];

void printNodeList () {
	int i;
	for (i=0; i < MaxNumConnections; i++) {
		if (connection[i].isconnected != 0) {
			fprintf (stdout, "%d\t%s\n",i+1, virConnectGetHostname (connection[i].conn));
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
		if (connection[conNum].domain[i].dom == NULL) {
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
	char line[100];
	char configFileName[MaxFileName], xmlConfig [MaxConfigSize];
	printf ("XML config file name: ");
	scanf ("%s", configFileName);
	FILE *fp;
	fp = fopen (configFileName, "r");
	while (fgets(line, 100, fp) != NULL) {
		strcat (xmlConfig, line);
	}
	connection[conNum].domain[domainNum].dom = virDomainDefineXML (connection[conNum].conn, xmlConfig);
	virConnectRef (connection[conNum].conn);
	connection[conNum].domain[domainNum].iscreated = 1;
	if (!connection[conNum].domain[domainNum].dom) {
		fprintf(stderr, "Error: Domain definition failed\n\n");
		return NULL;
	}
	if (virDomainCreate(connection[conNum].domain[domainNum].dom) < 0) {
		virDomainFree(connection[conNum].domain[domainNum].dom);
		fprintf(stderr, "Error: Cannot boot guest\n\n");
		return NULL;
	}
	fprintf(stdout, "Guest has booted\n\n");
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
	char uri[50];
	fprintf (stdout, "Enter URI: ");
	scanf ("%s", uri);
	if (connectionWithSameURI (uri) != -1) {
		fprintf (stderr, "Error: Connection to %s already exists\n\n", uri);
		return -1;
	}
	connection[conNum].conn = virConnectOpen (uri);
	if (connection[conNum].conn == NULL) {
		fprintf (stderr, "Error: Failed to open connection to %s\n\n", uri);
		return -1;
	}
	connection[conNum].isconnected = 1;
	fprintf (stdout, "Connection to %s established\n\n", uri);
	return 0;
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

int connectionWithSameURI (char *uri) {
	int i;
	for (i=0; i < MaxNumConnections; i++) {
		if (connection[i].isconnected == 1)
			if (!strcmp(virConnectGetURI(connection[i].conn), uri)) {
				return i;
			}
	}
	return -1;
}

void printDomInfo (virDomainInfoPtr dominfo) {
	fprintf (stdout, "Domain State: %c\n", dominfo->state);
}

int isDomCreated (char *domName, int conNum) {
	int i;
	for (i=0; i < MaxNumDomains; i++) {
		if (connection[conNum].domain[i].iscreated != 0) {
			if (!strcmp (virDomainGetName (connection[conNum].domain[i].dom), domName)) {
				return i;
			}
		}
	}
	return -1;
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
				fprintf (stderr, "Error: Invalid connection to %s\n\n", line);
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
				fprintf (stderr, "Error: Invalid connection to %s\n\n", hostname);
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
				fprintf (stderr, "Error: Invalid connection to %s\n\n", hostname);
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
				fprintf (stderr, "Error: Invalid connection to %s\n\n", hostname);
				return -1;
			}
// 			virDomainPtr dom;
			isret = isDomCreated (domName, conNum);
			if (isret < 0) {
				fprintf (stderr, "Error: Domain does not exists\n\n");
				return -1;
			}
			connection[conNum].domain[isret].iscreated = 0;
// 			dom = virDomainLookupByName (connection[conNum].conn, domName);
			isret = virDomainShutdown(connection[conNum].domain[isret].dom);
			virConnectClose (connection[conNum].conn);
			if (isret != 0) {
				fprintf (stderr, "Error: Cannot shutdown domain object\n\n");
				return -1;
			}
			fprintf (stdout, "Domain %s shutdown on %s\n\n", domName, hostname);
			if (virDomainFree (connection[conNum].domain[isret].dom) != 0) {
				fprintf (stderr, "Error: Cannot free domain datastructure\n\n");
				return -1;
			}
			fprintf (stdout, "All datastructure related to domain %s freed on %s\n\n", domName, hostname);
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
				fprintf (stderr, "Error: Invalid connection to %s\n\n", hostname);
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
				fprintf (stderr, "Error: Invalid connection to %s\n\n", hostname);
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
				fprintf (stderr, "Error: Invalid connection to %s\n\n", name);
				return -1;
			}
			fprintf (stdout, "Enter Domain name: ");
			scanf ("%s", name);
			virDomainPtr dom;
			dom = virDomainLookupByName (connection[conNum].conn, name);
			if (dom == NULL) {
				fprintf (stdout, "Error: Invalid domain %s\n\n", name);
				return -1;
			}
 			xmldump = virDomainGetXMLDesc (dom, VIR_DOMAIN_XML_SECURE | VIR_DOMAIN_XML_INACTIVE | VIR_DOMAIN_XML_UPDATE_CPU);
			fprintf (stdout, "%s\n\n", xmldump);
		}
		break;
		
		case REBOOT: {
			int isret, conNum;
			char name[50];
			fprintf (stdout, "Enter hostname: ");
			scanf ("%s", name);
			isret = isConnectionEstablished (name);
			if (isret >= 0) {
				conNum = isret;
			}
			else {
				fprintf (stderr, "Error: Invalid connection to %s\n\n", name);
				return -1;
			}
			fprintf (stdout, "Enter Domain name: ");
			scanf ("%s", name);
			virDomainPtr dom;
			dom = virDomainLookupByName (connection[conNum].conn, name);
			if (dom == NULL) {
				fprintf (stderr, "Error: Invalid domain %s\n\n", name);
				return -1;
			}
			
			isret = virDomainReboot (dom, VIR_DOMAIN_REBOOT_DEFAULT);
			if (isret < 0) {
				fprintf (stderr, "Error: Cannot boot domain %s\n\n", name);
				return -1;
			}
			fprintf (stdout, "Guest domain %s ready for reboot\n\n", name);
		}
		break;
		
		case SAVE: {
			int isret, conNum;
			char name[50];
			fprintf (stdout, "Enter hostname: ");
			scanf ("%s", name);
			isret = isConnectionEstablished (name);
			if (isret >= 0) {
				conNum = isret;
			}
			else {
				fprintf (stderr, "Error: Invalid connection to %s\n\n", name);
				return -1;
			}
			fprintf (stdout, "Enter Domain name: ");
			scanf ("%s", name);
			virDomainPtr dom;
			dom = virDomainLookupByName (connection[conNum].conn, name);
			if (dom == NULL) {
				fprintf (stderr, "Error: Invalid domain %s\n\n", name);
				return -1;
			}
			fprintf (stdout, "Enter filename to be saved as: ");
			scanf ("%s", name);
			isret = virDomainSave (dom, name);
			if (isret < 0) {
				fprintf (stderr, "Error: Cannot save domain memory\n\n");
				return -1;
			}
			fprintf (stdout, "Domain memory saved\n\n");
		}
		break;
		
		case RESTORE: {
			int isret, conNum;
			char name[50];
			fprintf (stdout, "Enter hostname: ");
			scanf ("%s", name);
			isret = isConnectionEstablished (name);
			if (isret >= 0) {
				conNum = isret;
			}
			else {
				fprintf (stderr, "Error: Invalid connection to %s\n\n", name);
				return -1;
			}
			fprintf (stdout, "Enter filename for restoring: ");
			scanf ("%s", name);
			isret = virDomainRestore (connection[conNum].conn, name);
			if (isret < 0) {
				fprintf (stderr, "Error: Cannot restore domain memory\n\n");
				return -1;
			}
			fprintf (stdout, "Domain memory restored from %s to %s\n\n", name, virConnectGetHostname (connection[conNum].conn));
		}
		break;
		
		case SUSPEND: {
			int isret, conNum;
			char name[50];
			fprintf (stdout, "Enter hostname: ");
			scanf ("%s", name);
			isret = isConnectionEstablished (name);
			if (isret >= 0) {
				conNum = isret;
			}
			else {
				fprintf (stderr, "Error: Invalid connection to %s\n\n", name);
				return -1;
			}
			fprintf (stdout, "Enter Domain name: ");
			scanf ("%s", name);
			virDomainPtr dom;
			dom = virDomainLookupByName (connection[conNum].conn, name);
			if (dom == NULL) {
				fprintf (stderr, "Error: Invalid domain %s\n\n", name);
				return -1;
			}
			isret = virDomainSuspend (dom);
			if (isret != 0) {
				fprintf (stderr, "Error: Domain %s cannot be suspended, check privileges\n\n", name);
				return -1;
			}
			fprintf (stdout, "Domain %s suspended. Memory still allocated\n\n", name);
		}
		break;
		
		case RESUME: {
			int isret, conNum;
			char name[50];
			fprintf (stdout, "Enter hostname: ");
			scanf ("%s", name);
			isret = isConnectionEstablished (name);
			if (isret >= 0) {
				conNum = isret;
			}
			else {
				fprintf (stderr, "Error: Invalid connection to %s\n\n", name);
				return -1;
			}
			fprintf (stdout, "Enter Domain name: ");
			scanf ("%s", name);
			virDomainPtr dom;
			dom = virDomainLookupByName (connection[conNum].conn, name);
			if (dom == NULL) {
				fprintf (stderr, "Error: Invalid domain %s\n\n", name);
				return -1;
			}
			isret = virDomainResume (dom);
			if (isret != 0) {
				fprintf (stderr, "Error: Domain %s cannot be restored\n\n", name);
				return -1;
			}
			fprintf (stdout, "Domain %s restored. Hypervisor resources available\n\n", name);
		}
		break;
		
		case LOAD: {
			int isret, conNum;
			char name[50], *net[30]; 
			int size = 30;
			fprintf (stdout, "Enter hostname: ");
			scanf ("%s", name);
			isret = isConnectionEstablished (name);
			if (isret >= 0) {
				conNum = isret;
			}
			else {
				fprintf (stderr, "Error: Invalid connection to %s\n\n", name);
				return -1;
			}
			isret = virConnectListNetworks (connection[conNum].conn, net, size);
			fprintf (stdout, "Number of networks in %s: %d\n\n",name, isret);
		}
		break;
		
		case DOMINFO: {
			int isret, conNum;
			char name[50]; 
			fprintf (stdout, "Enter hostname: ");
			scanf ("%s", name);
			isret = isConnectionEstablished (name);
			if (isret >= 0) {
				conNum = isret;
			}
			else {
				fprintf (stderr, "Error: Invalid connection to %s\n\n", name);
				return -1;
			}
			fprintf (stdout, "Enter Domain name: ");
			scanf ("%s", name);
			virDomainPtr dom;
			dom = virDomainLookupByName (connection[conNum].conn, name);
			if (dom == NULL) {
				fprintf (stderr, "Error: Invalid domain %s\n\n", name);
				return -1;
			}
			virDomainInfoPtr dominfo;
			dominfo = malloc (sizeof (virDomainInfoPtr));
			isret = virDomainGetInfo (dom, dominfo);
			if (isret != 0) {
				fprintf (stderr, "Error: Cannot get info about domain %s\n\n", name);
				return -1;
			}
			printDomInfo (dominfo);
		}
		break;
		
		default:
			fprintf (stderr, "Error: Invalid input\n\n");
		break;
	}
	return 0;
}
