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

class server_state;

class Ttest_server
{    
    private:    
    int server_listen_fd;    
    int ret_server;
    // int ret;
    int opt; //for setsockopt    
    struct sockaddr_in server_address;
    socklen_t server_address_len;
    int TIME_WAIT_curr_value;
    const char *old_server_ip_address;
    
    struct pollfd fds[1];

    static Ttest_server* test_server_01_ptr;
    size_t listen_loop_number;

    static std::atomic_bool STATE_sv_defined;
    static std::atomic_bool STATE_sv_def_enable;
    static std::atomic_bool STATE_sv_runs;
    static std::atomic_bool STATE_sv_start_demand;
    static std::atomic_bool STATE_sv_stopped;
    static std::atomic_bool STATE_sv_stop_demand;
    
    static std::atomic_bool STATE_server_monitors_connection_state;
    static std::atomic_bool STATE_monitoring_connection_state_enabled;

    static std::atomic_bool STATE_accepting;
    static std::atomic_bool STATE_accepting_enabled;
    
    
    static std::mutex mtx_srv_run;
    // static std::mutex mtx_srv_def;
    static std::mutex mtx_srv_accept;
    // static std::mutex mtx_conn_close;
    static std::mutex mtx_conn_monitoring;
    
    // static std::condition_variable cv_conn_close;
    static std::condition_variable cv_conn_monitoring_enable;
    static std::condition_variable cv_srv_accept_enable;
    static std::condition_variable cv_srv_run_enable;
    // static std::condition_variable cv_srv_def;

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
    void wait_for_state_neq(std::atomic_bool* state, bool yes);     

    public:
    int server_accepted_fd;
    const char *server_ip_address;
    int server_ip_port;
    
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

    /**
     * @brief define/redefine ip and port of test server
     * bind socket to ip address
     * @param ip_address 
     * @param ip_port 
     */
    void define_test_server(const char *ip_address, const int ip_port);

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


class server_state {
    public:
    enum {
        STATE_sv_defined,
        STATE_sv_def_enable,
        STATE_sv_runs,
        STATE_sv_start_demand,
        STATE_sv_stopped,
        STATE_sv_stop_demand,
        STATE_server_monitors_connection_state,
        STATE_monitoring_connection_state_enabled,
        STATE_accepting,
        STATE_accepting_enabled,
        STATE_COUNT
    };
    static std::atomic_int current_state;
    static std::atomic_int demand_state;

    bool reached() {
        // lock_guard<mutex> lg_c(mtx_sv_curr_state, defer_lock);
        // lock_guard<mutex> lg_d(mtx_sv_demand_state, defer_lock);
        // lock(lg_c, lg_d);
        return current_state == demand_state;
    }
};

#endif //TTEST_SERVER_FOR_ONE_CONNECTION_H