struct client{
	bool inCall, online, video;
	char name[10];
	int gp[10], g;
	struct sockets *sockts;
	struct session *ptr;
	pthread_mutex_t mutex;
	// std::string key;
};
struct group{
	char name[10];
	struct client *mem[10];
	int count;
};
struct session{
	struct client *mem[10];
	int count;
};
struct sockets{
	int sd, ssd, vsd;
};

extern struct client *clients[200];
extern struct group groups[20], *glptr;
extern int cli, temp_ind, grp;
extern void handleSigint(int sig);
extern void* clientHandler(void* input);