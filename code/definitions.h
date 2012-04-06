#define MaxNumDomains 10
#define MaxConfigSize 10000
#define MaxFileName 100
#define MaxNumConnections 10

#define NumOfInputOptions 15

#define CONNECT 1
#define CLOSECON 2
#define DUMPXML 3
#define CREATEDOM 4
#define SUSPEND 5
#define RESUME 6
#define SAVE 7
#define RESTORE 8
#define SHUTDOWN 9
#define REBOOT 10
#define DESTROY 11
#define NUMDOMAIN 12
#define NODEINFO 13
#define NODELIST 14
#define NODECAP 15

int assignNum (char *input);
int handleInput (int input);
int getNextDomainThreadNum (int conNum);
int createDomain (int conNum);
int getNextDomain (int conNum);
void *manageDomain (void *arg);
int createConnection (int conNum);
void *manageConnections (void *arg);
int isConnectionEstablished (char *hostname);
int getNextConnThread ();
void printNodeInfo (virNodeInfo nodeinfo);
void printNodeList ();