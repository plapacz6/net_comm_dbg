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

#ifndef TTEST_SERVER_FOR_ONE_CONNECTION_H
#define TTEST_SERVER_FOR_ONE_CONNECTION_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

// #include <cstdlib>
#include <cstddef>
#include <atomic>

// #include <unistd.h>
#include <netinet/in.h>
// #include <arpa/inet.h>
// #include <sys/types.h>
#include <sys/socket.h>

#include <poll.h>

class Ttest_server
{    
    private:    
    int server_listen_fd;    
    int ret_server;
    int ret;
    int opt; //for setsockopt    
    struct sockaddr_in server_address;
    socklen_t server_address_len;
    
    struct pollfd fds[1];

    static Ttest_server* test_server_01_ptr;

    static std::atomic_bool server_run_flag;
    static std::atomic_bool connection_exist_flag;

    Ttest_server();
    ~Ttest_server();
    Ttest_server(Ttest_server&) = delete;
    Ttest_server operator=(Ttest_server&) = delete;        

    public:
    int server_accepted_fd;
    const char *server_ip_address;
    int server_port;
    
    /*
    State indication not work well when 'run_test_server()' is in another thread than
    client, because client can write/read immedietly after accept, but server/connection state
    is changed in next line of code.
    */
    enum class sv_state {
        SV_NOT_PREPARED,
        SV_WAIT_FOR_CONNECTION,
        SV_CONNECTED,
        SV_DISCONNETED,
        SV_STOPPED
    };
    enum class conn_state {
        CONN_SV_NOT_READY,
        CONN_UNKNOWN
    };
    sv_state server_state;
    conn_state connection_state;

    static Ttest_server& create_test_server();
    void define_test_server(const char *ip_address, const int ip_port);
    void run_test_server();
    void close_connection();    
    void stop_test_server();
    bool in_fd_are_set_only(short flags);
    void debug_print_poll_state(bool comment, bool show_empty);
    int read_all_data_from_connection();
    
};
#endif //TTEST_SERVER_FOR_ONE_CONNECTION_H