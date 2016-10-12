#include "HeartAttack.h"

void opensocket(){
	struct sockaddr_un address;

	if ((u_sockfd = socket(PF_UNIX, SOCK_DGRAM, 0)) < 0) {
		perror("socket failed\n");
		exit(1);
	}

	address.sun_family = AF_UNIX;
	sprintf(address.sun_path, SOCKET_PATH, getpid());

	unlink(address.sun_path);

	if (bind(u_sockfd, (struct sockaddr*)&address, sizeof(address))==-1){
		__printf_chk(1, "bind socket failed: %s\n", address.sun_path);
		exit(1);
	}
}

void closesocket(){
	char buf[MAX_LEN];

	close(u_sockfd);
	sprintf(buf, SOCKET_PATH, getpid());
	unlink(buf);
}

void psend(int id, char *buf, msgtype type)
{
    if(!data->client_valid[id])
        return;
    if(!data->is_on[id] && type != MSG_SYSTEM)
        return;
    
    struct sockaddr_un addr;
    struct msg m;
    memset(&m, 0, sizeof(m));
    m.type = type;
    m.size = strlen(buf);
    strcpy(m.buf, buf);
    
    addr.sun_family = AF_UNIX;
    sprintf(addr.sun_path, SOCKET_PATH, data->pid[id]);
    sendto(u_sockfd, &m, sizeof(m), 0, (struct sockaddr*) &addr, sizeof(addr));

}

void ipc_msg(int id)
{
    if(id < 0)
        return;
    if(!data->client_valid[id])
        return;

    struct sockaddr_un addr;
    struct msg m;
    socklen_t len = sizeof(addr);
    pid_t pid;

    memset(&m, 0, sizeof(m));
    recvfrom(u_sockfd, &m, sizeof(m), 0, (struct sockaddr*) &addr, &len);    
    sscanf(addr.sun_path, SOCKET_PATH, &pid);
    switch(m.type){
        case MSG_SYSTEM:
            sends(data->sockfd[id], m.buf);
            break;
        case MSG_BROADCAST:
        case MSG_PRIVATE:
            recv_msg(pid, &m, pid2id(pid));
            break;
    }

}


