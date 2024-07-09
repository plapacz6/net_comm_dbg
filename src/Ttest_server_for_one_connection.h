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
#include <stddef.h>
#include <atomic>
#include <mutex>
#include <condition_variable>

// #include <unistd.h>
#include <netinet/in.h>
// #include <arpa/inet.h>
// #include <sys/types.h>
#include <sys/socket.h>

#include <poll.h>

#include "TSvSt.h"

class server_state;

class Ttest_server
{    
    private:    
    int server_listen_fd;    

    int opt; //for setsockopt    
    struct sockaddr_in server_address;
    socklen_t server_address_len;
    int TIME_WAIT_curr_value;
    const char *old_server_ip_address;
    
    static Ttest_server* test_server_01_ptr;
    size_t listen_loop_number;


    
    static std::mutex mtx_srv_state;

    static std::mutex mtx_srv_run;
    static std::mutex mtx_srv_def;
    static std::mutex mtx_srv_accept;

    static std::mutex mtx_cmd_run;
    static std::mutex mtx_cmd_acc;
    static std::mutex mtx_cmd_Nrun;
    static std::mutex mtx_cmd_Nacc;
    static std::mutex mtx_cmd_Ncon;

    // static std::mutex mtx_conn_close;
    // static std::mutex mtx_conn_monitoring;
    
    // static std::condition_variable cv_conn_close;
    // static std::condition_variable cv_conn_monitoring_enable;

    //for server main loop
    static std::condition_variable cv_srv_accept_enable;
    static std::condition_variable cv_srv_run_enable;    
    static std::condition_variable cv_srv_def;

    //for commands
    static std::condition_variable cv_st_run;
    static std::condition_variable cv_st_acc;
    static std::condition_variable cv_st_Nrun;
    static std::condition_variable cv_st_Nacc;    
    static std::condition_variable cv_st_Ncon;

    /**
     * @brief Construct a new Ttest_server object
     * create socket
     */
    Ttest_server();

    /**
     * @brief Destroy the Ttest_server object
     * close socket
     */
    ~Ttest_server();
    Ttest_server(Ttest_server&) = delete;
    Ttest_server operator=(Ttest_server&) = delete;   
    void wait_for_state_neq(int d, int r, int a, int c);     
    
    /**
     * @brief server start listening
     * 
     */
    void listen_fd_run();

    /**
     * @brief close file descriptor of current accepted connection
     * 
     * @return * close 
     */
    void close_accepted_fd();
    /**
     * @brief accept connection
     * 
     */
    void accepted_fd_connect();

    struct pollfd fds[1];
    int old_revents;           
    void monitor_connection_init();
    void monitor_connection();
    bool monitor_connection_no_change();
    bool check_if_clientd_disconnects();
    void print_connection_state();

    public:
    TSvSt st;  //server state
    int server_accepted_fd;
    const char *server_ip_address;
    int server_ip_port;
    
    static Ttest_server& create_test_server();

    /**
     * @brief define/redefine ip and port of test server
     * bind socket to ip address
     * @param ip_address 
     * @param ip_port 
     */
    void define_test_server(const char *ip_address, const int ip_port);

    /**
     * @brief close server socket
     * 
     */
    void destroy_socket();

    /**
     * @brief start listening on binded socket and start loop of accepting connections
     * The loop runs until stop_test_server() is called, which sets the loop condition 
     * to false, which then closes the server socket and terminates the function.
     * 
     * this method is intended to be worker fuction of detached thread
     */
    void test_server_connection_loop();

    /**
     * @brief start listening loop in test_server_connection_loop()
     * 
     */
    void start_test_server();

    /**
     * @brief break working loop within run_test_server() method
     * by setting booean flag which break the main loop
     * close socket, and give oportunity to redefine ip/port by define_test_server()
     */
    void stop_test_server();

    /**
     * @brief enable accept connection (number of inifity (0)) 
     * and then allow starting work connection monitoring loops
     * @param number - number of loop turn, 0 - inifinity
     */
    void start_accepting_connections(size_t number = 0);
    /**
     * @brief stop inner loop
     * 
     */
    void stop_accepting_connections();
    
    void start_monitoring_connection();
    
    void stop_monitoring_connection();
    
    /**
     * @brief close loop monitoring current connetion and not allow for
     * new connetion by hangin on conditional variable until call start_accepting_connections()
     */
    void close_monitored_connection();    

    bool in_fd_are_set_only(short flags);
    void debug_print_poll_state(bool comment, bool show_empty);
    int read_all_data_from_connection();
    
};

extern FILE *server_log;
/**
 * @brief print information in fomat "\nsource : message\n"
 * 
 * @param source 's' == "server", 'c' == "connection"
 * @param msg message
 */
void printmsg(const char source, const char *msg);
#endif //TTEST_SERVER_FOR_ONE_CONNECTION_H