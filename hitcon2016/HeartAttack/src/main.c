#include "HeartAttack.h"

/*
 * main
 * serv
 * help
 * whoami
 * sigalarm
 * sends
 * findid
 * remove_line
 */


int main(int argc, char **argv)
{
    int listenfd, connfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len; 
    pid_t child_pid; 
    unsigned short wPortNum = SRV_PORT;
    int shmid = shmget(IPC_PRIVATE, sizeof(struct Data), 0666);
    data = (struct Data*) shmat(shmid, 0, 0);
    memset(data, 0, sizeof(data));
    int i;
    if( argc > 1)
        wPortNum = atoi(argv[1]);
    // socket
    if( (listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        perror("listen error");
        exit(1);
    }
    memset(&server_addr, 0, sizeof(server_addr));
    memset(&client_addr, 0, sizeof(client_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(wPortNum);
    if( bind(listenfd, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind error");
        exit(1);
    }
    if( listen(listenfd, LISTEN_ENQ) < 0)
    {
        perror("listen error");
        exit(1);
    }

    asm volatile("dec %rbx");
    signal(SIGCHLD, SIG_IGN);
    signal(SIGALRM, sig_alarm);

    while(true)
    {
        client_len = sizeof(client_addr);
        if( (connfd = accept(listenfd, (struct sockaddr*) &client_addr, &client_len)) < 0)
        {
            perror("accept error");
            continue;
        }

        int cli_id;
        int i;
        
        for(i=0; i<MAX_CLIENT; i++)
        {
            // is occupied
            if(data->client_valid[i])
            {
                // check first
                if(kill(data->pid[i], 0) == 0)
                    continue;
            }
            data->client_valid[i] = true;
            cli_id = i;
            data->sockfd[i] = connfd;
            memset(data->name[i], 0, sizeof(data->name[i]));
            data->player_game[i] = -1;
            data->is_on[i] = false;
            break;
        }
        if(i == MAX_CLIENT)
            continue;
        for(i=0 ; i<MAX_GAME ; i++)
        {
            if(data->game_valid[i])
            {
                int j;
                bool is_empty = true;
                for(j=0 ; j<data->game_sum[i] ; j++)
                {
                    if(data->client_valid[data->uid[i][j]])
                        is_empty = false;
                }
                if(is_empty)
                    data->game_valid[i] = false;
            }
            
        }

        // fork a new child process
        if( (child_pid = fork()) == 0)
        {
            rand_fd = open("/dev/urandom", O_RDONLY);
            if(rand_fd == -1)
            {
                perror("Urandom error.\n");
                exit(1);
            }
            alarm(TIMEOUT);
            close(listenfd);
            int pid = getpid();
            data->pid[cli_id] = pid;
            opensocket();
            serv(connfd, cli_id);
            closesocket();
            exit(0);
        }
        close(connfd);
    }
    exit(0);

}

void serv(int sockfd, int id)
{
    setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, NULL, sizeof(int));
    setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, NULL, sizeof(int));
    fd_set rset;
    int recv_len;
    char buf[MAX_LEN];
    head = NULL;
    sends(sockfd, "Welcome to the HeartAttack server! \nWhat's your name?\n");
    memset(buf, 0, sizeof(buf));
    read(sockfd, buf, MAX_LEN-1);
    remove_line(buf);
    strcpy(data->name[id], buf);

    sends(sockfd, "You can type \"/help\" to get some information! Enjoy the game!:)\n");
    data->is_on[id] = true;
    
    
    while(true)
    {
        FD_ZERO(&rset);
        FD_SET(sockfd, &rset);
        FD_SET(u_sockfd, &rset);

        select(MAX(sockfd, u_sockfd)+1, &rset, NULL, NULL, NULL);
        if(FD_ISSET(sockfd, &rset))
        {
            recv_len = read(sockfd, buf, MAX_LEN-1);
            if(!recv_len)
                break;

            // not a command
            if(buf[0] != '/')
            {
                if(!strcmp(buf, "g\n"))
                {
                    player_slap(sockfd, id);
                }
                else if(buf[0] == '\n')
                {
                    player_flip(sockfd, id);
                }
                else if(data->is_on[id])
                    broadcast(id, buf);
                memset(buf, 0, sizeof(buf));
                continue;
            }

            // parse argv
            char argv[3][10]; 
            memset(argv, 0, sizeof(argv));
            int argc = sscanf(buf, "%9s %4s %4s", argv[0], argv[1], argv[2]);

            memset(buf, 0, sizeof(buf));

            if(!strcmp(argv[0], "/help"))
                help(sockfd);
            else if(!strcmp(argv[0], "/whoami"))
                whoami(sockfd, id);
            else if(!strcmp(argv[0], "/on"))
            {
                sends(sockfd, "Turn on chat mode.\n");
                data->is_on[id] = true;
            }
            else if(!strcmp(argv[0], "/off"))
            {
                sends(sockfd, "Turn off chat mode.\n");
                data->is_on[id] = false;
            }
            else if(!strcmp(argv[0], "/msg"))
            {
                if(argc != 3)
                {
                    sends(sockfd, "Usage: /msg player_id len\n");
                    continue;
                } 
                leave_msg(sockfd, (unsigned int)atoi(argv[1]), (unsigned int)atoi(argv[2]));
            }
            else if(!strcmp(argv[0], "/ban"))
            {
                if(argc != 2)
                {
                    sends(sockfd, "Usage: /ban player_id\n");
                    continue;
                } 
                ban(sockfd, id, (unsigned int)atoi(argv[1]));
            }
            else if(!strcmp(argv[0], "/log"))
                show_msg(sockfd);
            else if(!strcmp(argv[0], "/del"))
            {
                if(argc != 2)
                {
                    sends(sockfd, "Usage: /del message_id\n");
                    continue;
                } 
                del_msg(sockfd, (unsigned int)atoi(argv[1]));
            }
            else if(!strcmp(argv[0], "/room"))
                list_room(sockfd);
            else if(!strcmp(argv[0], "/player"))
                list_player(sockfd);
            else if(!strcmp(argv[0], "/create"))
                create(sockfd, id);
            else if(!strcmp(argv[0], "/join"))
            {
                if(argc != 2)
                {
                    sends(sockfd, "Usage: /join room_id\n");
                    continue;
                } 
                join(sockfd, id, (unsigned int)atoi(argv[1]));
            }
            else if(!strcmp(argv[0], "/leave"))
                player_leave(sockfd, id);
            else if(!strcmp(argv[0], "/start"))
                start(sockfd, id);
            else if(!strcmp(argv[0], "/exit"))
                player_exit(sockfd, id);
            else
                sends(sockfd, "Unknown command.\n");
        }
        if(FD_ISSET(u_sockfd, &rset))
        {
            ipc_msg(id);
        }
    }

    data->client_valid[id] = false;
    close(sockfd);
    if(data->player_game[id] != -1)
        leave(id, data->player_game[id]);
}

void help(int sockfd)
{
    sends(sockfd, "------------------------------------------------------------------------------\n\n");
    sends(sockfd, "HeartAttack is a simple card game.\n"
    "A 52-card deck is divided into face-down stacks \n"
    "  as equally as possible between all players.\n"
    "There is a counter counts between 1 and 13 which\n"
    "  will increase after a card is flipped.\n"
    "One player removes the top card of his stack and \n"
    "  places it face-up on the playing surface within\n"
    "  reach of all players. \n"
    "The players take turns doing this in a clockwise\n"
    "  manner until the card number matches the counter. \n"
    "At this point, any and all players may attempt to \n"
    "  slap the pile with the hand they used to place \n"
    "  the card; whoever covers the stack with his hand \n"
    "  last takes the pile, shuffles it, and adds it to \n"
    "  the bottom of his stack.\n"
    "When a player has run out of cards, he is the winner of the game. \n\n");
    sends(sockfd, "------------------------------------------------------------------------------\n\n");
    sends(sockfd, "Command list\n\n");
    sends(sockfd, "\tWhen you are in the lobby, you can type\n");
    sends(sockfd, "\t\t/whoami       : Get your own information\n");
    sends(sockfd, "\t\t/ban uid      : Ban someone\n");
    sends(sockfd, "\t\t/create       : Create a game room\n");
    sends(sockfd, "\t\t/join roomid  : Join a game room\n");
    sends(sockfd, "\t\t/leave        : Leave a game room\n");
    sends(sockfd, "\t\t/room         : List existing rooms\n");
    sends(sockfd, "\t\t/player       : List existing players\n");
    sends(sockfd, "\t\t/start        : Start the game (only the host can do this)\n");
    sends(sockfd, "\t\t/msg uid len  : Leave private messages to someone\n");
    sends(sockfd, "\t\t/log          : Show the messages you have leaved\n");
    sends(sockfd, "\t\t/del msgid    : Delete a message from log\n");
    sends(sockfd, "\t\t/exit         : Exit from HeartAttack server\n");
    sends(sockfd, "\t\tother         : Any sentence without a slash will be showed as public messages and won't be logged\n\n");
    sends(sockfd, "\tWhen you are in a game, you can use\n");
    sends(sockfd, "\t\t\\n (enter)    : Draw a card and flip it (only in your turn)\n");
    sends(sockfd, "\t\tg             : When the card number matches the counter, press g to slap\n\n");

}


void sig_alarm(int signo)
{
    int id = findid();
    if(id >= 0)
        sends(data->sockfd[id], "Timeout!\n");
    exit(1);
}

void sends(int sockfd, char *buf)
{
    write(sockfd, buf, strlen(buf));
}

int findid()
{
    pid_t pid = getpid();
    return pid2id(pid); 
}

int pid2id(pid_t pid)
{
    int i;
    for(i=0 ; i<MAX_CLIENT ; i++)
        if(data->pid[i] == pid && data->client_valid[i])
            return i;
    return -1;
}



void remove_line(char *buf)
{
    int i;
    int len = strlen(buf);
    for(i=0 ; i<len ; i++)
    {
        if(buf[i] == '\n')
        {
            buf[i] = '\0';
            break;
        }
    }
}




