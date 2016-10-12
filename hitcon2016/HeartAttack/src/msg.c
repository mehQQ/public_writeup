#include "HeartAttack.h"

/*
 * add msg
 * del msg
 * show log
 * add node
 * del node
 * psend
 * ipc_msg
 * broadcast
 */

void broadcast(int id, char *buf)
{
    int i;
    remove_line(buf);
    for(i=0 ; i<MAX_CLIENT ; i++)
    {
        if(data->player_game[i] == data->player_game[id])
        {
            psend(i, buf, MSG_BROADCAST);
        } 
    }
    usleep(300000);
}

void leave_msg(int sockfd, unsigned int id, unsigned int size)
{
    int end_size = 0x1000;
    struct node *new;
    char buf[MAX_LEN];

    if(id >= MAX_CLIENT || !data->client_valid[id] || id == findid())
    {
        sends(sockfd, "Invalid receiver!\n");
        return;
    }

    if(size < 0x1000 && size > 0)
        end_size = size;

    memset(buf, 0, sizeof(buf));
    new = add_node(end_size);
    sends(sockfd, "Your msg: ");
    read(sockfd, new->str, end_size);
    remove_line(new->str);
    new->str[end_size-1] = '\0';
    new->id = id;
    new->direction = DIR_SEND;
    strncpy(new->name, data->name[id], 15);
    psend(id, new->str, MSG_PRIVATE);
    usleep(100000);
    sprintf(buf, "\033[0;37m[PRIVATE] Told (Player %d) %.15s: ", id, new->name);
    sends(sockfd, buf);
    sends(sockfd, new->str);
    sends(sockfd, "\033[0m\n");
}

void recv_msg(int pid, struct msg *m, int id)
{
    struct node *new;
    int i;
    char buf[MAX_LEN];
    int sockfd = data->sockfd[findid()];

    if(id == -1)
        return;

    for(i=0 ; i<blacknum ; i++)
    {
        if(blacklist[i] == id)
            return;
    }
    
    memset(buf, 0, sizeof(buf));
    if(m->type == MSG_BROADCAST)
    {
        if(id == findid())
            sprintf(buf, "(Player %d) \033[1;33m%.15s\033[0m said: ", id, data->name[id]);
        else
            sprintf(buf, "(Player %d) %.15s said: ", id, data->name[id]);

        sends(sockfd, buf);
        sends(sockfd, m->buf);
        sends(sockfd, "\n");
    }
    else if(m->type == MSG_PRIVATE)
    {
        new = add_node(m->size);
        strcpy(new->str, m->buf);
        strncpy(new->name, data->name[id], 15);
        new->id = id;
        new->direction = DIR_RECV;
        sprintf(buf, "\033[0;37m[PRIVATE] (Player %d) %.15s said: ", id, data->name[id]);
        sends(sockfd, buf);
        sends(sockfd, m->buf);
        sends(sockfd, "\033[0m\n");
    }

}


void del_msg(int sockfd, unsigned int n)
{
    if(del_node(n))
        sends(sockfd, "Delete Success!\n");
    else
        sends(sockfd, "Delete Failed!\n");
}

void show_msg(int sockfd)
{
    int i;
    struct node *current;
    char buf[MAX_LEN];
    if(!head)
    {
        sends(sockfd, "Log is empty!\n");
        return;
    }
    current = head;
    sends(sockfd, "Message Log:\n");
    for(i=0 ; current ; i++, current=current->next)
    {
        if(current->direction == DIR_SEND)
            sprintf(buf, "[%d] Told (Player %d) %.15s : ", i, current->id, current->name);
        else
            sprintf(buf, "[%d] (Player %d) %.15s said : ", i, current->id, current->name);
        sends(sockfd, buf);
        sends(sockfd, current->str);
        sends(sockfd, "\n");
    }
}

struct node *add_node(unsigned int size)
{
    struct node *new;
    new = calloc(1, sizeof(struct node));
    if(!new)
    {
        perror("memory allocation error.\n");
        exit(1);
    }
    new->str = calloc(1, size);
    if(!new->str)
    {
        perror("memory allocation error.\n");
        exit(1);
    }
    new->next = NULL;
    
    if(!head)
        head = new;
    else
    {
        struct node *current;
        current = head;
        while(current->next != NULL)
            current = current->next;
        current->next = new;
    }

    return new;
}

bool del_node(unsigned int n)
{
    struct node *current;
    struct node *prev;
    unsigned int i=0;
    if(!head)
        return false;
    if(n == 0)
    {
        current = head;
        head = head->next;
        free(current->str);
        free(current);
        return true;
    }
    current = head;
    for(i=0 ; i<n-1 ; i++)
    {
        if(current->next)
            current = current->next;
        else
            return false;
    }
    prev = current;
    if((current = current->next) == NULL)
        return false;
    prev->next = current->next;
    free(current->str);
    free(current);
    return true;
}

