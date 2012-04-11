/*
 * handler.c
*/

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
#include "functions.h"


extern virDomainPtr globalDomainHandler;

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
/*			initializeCon (conNum);*/
			createDomain (conNum, CREATE_THREAD);
		}
		break;
		
		case DEFINE: {
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
			defineDom (conNum);
		}
		break;
		
		case UNDEFINE: {
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
			undefineDom (conNum);
		}
		break;
		
		case DOMSTATE: {
			int isret, conNum;
			char hostname [50];
			char name [50];
			char *state;
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
			fprintf (stdout, "Enter Domain name: ");
			scanf ("%s", name);
			virDomainPtr dom;
			dom = virDomainLookupByName (connection[conNum].conn, name);
			if (dom == NULL) {
				fprintf (stdout, "Error: Invalid domain %s\n\n", name);
				return -1;
			}
			virDomainInfoPtr info;
			info = malloc (sizeof (virDomainInfo));
			isret = virDomainGetInfo (dom, info);
			if (isret != 0) {
				fprintf (stderr, "Cannot extract info from domain %s\n\n", name);
				return -1;
			}
			state = extractState (info->state);
			fprintf (stdout, "%s\n\n", state);
		}
		break;
		
		case START: {
			int isret, conNum;
			char hostname [50];
			char name [50];
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
			fprintf (stdout, "Enter Domain name: ");
			scanf ("%s", name);
			virDomainPtr dom;
			dom = virDomainLookupByName (connection[conNum].conn, name);
			if (dom == NULL) {
				fprintf (stdout, "Error: Invalid domain %s\n\n", name);
				return -1;
			}
			globalDomainHandler = dom;
			createDomain (conNum, START_THREAD);
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
			isret = isDomCreated (domName, conNum);
			if (isret < 0) {
				fprintf (stderr, "Error: Domain %s does not exists\n\n", domName);
				return -1;
			}
			isret = virDomainShutdown(connection[conNum].domain[isret].dom);
			virConnectClose (connection[conNum].conn);
			if (isret != 0) {
				fprintf (stderr, "Error: Cannot shutdown domain object\n\n");
				return -1;
			}
			fprintf (stdout, "Domain %s ready for shutdown on %s\n\n", domName, hostname);
		}
		break;
		
		case DESTROY: {
			fprintf (stdout, "Enter domain name: ");
			char domName [20];
			bzero (domName, 20);
			int isret, conNum;
			scanf ("%s", domName);
			char hostname [50];
			bzero (hostname, 50);
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
			isret = isDomCreated (domName, conNum);
			if (isret < 0) {
				fprintf (stderr, "Error: Domain %s does not exists\n\n", domName);
				return -1;
			}
			isret = virDomainDestroy(connection[conNum].domain[isret].dom);
			virConnectClose (connection[conNum].conn);
			if (isret != 0) {
				fprintf (stderr, "Error: Cannot destroy domain object\n\n");
				return -1;
			}
			connection[conNum].domain[isret].isdefined = 0;
			connection[conNum].domain[isret].isrunning = 0;
			fprintf (stdout, "Domain %s destroy on %s\n\n", domName, hostname);
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
			
			//isret = virDomainReboot (dom, VIR_DOMAIN_REBOOT_DEFAULT);
			if (isret < 0) {
				fprintf (stderr, "Error: Cannot reboot domain %s\n\n", name);
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
			dominfo = malloc (sizeof (virDomainInfo));
			isret = virDomainGetInfo (dom, dominfo);
			if (isret != 0) {
				fprintf (stderr, "Error: Cannot get info about domain %s\n\n", name);
				return -1;
			}
			printDomInfo (dominfo);
		}
		break;
		
		case DOMLIST: {
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
			printDomList(conNum);
		}
		break;
		
		default:
			fprintf (stderr, "Error: Invalid input\n\n");
		break;
	}
	return 0;
}
