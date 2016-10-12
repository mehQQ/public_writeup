#include "HeartAttack.h"

/*
 * start
 * slap
 * flip
 * AI
 * counter
 * turn
 * card
 */




void start(int sockfd, int id)
{
    int gid;
    if(data->player_game[id] == -1)
    {
        sends(sockfd, "Create or join a game room first!\n");
        return;
    }
    gid = data->player_game[id];
    if(data->game_host[gid] != id)
    {
        sends(sockfd, "You are not the host!\n");
        return;
    }
    
    // deal cards
    memset(data->game_cards[gid], -1, sizeof(data->game_cards[gid]));
    memset(data->player_card[gid], 0, sizeof(data->player_card[gid]));
    data->game_start[gid] = true;
    data->is_match[gid] = false;
    data->game_counter[gid] = 0;
    // start from host
    data->game_turnto[gid] = 0;
    int pid[4];
    int i;
    char buf[MAX_LEN];
    for(i=0 ; i<data->game_sum[gid] ; i++)
    {
        pid[i] = data->pid[data->uid[gid][i]];
    }
    char start_msg[] = "\n\n   Game Start!  \n\n";
    for(i=0 ; i<data->game_sum[gid] ; i++)
    {
        if(data->game_sum[gid] < 4)
            sprintf(buf, "%d computer players added!%s", (4-data->game_sum[gid]), start_msg);
        else
            strcpy(buf, start_msg);
        sprintf(buf, "%sYou are \033[1;35mPlayer %d\033[0m\n\n", buf, i+1);
        if(id == data->uid[gid][i])
            sends(data->sockfd[id], buf);
        else
            psend(data->uid[gid][i], buf, MSG_SYSTEM);
    }
    int fd = rand_fd;
    if(fd == -1)
    {
        perror("urandom error\n");
        exit(1);
    }
    
    unsigned int num;
    int j;
    for(i=0 ; i<4 ; i++)
    {
        for(j=0 ; j<13 ; j++)
        {
            while(true)
            {
                read(fd, &num, sizeof(num));
                num %= 52;
                if(data->game_cards[gid][num] == -1)
                {
                    data->game_cards[gid][num] = i;
                    data->player_card[gid][i]++;
                    break;
                }
            }
        }
    }

    usleep(500000);
    show_status(gid);
}

// hit!!
void player_slap(int sockfd, int id)
{
    if(data->player_game[id] == -1)
        return;
    int gid = data->player_game[id];
    if(!data->game_valid[gid])
        return;
    if(!data->game_start[gid])
        return;

    int pid[4];
    int my_id;
    int i;
    char buf[MAX_LEN];
    for(i=0 ; i<data->game_sum[gid] ; i++)
    {
        pid[i] = data->pid[data->uid[gid][i]];
        if(data->uid[gid][i] == id)
            my_id = i;
    }
    
    // slap wrong!
    if(!data->is_match[gid])
    {
        for(i=0 ; i<52 ; i++)
        {
            if(data->game_cards[gid][i] == -1)
            {
                data->game_cards[gid][i] = my_id;
                data->player_card[gid][my_id]++;
            }
        }
        data->game_turnto[gid] = my_id;
        data->game_counter[gid] = 0;
        
        for(i=0 ; i<data->game_sum[gid] ; i++)
        {
            sprintf(buf, "Player %.20s slapped incorrectly! All cards has been given to player %.20s!\n\n", data->name[id], data->name[id]);
            psend(data->uid[gid][i], buf, MSG_SYSTEM);
        }
        usleep(500000);
        show_status(gid);
    }
    else if(data->is_slap[gid][my_id])
    {
        sends(sockfd, "You have already slapped. Wait for other players.\n");
    }
    else
    {
        // player slaps correctly, lock the game
        while(pthread_mutex_trylock(&data->game_lock[gid]))
            usleep(10);
        slap(my_id, gid);    
        pthread_mutex_unlock(&data->game_lock[gid]);
    }


}

void slap(int uid, int gid)
{
    int pid[4];
    int id = data->uid[gid][uid];
    int i;
    char buf[MAX_LEN];
    for(i=0 ; i<data->game_sum[gid] ; i++)
        pid[i] = data->pid[data->uid[gid][i]];
    data->is_slap[gid][uid] = true;
    data->slap_count[gid]++;

    // messages
    if(uid < data->game_sum[gid])
    {
        for(i=0 ; i<data->game_sum[gid] ; i++)
        {
            sprintf(buf, "Player %.20s Slapped!\n", data->name[id]);
            if(data->uid[gid][i] == uid)
                sends(data->sockfd[uid], buf);
            else
                psend(data->uid[gid][i], buf, MSG_SYSTEM);
        }
    }
    else
    {
        for(i=0 ; i<data->game_sum[gid] ; i++)
        {
            sprintf(buf, "Computer %d Slapped!\n", (uid - data->game_sum[gid]+1));
            psend(data->uid[gid][i], buf, MSG_SYSTEM);
        }
    }

    // everyone has slapped
    if(data->slap_count[gid] == 4)
    {

        // not AI
        if(uid < data->game_sum[gid])
        {
            for(i=0 ; i<data->game_sum[gid] ; i++)
            {
                sprintf(buf, "Player %.20s is the last one! All cards has been given to player %.20s!\n\n", data->name[id], data->name[id]);
                psend(data->uid[gid][i], buf, MSG_SYSTEM);
            }
        }
        else
        {
            for(i=0 ; i<data->game_sum[gid] ; i++)
            {
                sprintf(buf, "Computer %d is the last one! All cards has been given to Computer %d!\n\n", (uid - data->game_sum[gid]+1), (uid - data->game_sum[gid]+1));
                psend(data->uid[gid][i], buf, MSG_SYSTEM);
            }
            
        }
        
        int win = data->game_turnto[gid];
        // check if someone wins
        if(win != uid)
        {
            if(data->player_card[gid][win] == 0)
            {
                if(win < data->game_sum[gid])
                    sprintf(buf, "Player %.20s wins!!\nBack to the lobby ...\n\n", data->name[data->uid[gid][win]]);
                else
                    sprintf(buf, "Computer %d wins!!\nBack to the lobby ...\n\n", (win-data->game_sum[gid]+1));
                // show end message
                for(i=0 ; i<data->game_sum[gid] ; i++)
                    psend(data->uid[gid][i], buf, MSG_SYSTEM);
                end_game(gid);
                return;
            }
        }

        // no one wins, give card to the loser
        for(i=0 ; i<52 ; i++)
        {
            if(data->game_cards[gid][i] == -1)
            {
                data->game_cards[gid][i] = uid;
                data->player_card[gid][uid]++;
            }
        }
        // set counter and turn
        data->game_counter[gid] = 0;
        data->is_match[gid] = false;
        data->game_turnto[gid] = uid;

        sleep(1);
        pthread_t thd;
        // if last one is player, call him to flip
        // otherwise, call ai
        show_status(gid);
        if(!(uid < data->game_sum[gid]))
            pthread_create(&thd, NULL, &AI_start, NULL);

    }
    return;
}

void flip(int uid, int gid)
{
    int pid[4];
    int sum = data->player_card[gid][uid];
    int fd = rand_fd;
    int i;
    char buf[MAX_LEN];
    pthread_t thd[4];
    if(fd == -1)
    {
        perror("urandom error\n");
        exit(1);
    }
    for(i=0 ; i<data->game_sum[gid] ; i++)
    {
        pid[i] = data->pid[data->uid[gid][i]];
    }
    unsigned int victim;
    read(fd, &victim, sizeof(victim));
    victim %= data->player_card[gid][uid];
    int  j;
    for(i=0,j=0 ; i<52 ; i++)
    {
        if(data->game_cards[gid][i] == uid)
        {
            if(j == victim)
            {
                victim = i;
                break;
            }
            j++;
        }
    }

    data->game_cards[gid][victim] = -1;
    data->player_card[gid][uid]--;
    if(data->game_counter[gid] == victim%13)
        data->is_match[gid] = true;

    char *pic = card_graph(victim);
    char *name = card_name(victim);
    sprintf(buf, "\n\n\n                  %s\n\n%s\n\n", name, pic);
    // show every player the card
    for(i=0 ; i<data->game_sum[gid] ; i++)
    {
        if(data->uid[gid][i] == uid)
            sends(data->sockfd[uid], buf);
        else
            psend(data->uid[gid][i], buf, MSG_SYSTEM);
    
    }

    // not match
    if(!data->is_match[gid])
    {
        // check if the flipper wins
        if(data->player_card[gid][uid] == 0)
        {
            // player wins
            if(uid < data->game_sum[gid])
                sprintf(buf, "Player %.20s wins!!\nBack to the lobby ...\n\n", data->name[data->uid[gid][uid]]);
            else
                sprintf(buf, "Computer %d wins!!\nBack to the lobby ...\n\n", (uid-data->game_sum[gid]+1));
            // show end message
            for(i=0 ; i<data->game_sum[gid] ; i++)
                psend(data->uid[gid][i], buf, MSG_SYSTEM);
            end_game(gid);
            return;
        }
        char count_msg[] = "";
        data->game_counter[gid]++;
        data->game_counter[gid] %= 13;
        data->game_turnto[gid]++;
        data->game_turnto[gid] %= 4;

        usleep(500000);
        show_status(gid);
        // AI's turn, call AI to flip
        if(data->game_turnto[gid] >= data->game_sum[gid])
        {
            pthread_create(&thd[0], NULL, &AI_start, NULL);
        }
    }
    else
    {
        for(i=0 ; i<4 ; i++)
            data->is_slap[gid][i] = false;
        data->slap_count[gid] = 0;
        // call AI to slap
        for(i=0 ; i<(4-data->game_sum[gid]) ; i++)
        {
            pthread_create(&thd[i], NULL, &AI_start, NULL);
        }
    }
    return;
}

void AI_slap(int gid)
{
    if(!data->game_valid[gid])
        return;
    unsigned int n;
    int fd = rand_fd;
    signed int time;
    read(fd, &n, sizeof(n));

    // AI slapped, lock the game
    while(pthread_mutex_trylock(&data->game_lock[gid]))
        usleep(10);
    int i,j;
    int ai_count=0;
    for(i=data->game_sum[gid],j=0 ; i<4 ; i++)
        if(!data->is_slap[gid][i])
            ai_count++;

    n %= ai_count;

    for(i=data->game_sum[gid],j=0 ; i<4 ; i++)
    {
        if(!data->is_slap[gid][i])
        {
            if(j == n)
            {
                slap(i, gid);
                break;
            }
            j++;    
        }
    }
    pthread_mutex_unlock(&data->game_lock[gid]);
}

// open a new card
void player_flip(int sockfd, int id)
{
    if(data->player_game[id] == -1)
        return;
    int gid = data->player_game[id];
    if(!data->game_valid[gid])
        return;
    if(!data->game_start[gid])
        return;
    if(data->is_match[gid])
        return;
    int my_id;
    int i;
    for(i=0 ; i<data->game_sum[gid] ; i++)
    {
        if(data->uid[gid][i] == id)
            my_id = i;
    }
    if(data->game_turnto[gid] == my_id)
    {
        flip(my_id, gid);
    }
    else
    {
        char err_msg[] = "\n   It's not your turn!  \n";
        sends(sockfd, err_msg);
        return;
    }
}

void show_status(int gid)
{
    char msg[] = " -------------------------------------------------------\n"
                 " | Turn to: Player %2d | \033[1;36mCounter:  %2d\033[0m  | Left cards: %2d |\n"
                 " -------------------------------------------------------\n";

    char your_msg[] = " -------------------------------------------------------\n"
                      " | Turn to:  \033[1;35mYou!!!!!\033[0m  | \033[1;36mCounter:  %2d\033[0m  | Left cards: %2d |\n"
                      " -------------------------------------------------------\n";

    char buf[MAX_LEN];
    int i;
    for(i=0 ; i<data->game_sum[gid] ; i++)
    {
        if(i == data->game_turnto[gid])
            sprintf(buf, your_msg, data->game_counter[gid]+1, data->player_card[gid][i]);
        else
            sprintf(buf, msg, data->game_turnto[gid]+1, data->game_counter[gid]+1, data->player_card[gid][i]);
        psend(data->uid[gid][i], buf, MSG_SYSTEM);
    }
}


void AI_flip(int gid)
{
    if(!data->game_valid[gid])
        return;
    if(!data->is_match[gid] && data->game_turnto[gid] >= data->game_sum[gid])
        flip(data->game_turnto[gid], gid);
}

static void *AI_start(void *argv)
{
    pthread_detach(pthread_self());
    int fd = rand_fd;
    if(fd == -1)
    {
        perror("urandom error\n");
        exit(1);
    }
    signed int time;
    read(fd, &time, sizeof(time));
    if(time < 0)
        time *= (-1);
    time %= 800000;
    time += 1000000;
    usleep(time);
    sig_AI();
}

void sig_AI()
{
    int pid = getpid();
    int i, gid;
    for(i=0 ; i<MAX_CLIENT ; i++)
        if(data->pid[i] == pid)
            break;
    gid = data->player_game[i];
    if(data->is_match[gid])
        AI_slap(gid);
    else
        AI_flip(gid);

}


char *card_name(unsigned int n)
{
    char *buf = (char *)calloc(1, 24);
    char num[4];
    //n = 35;
    switch(n%13)
    {
        case 0:
            strcpy(num, "A");
            break;
        case 10:
            strcpy(num, "J");
            break;
        case 11:
            strcpy(num, "Q");
            break;
        case 12:
            strcpy(num, "K");
            break;
        default:
            sprintf(num, "%u", (n%13)+1);
    }

    switch(n/13)
    {
        case 0:
            sprintf(buf, "♠   Spades  %s   ♠", num);
            break;
        case 1:
            sprintf(buf, "♥   Hearts  %s   ♥", num);
            break;
        case 2:
            sprintf(buf, "♦   Diamonds  %s   ♦", num);
            break;
        case 3:
            sprintf(buf, "♣   Clubs  %s   ♣", num);
            break;
    }
    return buf;
}

char *card_graph(unsigned int n)
{
    char *buf = (char *)calloc(1, 600);
    char *c;
    switch(n/13)
    {
        case 0:
            c = "♠";
            break;
        case 1:
            c = "♥";
            break;
        case 2:
            c = "♦";
            break;
        case 3:
            c = "♣";
            break;
    }
    switch(n%13)
    {
        case 0:
            sprintf(buf, "                    ------------- \n                   |A%s           |\n                   |   -------   |\n                   |  |       |  |\n                   |  |       |  |\n                   |  |       |  |\n                   |  |   %s   |  |\n                   |  |       |  |\n                   |  |       |  |\n                   |  |       |  |\n                   |   -------   |\n                   |          %s A|\n                    ------------- \n", c, c, c);
            break;
        case 1:
            sprintf(buf, "                    ------------- \n                   |2%s           |\n                   |   -------   |\n                   |  |       |  |\n                   |  |   %s   |  |\n                   |  |       |  |\n                   |  |       |  |\n                   |  |       |  |\n                   |  |   %s   |  |\n                   |  |       |  |\n                   |   -------   |\n                   |          %s 2|\n                    ------------- \n", c, c, c, c);
            break;
        case 2:
            sprintf(buf, "                    ------------- \n                   |3%s           |\n                   |   -------   |\n                   |  |       |  |\n                   |  |   %s   |  |\n                   |  |       |  |\n                   |  |   %s   |  |\n                   |  |       |  |\n                   |  |   %s   |  |\n                   |  |       |  |\n                   |   -------   |\n                   |          %s 3|\n                    ------------- \n", c, c, c, c, c);
            break;
        case 3:
            sprintf(buf, "                    ------------- \n                   |4%s           |\n                   |   -------   |\n                   |  |%s     %s|  |\n                   |  |       |  |\n                   |  |       |  |\n                   |  |       |  |\n                   |  |       |  |\n                   |  |       |  |\n                   |  |%s     %s|  |\n                   |   -------   |\n                   |          %s 4|\n                    ------------- \n", c, c, c, c, c, c);
            break;
        case 4:
            sprintf(buf, "                    ------------- \n                   |5%s           |\n                   |   -------   |\n                   |  |%s     %s|  |\n                   |  |       |  |\n                   |  |       |  |\n                   |  |   %s   |  |\n                   |  |       |  |\n                   |  |       |  |\n                   |  |%s     %s|  |\n                   |   -------   |\n                   |          %s 5|\n                    ------------- \n", c, c, c, c, c, c, c);
            break;
        case 5:
            sprintf(buf, "                    ------------- \n                   |6%s           |\n                   |   -------   |\n                   |  |%s     %s|  |\n                   |  |       |  |\n                   |  |       |  |\n                   |  |%s     %s|  |\n                   |  |       |  |\n                   |  |       |  |\n                   |  |%s     %s|  |\n                   |   -------   |\n                   |          %s 6|\n                    ------------- \n", c, c, c, c, c, c, c, c);
            break;
        case 6:
            sprintf(buf, "                    ------------- \n                   |7%s           |\n                   |   -------   |\n                   |  |%s     %s|  |\n                   |  |   %s   |  |\n                   |  |       |  |\n                   |  |%s     %s|  |\n                   |  |       |  |\n                   |  |       |  |\n                   |  |%s     %s|  |\n                   |   -------   |\n                   |          %s 7|\n                    ------------- \n", c, c, c, c, c, c, c, c, c);
            break;
        case 7:
            sprintf(buf, "                    ------------- \n                   |8%s           |\n                   |   -------   |\n                   |  |%s     %s|  |\n                   |  |   %s   |  |\n                   |  |       |  |\n                   |  |%s     %s|  |\n                   |  |       |  |\n                   |  |   %s   |  |\n                   |  |%s     %s|  |\n                   |   -------   |\n                   |          %s 8|\n                    ------------- \n", c, c, c, c, c, c, c, c, c, c);
            break;
        case 8:
            sprintf(buf, "                    ------------- \n                   |9%s           |\n                   |   -------   |\n                   |  |%s     %s|  |\n                   |  |       |  |\n                   |  |%s     %s|  |\n                   |  |   %s   |  |\n                   |  |%s     %s|  |\n                   |  |       |  |\n                   |  |%s     %s|  |\n                   |   -------   |\n                   |          %s 9|\n                    ------------- \n", c, c, c, c, c, c, c, c, c, c, c);
            break;
        case 9:
            sprintf(buf, "                    ------------- \n                   |10%s          |\n                   |   -------   |\n                   |  |%s     %s|  |\n                   |  |   %s   |  |\n                   |  |%s     %s|  |\n                   |  |       |  |\n                   |  |%s     %s|  |\n                   |  |   %s   |  |\n                   |  |%s     %s|  |\n                   |   -------   |\n                   |         %s 10|\n                    ------------- \n", c, c, c, c, c, c, c, c, c, c, c, c);
            break;
        case 10:
            sprintf(buf, "                    ------------- \n                   |J%s           |\n                   |   -------   |\n                   |  |%s      |  |\n                   |  |       |  |\n                   |  |       |  |\n                   |  |   J   |  |\n                   |  |       |  |\n                   |  |       |  |\n                   |  |      %s|  |\n                   |   -------   |\n                   |          %s J|\n                    ------------- \n", c, c, c, c);
            break;
        case 11:
            sprintf(buf, "                    ------------- \n                   |Q%s           |\n                   |   -------   |\n                   |  |%s      |  |\n                   |  |       |  |\n                   |  |       |  |\n                   |  |   Q   |  |\n                   |  |       |  |\n                   |  |       |  |\n                   |  |      %s|  |\n                   |   -------   |\n                   |          %s Q|\n                    ------------- \n", c, c, c, c);
            break;
        case 12:
            sprintf(buf, "                    ------------- \n                   |K%s           |\n                   |   -------   |\n                   |  |%s      |  |\n                   |  |       |  |\n                   |  |       |  |\n                   |  |   K   |  |\n                   |  |       |  |\n                   |  |       |  |\n                   |  |      %s|  |\n                   |   -------   |\n                   |          %s K|\n                    ------------- \n", c, c, c, c);
            break;
    }
    return buf;
}

