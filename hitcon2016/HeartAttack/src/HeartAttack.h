#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/select.h>
#include <ctype.h>
#include <stdbool.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>

#define SRV_PORT 5566
#define LISTEN_ENQ 5
#define MAX_LEN 1024
#define MAX_CLIENT 1000
#define MAX_GAME 50
#define TIMEOUT 3600
#define SOCKET_PATH "/tmp/.%d"
#define MAX(a,b) ((a) > (b) ? (a) : (b))

typedef enum msgtype{
    MSG_SYSTEM,
    MSG_BROADCAST,
    MSG_PRIVATE
} msgtype;

typedef enum dir{
    DIR_SEND,
    DIR_RECV

} dir;

struct msg
{
    msgtype type;
    unsigned int size;
    char buf[0x1000];    
};

// main.c
void serv(int, int );
void help(int );
void sig_alarm(int );
void sends(int, char *);
int findid();
int pid2id(pid_t );
void remove_line(char *);

// msg.c
void broadcast(int , char*);
void leave_msg(int , unsigned int , unsigned int );
void recv_msg(int, struct msg*, int);
void show_msg(int );
void del_msg(int , unsigned int);
struct node *add_node(unsigned int );
bool del_node(unsigned int );

// room.c
void ban(int, int, unsigned int);
void whoami(int, int);
void list_room(int );
void list_player(int );
void create(int, int);
void join(int, int, unsigned int);
void leave(int, int);
bool player_leave(int, int);
void player_exit(int, int);
void end_game(int);

// game.c
void start(int, int);
void slap(int, int);
void flip(int, int);
void player_slap(int, int);
void player_flip(int, int);
void AI_slap(int);
void AI_flip(int);
void show_status(int);
static void *AI_start(void *);
void sig_AI();
char *card_name(unsigned int );
char *card_graph(unsigned int );

// sock.c
void opensocket();
void closesocket();
void ipc_msg(int );
void psend(int, char *, msgtype);


struct node
{
    struct node *next;
    char *str;
    char name[16];
    int id;
    int direction;
};

struct Data
{
    pid_t pid[MAX_CLIENT];
    bool client_valid[MAX_CLIENT];
    bool is_on[MAX_CLIENT];
    int sockfd[MAX_CLIENT];
    int player_game[MAX_CLIENT];
    char name[MAX_CLIENT][MAX_LEN];
    bool game_valid[MAX_GAME];
    int game_host[MAX_GAME];
    int game_sum[MAX_GAME];
    bool game_start[MAX_GAME];
    int game_cards[MAX_GAME][52];
    int player_card[MAX_GAME][4];
    int game_counter[MAX_GAME];
    int game_turnto[MAX_GAME];
    int uid[MAX_GAME][4];
    bool is_slap[MAX_GAME][4];
    bool is_match[MAX_GAME];
    int slap_count[MAX_GAME];

    pthread_mutex_t game_lock[MAX_GAME];
    //pthread_mutex_t child_lock;
};

struct node *head;
struct Data *data;
int rand_fd;
int u_sockfd;
unsigned int *blacklist;
unsigned int blacknum;

