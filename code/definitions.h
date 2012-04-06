#define MaxNumThreads 10
#define MaxConfigSize 10000
#define MaxFileName 100
#define MAxGuestDomains 5

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

int assignNum (char *input);
int handleInput (int input);
int getNextThreadNum ();
int createDomain (int domainNum);
int getNextDomain ();
void *manageDomain (void *arg);