#include <stdio.h>
#include <stdlib.h>
#include <libvirt/libvirt.h>			// INCLUDING THE LIBVIRT LIBRARY

/* -------------------------------------ERROR LOG -----------------------------------------------*/
void error (char *errMsg) {
	fprintf (stderr, "ERROR: %s\n", errMsg);
	exit(1);
}

/* ---------------------------------------MAIN--------------------------------------------------- */
int main (int argc, char *argv[]) {
	virConnectPtr conn;			// CREATING AN OBJECT USING virConnectPtr HANDLER
	conn = virConnectOpen ("xen:///system");		// CONNECTING TO LOCAL XEN HYPERVISOR 
	if (conn == NULL)
		error ("Failed to open connection to xen:///system");
	
	virConnectClose (conn);			// CLOSING CONNECTION
	return 0;
}

