#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <libvirt/libvirt.h>			// INCLUDING THE LIBVIRT LIBRARY
#include "vm_functions.h"

/* ---------------------------------------MAIN--------------------------------------------------- */
int main (int argc, char *argv[]) {
	virConnectPtr conn;			// CREATING AN OBJECT USING virConnectPtr HANDLER
	if (argc != 2) 
		vm_error ("Insufficient arguments");
	char *uri = argv[1];
	conn = virConnectOpen (uri);		// CONNECTING TO LOCAL XEN HYPERVISOR 
	if (conn == NULL)
		fprintf (stderr, "ERROR %d: Failed to open connection to %s", errno, uri);
	vm_getcapabilities (conn);
	virConnectClose (conn);			// CLOSING CONNECTION
	return 0;
}

