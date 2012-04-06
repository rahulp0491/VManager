#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <libvirt/libvirt.h>			// INCLUDING THE LIBVIRT LIBRARY
#include <pthread.h>
#include <assert.h>
#include <string.h>
#define MaxNumThreads 10
#define MaxConfigSize 10000
#define MaxFileName 100
#define MAxGuestDomains 5

virConnectPtr conn;
pthread_t domainThread [MaxNumThreads];


void *manageDomain (void *arg) {
	int domainNum;
	virDomainPtr dom [MaxNumThreads];
	domainNum = (int)arg;
	printf ("Domain num: %d\n", domainNum);
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
		fprintf(stderr, "Domain definition failed\n");
		return NULL;
	}
	if (virDomainCreate(dom [domainNum]) < 0) {
		virDomainFree(dom [domainNum]);
		fprintf(stderr, "Cannot boot guest\n");
		return NULL;
	}
	fprintf(stderr, "Guest has booted\n");
	virDomainFree(dom [domainNum]);

	return NULL;
}

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


/* -MAIN- */
int main (int argc, char *argv[]) {
	
	conn = virConnectOpen ("qemu:///session");
	if (conn == NULL) {
		fprintf (stderr, "Failed to open connection to qemu:///session\n");
		return -1;
	}
	printf ("Connection to qemu:///session established\n\n");


// 	char *s;
// 	s = virConnectGetCapabilities (conn);
// 	printf ("Capabilities:\n%s\n", s);
	int numDomains;
	numDomains = virConnectNumOfDomains (conn);
	printf ("Num of domains in host: %d\n", numDomains);
	
	char *s;
	s = virConnectGetHostname (conn);
	printf ("Hostname: %s\n", s);
	
	createDomain (getNextDomain());
	if (virConnectClose (conn) != 0) {
		fprintf (stderr, "Cannot close connection to qemu:///session\n");
		return -1;
	}
	
	printf ("Connection to qemu:///session closed\n");
	return 0;
}

