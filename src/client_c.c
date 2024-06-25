/*
Copyright 2024-2024 plapacz6@gmail.com

This file is part of net_comm_dbg.

net_comm_dbg is free software: you can redistribute it and/or modify it under the terms
of the GNU Lesser General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

net_comm_dbg is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
 with net_comm_dbg. If not, see <https://www.gnu.org/licenses/>.
*/

#include "client_c.h"

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <stddef.h>

#include <sys/socket.h>
#include <netinet/in.h> //sockaddr_in
#include <arpa/inet.h>  //htons, inet_pton


#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <stddef.h>
#include <unistd.h> //close

#include <sys/socket.h>
#include <netinet/in.h> //sockaddr_in
#include <arpa/inet.h>  //htons, inet_pton


int connect_to_server(char* server_address, int server_port)
{
    // struct sockaddr sv_address; 
    struct sockaddr_in sv_address;    
    socklen_t sv_address_len = sizeof(sv_address);    
    
    sv_address.sin_family = AF_INET;
    // sv_address.sin_addr.s_addr = inet_addr(server_address);  //not recommended
    if(inet_pton(AF_INET, server_address, &sv_address.sin_addr.s_addr) != 1) {
        perror("client: inet_pton()");
        return -1;
    }
    sv_address.sin_port = htons(server_port);
    
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);    
    if(client_fd < 0) {
        perror("client: socket()");
        return -1;
    }
    int ret = connect(client_fd, (const struct sockaddr*) &sv_address, sv_address_len);
    if(ret < 0) {
        perror("client: connect()");
        return -1;
    }
    return client_fd;
}

int disconnect_from_server(int socket_fd) 
{
    if(close(socket_fd)) {
        perror("client: close()");
        return -1;
    }
    return 0;
}

int send_msg(int socket_fd, char *msg)
{
    return -1;
}


int send_n_msg(int socket_fd, size_t msg_len, char *msg)
{
    return -1;
}


int register_server_answer_callback(
    void* (f)(void*), 
    size_t ans_buff_len, 
    char *answer_buffer)
{
    return -1;
}


