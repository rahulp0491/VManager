#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <libvirt/libvirt.h>
#define BUFFSIZE 100


/* ERROR */
void vm_error (char *errMsg) {
	fprintf(stderr, "ERROR: %s\n", errMsg);
	exit(1);
}

void vm_getcapabilities (virConnectPtr conn) {
	FILE *fp;
	char *caps;
	fp = fopen ("capabilities.xml", "w");
	caps = virConnectGetCapabilities(conn);
	fprintf(fp, "%s\n", caps);
}
