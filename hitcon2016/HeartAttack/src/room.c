#include "HeartAttack.h"

/*
 * list_room
 * list_player
 * create
 * join
 * leave
 * exit 
 * end_game
 * start
 * whoami
 * ban
*/


void ban(int sockfd, int id, unsigned int tid)
{
    if(tid >= MAX_CLIENT || !data->client_valid[tid] || tid == id)
    {
        sends(sockfd, "Invalid ID!\n");
        return;
    }
    unsigned int i;
    for(i=0 ; i<blacknum ; i++)
    {
        if(blacklist[i] == tid)
        {
            sends(sockfd, "You have already banned this ID!\n");
            return;
        }
    }
    unsigned int *new = (unsigned int*)realloc(blacklist, sizeof(unsigned int)*(blacknum+1));
    if(!new)
    {
        perror("Realloc error.\n");
        return;
    }
    blacklist = new;
    blacklist[blacknum++] = tid;

    sends(sockfd, "Success!\n");
}



void whoami(int sockfd, int id)
{
    char buf[MAX_LEN];    
    sprintf(buf, "------------------------------------------------------------------------------\n    ID                    Name                Room\n------------------------------------------------------------------------------\n");

    sends(sockfd, buf);
    if(data->player_game[id] != -1)
        sprintf(buf, " Player %3.3d        %-20.20s        %d\n\n", id, data->name[id], data->player_game[id]);
    else
        sprintf(buf, " Player %3.3d        %-20.20s        N\n\n", id, data->name[id]);

    sends(sockfd, buf);
}

void list_room(int sockfd)
{
    int i;
    char buf[MAX_LEN*2];
    sends(sockfd, "------------------------------------------------------------------------------\n    ID           Host                           Players            State\n------------------------------------------------------------------------------\n");
    for(i=0 ; i<MAX_GAME ; i++)
    {
        if(data->game_valid[i])
        {
            sprintf(buf, " Room %2.2d          %-20.20s         %d players          ", i, data->name[data->game_host[i]], data->game_sum[i]);
            if(data->game_start[i])
                sprintf(buf, "%sPlaying\n\n", buf);
            else 
                sprintf(buf, "%sWaiting\n\n", buf);
            sends(sockfd, buf);
        }
    }
}

void list_player(int sockfd)
{
    int i;
    char buf[MAX_LEN*2];
    sends(sockfd, "------------------------------------------------------------------------------\n    ID           Name                            Room\n------------------------------------------------------------------------------\n");
    for(i=0 ; i<MAX_CLIENT ; i++)
    {
        if(data->client_valid[i])
        {
            if(data->player_game[i] != -1)
                sprintf(buf, " Player %3.3d       %-20.20s            %d\n\n", i, data->name[i], data->player_game[i]);
            else
                sprintf(buf, " Player %3.3d       %-20.20s            N\n\n", i, data->name[i]);

            sends(sockfd, buf);
        }
    }
}

void create(int sockfd, int id)
{
    int i;
    char buf[MAX_LEN];

    if(data->player_game[id] != -1)
    {
        sends(sockfd, "You are already in a room!\n");
        return;
    }

    for(i=0 ; i<MAX_GAME ; i++)
    {
        if(!data->game_valid[i])
        {
            while(pthread_mutex_trylock(&data->game_lock[i]))
                usleep(10);
            data->game_valid[i] = true;
            data->game_start[i] = false;
            data->game_host[i] = id;
            data->game_sum[i] = 1;
            data->player_game[id] = i;
            data->uid[i][0] = id;
            sprintf(buf, "Create Success! Your room number is %d!\n", i);
            sends(sockfd, buf);
            pthread_mutex_unlock(&data->game_lock[i]);
            return;
        }
    }
    sprintf(buf, "Sorry, there are too many rooms. We can only afford %d rooms.\n", MAX_GAME);
    sends(sockfd, buf);
}

void join(int sockfd, int id, unsigned int gid)
{
    int i;
    char buf[MAX_LEN];

    if(data->player_game[id] != -1)
    {
        sends(sockfd, "You are already in a room!\n");
        return;
    }

    if(gid < MAX_GAME && data->game_valid[gid])
    {
        if(data->game_sum[gid] == 4)
        {
            sends(sockfd, "This room is full! A game is limited to four players.\n");
            return;
        }
        if(data->game_start[gid])
        {
            sends(sockfd, "This room has started to play!\n");
            return;
        }
        while(pthread_mutex_trylock(&data->game_lock[gid]))
            usleep(10);
        data->uid[gid][data->game_sum[gid]] = id;
        data->game_sum[gid]++;
        data->player_game[id] = gid;
        pthread_mutex_unlock(&data->game_lock[gid]);
        sprintf(buf, "Player %.20s has joined room %d. Room %d has %d players now.\n", data->name[id], gid, gid, data->game_sum[gid]);
        for(i=0 ; i<MAX_CLIENT ; i++)
        {
            if(data->client_valid[i] && (data->player_game[i] == gid))
            {
                psend(i, buf, MSG_SYSTEM);
            }
        }
    }
    else
        sends(sockfd, "This room doesn't exist!\n");
}

bool player_leave(int sockfd, int id)
{
    unsigned int gid;
    int i;
    if(data->player_game[id] == -1)
    {
        sends(sockfd, "You are not in a room.\n");
        return false;
    }
    gid = data->player_game[id];
    if(data->game_start[data->player_game[id]])
    {
        sends(sockfd, "Game has already start!\n");
        return false;
    }
    leave(id, gid);
    return true;
}

void player_exit(int sockfd, int id)
{
    if(!(data->player_game[id] == -1))
        if(!player_leave(sockfd, id))
            return;
    data->client_valid[id] = false;
    close(sockfd);
    closesocket();
    exit(0);
}

void leave(int id, int gid)
{
    int i;
    char buf[MAX_LEN];
    if(!data->game_valid[gid])
        return;
    // host leaving or game interrupt, close
    if(data->game_host[gid] == id || data->game_start[gid])
    {
        if(data->game_host[gid] == id)
            sprintf(buf, "The host %.20s has leaved. Room %d has been closed.\nBack to lobby ...\n\n", data->name[id], gid);
        else
            sprintf(buf, "The game has been terminated due to Player %.20s's leave.\nBack to lobby ...\n\n", data->name[id]);
        for(i=0 ; i<MAX_CLIENT ; i++)
        {
            if(data->client_valid[i] && (data->player_game[i] == gid))
            {
                psend(i, buf, MSG_SYSTEM);
                data->is_on[i] = true;
            }
        
        }
        end_game(gid);
    }
    else
    {
        data->player_game[id] = -1;
        for(i=0 ; i<data->game_sum[gid] ; i++)
            if(data->uid[gid][i] == id)
                break;
        for(i ; i<data->game_sum[gid]-1 ; i++)
            data->uid[gid][i] = data->uid[gid][i+1];
        data->game_sum[gid]--;

        sprintf(buf, "Player %.20s has leaved room %d. Room %d has %d players now.\n", data->name[id], gid, gid, data->game_sum[gid]);
        for(i=0 ; i<MAX_CLIENT ; i++)
        {
            if(data->client_valid[i] && (data->player_game[i] == gid))
            {
                psend(i, buf, MSG_SYSTEM);
            }
        }
    }

}

void end_game(int gid)
{
    int i;
    while(pthread_mutex_trylock(&data->game_lock[gid]))
        usleep(10);
    for(i=0 ; i<MAX_CLIENT ; i++)
    {
        if(data->player_game[i] == gid)
            data->player_game[i] = -1;
    }
    data->game_valid[gid] = false;
    pthread_mutex_unlock(&data->game_lock[gid]);
}

