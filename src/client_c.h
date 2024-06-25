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

#ifndef CLIENT_C_H
#define CLIENT_C_H

#ifdef __cplusplus
extern "C" 
{
#endif 

#include <stddef.h>

/**
 * @brief Try to establish connetion to net_comm_dbg server.
 * 
 * @param server_address exampl: "127.0.0.10"
 * @param server_port exampl: 8080
 * @return int 
 * -  socket file descriptor
 * - -1: error
 */
int connect_to_server(char* server_address, int server_port);

/**
 * @brief Close connection with net_comm_dbg server.
 * 
 * @param socket_fd 
 * @return int 
 * -  0: ok
 * - -1: error
 */
int disconnect_from_server(int socket_fd);

#ifdef __cplusplus
}
#endif 
#endif // CLIENT_C_H