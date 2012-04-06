#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <libvirt/libvirt.h>			// INCLUDING THE LIBVIRT LIBRARY

virConnectPtr conn;

/* -MAIN- */
int main (int argc, char *argv[]) {
	
	conn = virConnectOpen ("qemu:///session");
	if (conn == NULL) {
		fprintf (stderr, "Failed to open connection to qemu:///session\n");
		return -1;
	}
	printf ("Connection to qemu:///session established\n");


	char *s;
	s = virConnectGetCapabilities (conn);
	printf ("Capabilities:\n%s\n", s);
	
	free(s);
	s = virConnectGetHostname (conn);
	printf ("Hostname: %s\n", s);
	
	
	if (virConnectClose (conn) != 0) {
		fprintf (stderr, "Cannot close connection to qemu:///session\n");
		return -1;
	}
	
	printf ("Connection to qemu:///session closed\n");
	return 0;
}

