#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <libvirt/libvirt.h>		// INCLUDING THE LIBVIRT LIBRARY
#include <libvirt/virterror.h>
#include <pthread.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "definitions.h"

char inputOptions[][NumOfInputOptions] = {"connect", "close", "dumpxml", "createdom", "suspend", "resume", "save", "restore", "shutdown", "reboot", "dominfo", "numdomain", "nodeinfo", "nodelist", "nodecap", "load", "domlist", "destroy", "define", "start", "undefine", "domstate"};

extern virDomainPtr globalDomainHandler;

struct connThreadStruct {
	pthread_t domainThread [MaxNumDomains];
}cThread[MaxNumConnections];

struct domStruct {
	virDomainPtr dom;
	int isdefined;
	int isrunning;
};

struct connStruct {
	virConnectPtr conn;
	int isconnected;
	struct domStruct domain [MaxNumDomains];
	int numGuestDomains;
}connection[MaxNumConnections];


static void vmError (void *userdata, virErrorPtr err) {
	fprintf (stderr, "\n--------------------------------------------------------------------\n");
	fprintf (stderr, "Libvirt error on connection to %s\n", (char *)userdata);
	fprintf (stderr, "--------------------------------------------------------------------\n");
	fprintf (stderr, "Code: %d\n", err->code);
	fprintf (stderr, "Domain: %d\n", err->domain);
	fprintf (stderr, "Message: %s\n", err->message);
	fprintf (stderr, "Level: %d\n", err->level);
}

void printNodeList () {
	int i;
	for (i=0; i < MaxNumConnections; i++) {
		if (connection[i].isconnected != 0) {
			fprintf (stdout, "%s\t\t%s\n",virConnectGetURI (connection[i].conn), virConnectGetHostname (connection[i].conn));
		}
	}
	fprintf (stdout, "\n");
}

void printDomList (int conNum) {
	int i;
	for (i=0; i < MaxNumDomains; i++) {
		if (connection[conNum].domain[i].isdefined != 0 || connection[conNum].domain[i].isrunning != 0) {
			fprintf (stdout, "%d\t%s\n", i+1, virDomainGetName (connection[conNum].domain[i].dom));
		}
	}
	fprintf (stdout, "\n");
}

void createDomain (int conNum, int flag) {
	int rc;
	int domainNum;
	domainNum = getNextDomainThreadNum (conNum);
	globalConHandler = conNum;
	if (flag == CREATE_THREAD)
		rc = pthread_create (&cThread[conNum].domainThread[domainNum], NULL, manageDomain, (void *)domainNum);
	else if (flag == START_THREAD)
		rc = pthread_create (&cThread[conNum].domainThread[domainNum], NULL, startDomain, (void *)globalDomainHandler);
	assert (rc == 0);
	rc = pthread_join (cThread[conNum].domainThread[domainNum], NULL);
	assert (rc == 0);
}

int getNextDomainThreadNum (int conNum) {
	int i;
	for (i=0; i < MaxNumDomains; i++) {
		if (connection[conNum].domain[i].isdefined == 0 && connection[conNum].domain[i].isrunning == 0) {
			return i;
		}
	}
	return -1;
}


int getNextConnThread () {
	int i;
	for (i=0; i < MaxNumConnections; i++) {
		if (connection[i].isconnected == 0) {
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

/*void initializeCon (int conNum) {*/
/*	int domID [10];*/
/*	virDomainlist (connection [conNum].conn, domID);*/
/*	storeDom (conNum, domID);*/
/*}*/

/*void storeDom (int conNum, int *domID) {*/
/*	int i = 0, tempDomNum;*/
/*	int id;*/
/*	virDomainPtr dom;*/
/*	while (i<10) {*/
/*		id = *(domID + i)*/
/*		dom = virDomainLookupByID (id);*/
/*		tempDomNum = getNextDomainThreadNum */
/*		connection [conNum].domain[]*/
/*	}*/
/*}*/

void *manageDomain (void *arg) {
	int domainNum, conNum;
	conNum = globalConHandler;
	domainNum = (int)arg;
	char line[100];
	char configFileName[MaxFileName], xmlConfig [MaxConfigSize];
	bzero (xmlConfig, MaxConfigSize);
	printf ("XML config file name: ");
	scanf ("%s", configFileName);
	FILE *fp;
	fp = fopen (configFileName, "r");
	if (fp == NULL) {
		fprintf(stderr, "Error: Invalid file\n\n");
		return NULL;
	}
	while (fgets(line, 100, fp) != NULL) {
		strcat (xmlConfig, line);
	}
	connection[conNum].domain[domainNum].dom = virDomainCreateXML (connection[conNum].conn, xmlConfig, 0);
	virConnectRef (connection[conNum].conn);
	
	if (connection[conNum].domain[domainNum].dom == NULL) {
		fprintf(stderr, "Error: Domain definition failed\n\n");
		return NULL;
	}
	connection[conNum].domain[domainNum].isrunning = 1;
	fprintf(stdout, "Guest is running\n\n");
	return NULL;
}

void *startDomain (void *arg) {
	virDomainPtr dom;
	dom = (virDomainPtr) globalDomainHandler;
	int isret;
	isret = virDomainCreate (dom);
	if (isret != 0) {
		fprintf(stderr, "Error: Domain cannot be created\n\n");
		return NULL;
	}
	fprintf (stdout, "Domain started\n\n");
	return NULL;
}


int assignNum (char *input) {
	int i=0;
	while(i < NumOfInputOptions)
	 	if (!strcmp(input, inputOptions[i++]))
	 		return i;
	return -1;
} 

void createConnection (int conNum) {
	char uri[50];
	char *hostname;
	fprintf (stdout, "Enter URI: ");
	scanf ("%s", uri);
	if (connectionWithSameURI (uri) != -1) {
		fprintf (stderr, "Error: Connection to %s already exists\n\n", uri);
		return;
	}
	connection[conNum].conn = virConnectOpen (uri);
	if (connection[conNum].conn == NULL) {
		fprintf (stderr, "Error: Failed to open connection to %s\n\n", uri);
		return;
	}
	hostname = virConnectGetHostname (connection[conNum].conn);
	virConnSetErrorFunc(connection[conNum].conn, (void *)hostname , vmError);
	connection[conNum].isconnected = 1;
	fprintf (stdout, "Connection to %s established\n\n", uri);
}

void printNodeInfo (virNodeInfo nodeinfo) {
	fprintf (stdout, "Model			: %s\n", nodeinfo.model);
	fprintf (stdout, "Memory size		: %lu kb\n", nodeinfo.memory);
	fprintf (stdout, "Number of CPUs		: %u\n", nodeinfo.cpus);
	fprintf (stdout, "MHz of CPUs		: %u\n", nodeinfo.mhz);
	fprintf (stdout, "NUMA nodes		: %u\n", nodeinfo.nodes);
	fprintf (stdout, "CPU sockets		: %u\n", nodeinfo.sockets);
	fprintf (stdout, "CPU cores per socket	: %u\n", nodeinfo.cores);
	fprintf (stdout, "CPU threads per socket	: %u\n\n", nodeinfo.threads);
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
	fprintf (stdout, "State			: %d\n", dominfo->state);
	fprintf (stdout, "Memory allowed		: %lu Kbs\n", dominfo->maxMem);
	fprintf (stdout, "Memory used		: %lu Kbs\n", dominfo->memory);
	fprintf (stdout, "Virtual CPUs		: %d\n", dominfo->nrVirtCpu);
	fprintf (stdout, "CPU Time		: %llu ns\n\n", dominfo->cpuTime);
}

int isDomainDefined (char *xml, int conNum) {
	int i;
	for (i=0; i < MaxNumDomains; i++) {
		if (connection[conNum].domain[i].dom != NULL) {
			if (connection[conNum].domain[i].isdefined == 1) {
				return i;
			}
		}
	}
	return -1;
}

void defineDom (int conNum) {
	char configFileName[MaxFileName], xmlConfig [MaxConfigSize];
	fprintf (stdout, "Enter XML config: ");
	scanf ("%s", configFileName);
	int domNum;
	char line [100];
	bzero (xmlConfig, MaxConfigSize);
	FILE *fp;
	fp = fopen (configFileName, "r");
	if (fp == NULL) {
		fprintf(stderr, "Error: Invalid file\n\n");
		return ;
	}
	while (fgets(line, 100, fp) != NULL) {
		strcat (xmlConfig, line);
	}
	if (isDomainDefined (xmlConfig, conNum) != -1) {
		fprintf (stdout, "Domain already defined\n\n");
		return ;
	}
	domNum = getNextDomainThreadNum (conNum);
	connection[conNum].domain[domNum].dom = virDomainDefineXML (connection [conNum].conn, xmlConfig);
	if (connection[conNum].domain[domNum].dom == NULL) {
		fprintf (stderr, "Error: Cannot define domain with given configuration\n\n");
		return ;
	}
	connection[conNum].domain[domNum].isdefined = 1;
	fprintf (stdout, "Domain defined\n\n");
}

void undefineDom (int conNum) {
	char domainName[50];
	fprintf (stdout, "Enter domain name: ");
	scanf ("%s", domainName);
	int domNum, isret;
	domNum = isDomCreated (domainName, conNum);
	if (domNum == -1) {
		fprintf (stderr, "Domain %s does not exists\n\n", domainName);
		return;
	}
	connection[conNum].domain[domNum].isdefined = 0;
	isret = virDomainUndefine (connection[conNum].domain[domNum].dom);
	if (isret < 0) {
		fprintf (stderr, "Cannot undefine domain %s\n\n", domainName);
	}
	virDomainFree (connection[conNum].domain[domNum].dom);
	fprintf (stdout, "Domain %s undefined\n\n", domainName);
}

int isDomCreated (char *domName, int conNum) {
	int i;
	for (i=0; i < MaxNumDomains; i++) {
		if (connection[conNum].domain[i].isdefined != 0 || connection[conNum].domain[i].isrunning != 0) {
			if (!strcmp (virDomainGetName (connection[conNum].domain[i].dom), domName)) {
				return i;
			}
		}
	}
	return -1;
}

char *extractState (int state) {
	switch (state) {
		case 0:
			return "No state";
		break;
		
		case 1:
		return "Running";
		
		case 2:
		return "Blocked";
		
		case 3:
		return "Paused";
		
		case 4:
		return "Shutdown";
		
		case 5:
		return "Shut off";
		
		case 6:
		return "Crashed";
		
		default:
		return "State undefined";
	}
}
