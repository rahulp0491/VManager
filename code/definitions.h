#define MaxNumDomains 10
#define MaxConfigSize 10000
#define MaxFileName 100
#define MaxNumConnections 10

#define CREATE_THREAD 1
#define START_THREAD 2

#define NumOfInputOptions 20

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
#define DOMINFO 11
#define NUMDOMAIN 12
#define NODEINFO 13
#define NODELIST 14
#define NODECAP 15
#define LOAD 16
#define DOMLIST 17
#define DESTROY 18
#define DEFINE 19
#define START 20


int globalConHandler;
virDomainPtr globalDomainHandler;

int assignNum (char *input);
int handleInput (int input);
int getNextDomainThreadNum (int conNum);
void *manageDomain (void *arg);
void createConnection (int conNum);
int isConnectionEstablished (char *hostname);
int getNextConnThread ();
void printNodeInfo (virNodeInfo nodeinfo);
void printNodeList ();
int connectionWithSameURI (char *uri);
void printDomInfo (virDomainInfoPtr dominfo);
int isDomCreated (char *domName, int conNum);
void printDomList (int conNum);
void createDomain (int conNum, int flag);
void *startDomain (void *arg);
int isDomainDefined (char *xml, int conNum);
void printDomList (int conNum);