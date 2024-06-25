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

#include "Ttest_server_for_one_connection.h"

#include <cstdlib>
#include <cstddef>
#include <cstdio>
#include <cerrno>
#include <cassert>
#include <cstdint>
#include <fstream>

#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <poll.h>
#include <fcntl.h>

using namespace std;

#define ERROR_WHERE "test_server: "
#define ERROR_IN(X) do {perror(ERROR_WHERE#X); exit(1);} while(0);


Ttest_server* Ttest_server::test_server_01_ptr {nullptr};

atomic_bool Ttest_server::STATE_sv_defined {false};
atomic_bool Ttest_server::STATE_sv_def_enable {true};
atomic_bool Ttest_server::STATE_sv_runs {false};
atomic_bool Ttest_server::STATE_sv_start_demand {false};
atomic_bool Ttest_server::STATE_sv_stopped {false};
atomic_bool Ttest_server::STATE_sv_stop_demand {false};

atomic_bool Ttest_server::STATE_server_monitors_connection_state {false};
atomic_bool Ttest_server::STATE_monitoring_connection_state_enabled {false};

atomic_bool Ttest_server::STATE_accepting {false};
atomic_bool Ttest_server::STATE_accepting_enabled {false};


mutex Ttest_server::mtx_srv_run;
// mutex Ttest_server::mtx_srv_def;
mutex Ttest_server::mtx_srv_accept;
// mutex Ttest_server::mtx_conn_close;
mutex Ttest_server::mtx_conn_monitoring;

// condition_variable Ttest_server::cv_conn_close;
condition_variable Ttest_server::cv_conn_monitoring_enable;
condition_variable Ttest_server::cv_srv_accept_enable;
condition_variable Ttest_server::cv_srv_run_enable;
// condition_variable Ttest_server::cv_srv_def;





Ttest_server::Ttest_server()
    : 
    server_address_len {sizeof(server_address)},             
    server_accepted_fd {-1}    
{    
    server_state = sv_state::SV_NOT_PREPARED;
    connection_state = conn_state::CONN_SV_NOT_READY;
    
    old_server_ip_address = nullptr;
    server_listen_fd = -1;     


    /* reading out system value of TIME_WIAT for just closed sockets */

    // ifstream proc_sys_net_ipv4_tcp_fin_timeout;
    // proc_sys_net_ipv4_tcp_fin_timeout.open("/proc/sys/net/ipv4/tcp_fin_timeout", ios::in);
    // proc_sys_net_ipv4_tcp_fin_timeout >> TIME_WAIT_curr_value;
    int proc_sys_net_ipv4_tcp_fin_timeout_fd = open("/proc/sys/net/ipv4/tcp_fin_timeout", O_RDONLY);
    if(proc_sys_net_ipv4_tcp_fin_timeout_fd < 0) {
        ERROR_IN("open: proc_sys_net_ipv4_tcp_fin_timeout_fd");
    }    
    constexpr size_t time_wait_buff_size {256};
    char time_wait_buff[time_wait_buff_size];
    int readed = read(proc_sys_net_ipv4_tcp_fin_timeout_fd, time_wait_buff, time_wait_buff_size - 1);
    if(readed < 0) {
        ERROR_IN("read: proc_sys_net_ipv4_tcp_fin_timeout_fd");
    }    
    time_wait_buff[readed] = 0;
    if(close(proc_sys_net_ipv4_tcp_fin_timeout_fd) < 0) {
        ERROR_IN("close: proc_sys_net_ipv4_tcp_fin_timeout_fd");
    }    
    TIME_WAIT_curr_value = atoi(time_wait_buff);
    if(TIME_WAIT_curr_value == 0) {
        TIME_WAIT_curr_value = 60;
    }
}

Ttest_server::~Ttest_server() {
    fprintf(stdout, "\nserver: %s\n", "destructor: in");    
    if(!test_server_01_ptr) {
        if(STATE_sv_runs) {
            stop_test_server();  //implicity close_monitored_connection()
        }
        delete(test_server_01_ptr);
    }
    test_server_01_ptr = nullptr;
    fprintf(stdout, "\nserver: %s\n", "destructor: out");    
}

Ttest_server& Ttest_server::create_test_server()
{
    if(!test_server_01_ptr) {
        test_server_01_ptr = new Ttest_server();
    }
    return *test_server_01_ptr;
}

void Ttest_server::define_test_server(const char *ip_address, const int ip_port)
{       
    /* logic to guarantee correct use */    
    if(!STATE_sv_def_enable) {
        fprintf(stdout, "\nserver: %s\n", "I can't define server when it runs. call stop_test_server().");    
        return;
    }
    STATE_sv_defined = false;

    /*  defining addres ip and port */
    int ret;
    server_ip_address = ip_address;
    server_ip_port = ip_port;
    server_listen_fd = socket(AF_INET, SOCK_STREAM, 0); //ip, TCP
    if(server_listen_fd < 0) {
        ERROR_IN("socket");            
    }
    ret = setsockopt(server_listen_fd, SOL_SOCKET, 
        SO_REUSEADDR | SO_REUSEPORT,    
        &opt, sizeof(opt));
    if(ret < 0) 
    {
        ERROR_IN("setsockopt");            
    }
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr(server_ip_address);
    server_address.sin_port = htons(server_ip_port);

    /* previous ip addres may be uaccesible during some short time */
    if(old_server_ip_address == server_ip_address) {
        fprintf(stdout, "\nserver: %s%d%s\n", "waiting ", TIME_WAIT_curr_value + 1, " sec for the socket's TIME_WAIT to pass");    
        sleep(TIME_WAIT_curr_value + 1);
    }

    ret = bind(
        server_listen_fd, 
        (struct sockaddr*)&server_address, 
        sizeof(struct sockaddr));
    if(ret < 0)
    {
        fprintf(stderr, "server_listen_fd : %d\n", server_listen_fd);
        ERROR_IN("bind");            
    }
    fprintf(stdout, "\nserver: %s\n", "test server defined");    
    old_server_ip_address = server_ip_address;    

    STATE_sv_defined = true;
}

void Ttest_server::test_server_connection_loop()
{       
    /* logic to guarantee correct use*/
    if(!STATE_sv_defined) {
        fprintf(stdout, "\nserver: %s\n", "ip adrres not defined. First call define_test_server().");    
        fflush(stdout);
        return;
    }
    // if() {
    //     fprintf(stdout, "\nserver: %s\n", "Only one instance of runnign function 'start_test_server()' is allowed. First call stop_test_server().");    
    //     fflush(stdout);
    //     return;
    // }    

    //TODO cv_run_enable.wait(...)

    STATE_sv_def_enable = false;    

    int ret;    
    fprintf(stdout, "\nserver: %s\n", "test server starting");
       
    /* working server loop (after create bind ip addres)*/
    unique_lock<mutex> ulrs(mtx_srv_run);
    cv_srv_run_enable.wait(ulrs, [& ]{return STATE_sv_start_demand == true;});
    if(STATE_sv_start_demand) {
        STATE_sv_runs = true;

        fprintf(stdout, "\nserver: %s\n", "start listening");            
        ret = listen(server_listen_fd, 1);   //one pending connection only
        if(ret < 0) {
            ERROR_IN("listen");                      
        }

        while(!STATE_sv_stop_demand)
        {   
            fprintf(stdout, "\nserver: %s\n", "waiting for accepting enable");                      
            /* connection accepting loop */
            unique_lock<mutex> ula(mtx_srv_accept);
            cv_srv_accept_enable.wait(ula, [&]{return (
                STATE_accepting_enabled == true ||
                STATE_sv_stop_demand == true
            );});
            fprintf(stdout, "\nserver: %s\n", "'accepting enable' issued'");            
            if(STATE_accepting_enabled) 
            {
                STATE_accepting = true;
                fprintf(stdout, "\nserver: %s\n", "accepting enable");            

                /* connection accepting loop - allowed amount */
                for(size_t listen_loop_counter = listen_loop_number; 
                    1;
                    // listen_loop_number ? 1 : listen_loop_counter;                
                    // [&] {if(! listen_loop_number) --listen_loop_counter;}) 
                    listen_loop_number ? : --listen_loop_counter)     
                // while(1)           
                {
                    if(!STATE_accepting_enabled) {
                        break;
                    }
                    fprintf(stdout, "\nserver: %s %s:%d ...\n", "wait for connection on", server_ip_address, server_ip_port);
                    server_state = sv_state::SV_WAIT_FOR_CONNECTION;

                    server_accepted_fd = accept(
                        server_listen_fd, 
                        (struct sockaddr*)& server_address, 
                        &server_address_len);
                    if(server_accepted_fd < 0) {
                        ERROR_IN("accept");               
                    }
                    
                    server_state = sv_state::SV_CONNECTED; //client may not notice that, if start_test_server() is in separate thread
                    if(listen_loop_number) {
                        fprintf(stdout, "\nserver: %s %zu/%zu %s\n", "connection", 
                            (listen_loop_number - listen_loop_counter), listen_loop_number, "accepted");
                    }
                    else {
                        fprintf(stdout, "\nserver: %s %s\n", "connection", "accepted");
                    }
                                    
                    fds[0].fd = server_accepted_fd;
                    fds[0].events = POLLIN | POLLOUT | POLLRDHUP;                
                    int old_revents = 0;                
                    
                    /* monitoring connetion state */    
                    unique_lock<mutex> ulm(mtx_conn_monitoring);
                    cv_conn_monitoring_enable.wait(
                        ulm, [&]{return STATE_monitoring_connection_state_enabled == true;});
                    if(STATE_monitoring_connection_state_enabled) {
                        fprintf(stdout, "\nserver: %s\n", "connection monitoring start");
                        STATE_server_monitors_connection_state = true;
                        while(STATE_monitoring_connection_state_enabled) {
                            // struct pollfd fds[1];  //<- member field
                            old_revents = fds[0].revents;
                            fds[0].revents = 0;
                            int poll_ret = poll(fds, 1, 1000); 
                            // int poll_ret = poll(fds, 1, -1); 
                            assert(poll_ret > 0);
                            if(old_revents == fds[0].revents) {
                                continue;
                            }            
                            /* f(comment, not comment for empty position)*/
                            debug_print_poll_state(true, false);
                            
                            //TODO:
                            //ioclt()  - check if server_accepted_fd still open
                            // if(in_fd_are_set_only(POLLRDHUP))

                        } //while(STATE_monitoring_connection_state) {                    
                    } //if
                    STATE_server_monitors_connection_state = false;
                    if(server_accepted_fd != -1) {
                        close_monitored_connection();
                    }
                    fprintf(stdout, "\nserver: %s\n", "connection monitoring stopped");
                    
                } //for(listen_loop_counter)
            
            }//if(STATE_accepting_enabled);
            STATE_accepting = false;
            fprintf(stdout, "\nserver: %s\n", "accepting connection stopped");            
            
        } //while(!STATE_sv_stop_demand)    
        fprintf(stdout, "\nserver: %s\n", "run loop: finished");
        fflush(stdout);                    
    }//if
    fprintf(stdout, "\nserver: %s\n", "run: finished");
    fflush(stdout);            

    if(shutdown(server_listen_fd, SHUT_RDWR) == -1) {  //debug
        ERROR_IN("shutdown: server_ip_addres, SHUT_RDWR");
    }    
    if(close(server_listen_fd) < 0){
        perror("test_server: close(server_listen_fd)");
        exit(1);
    }   
    fprintf(stdout, "\nserver: %s\n", "test server stopped");
    fflush(stdout);            

    do {
        //waiting for closing fd and free ip addres/port
        errno = 0;
    } while(fcntl(server_listen_fd, F_GETFD) != -1 || errno != EBADF);
    server_listen_fd = -1;
    server_state = sv_state::SV_STOPPED;
        
    STATE_sv_runs = false;
    STATE_sv_def_enable = true;     
    fprintf(stdout, "\nserver: %s\n", "test_server_connection_loop(): finised");
    fflush(stdout);
    return;
}

void Ttest_server::wait_for_state_neq(atomic_bool* state, bool yes)
{
        fprintf(stdout, "%c%p", '\n', static_cast<void*>(state));
        while(yes ? *state == true : ! *state == true){
            sleep(1);
        fprintf(stdout, "%c", '.');
        fflush(stdout);
        }
        fprintf(stdout, "%c", '\n');
}



void Ttest_server::start_test_server() 
{
    fprintf(stdout, "\ncommand: %s\n", "start_test_server");
    if(STATE_sv_defined && !STATE_sv_runs) {
        STATE_sv_start_demand = true;
        STATE_sv_stop_demand = false;
        fprintf(stdout, "\ncommand: %s\n", "start_test_server: issued");
        cv_srv_run_enable.notify_one();
        wait_for_state_neq(&STATE_sv_runs, false);        
    }    
}

void Ttest_server::stop_test_server()
{  
    fprintf(stdout, "\ncommand: %s\n", "stop_test_server");           
    if(STATE_sv_runs) {
        STATE_sv_start_demand = false;  
        STATE_sv_stop_demand = true;   
        fprintf(stdout, "\ncommand: %s\n", "stop_test_server: issued");           
        stop_accepting_connections();  
        stop_monitoring_connection();        
        // wait_for_state_neq(&STATE_sv_runs, true);        
        cv_srv_accept_enable.notify_one();
    }    
}   

void Ttest_server::start_accepting_connections(size_t number)
{    
    fprintf(stdout, "\ncommand: %s\n", "start_accepting_connections");
    if(STATE_sv_runs && !STATE_accepting) {
                
        unique_lock<mutex> ula(mtx_srv_accept, defer_lock);
        ula.lock();
        listen_loop_number = number;
        STATE_accepting_enabled = true;   
        ula.unlock();
        fprintf(stdout, "\ncommand: %s\n", "start_accepting_connections: issued");
        cv_srv_accept_enable.notify_one();
        wait_for_state_neq(&STATE_accepting, false);
    }    
}

void Ttest_server::stop_accepting_connections()
{
    fprintf(stdout, "\ncommand: %s\n", "stop_accepting_connections");
    if(
        STATE_accepting && 
        STATE_accepting_enabled) 
    {
        STATE_accepting_enabled = false;
        fprintf(stdout, "\ncommand: %s\n", "stop_accepting_connections: issued");
        // wait_for_state_neq(&STATE_accepting, true);
    }
}

void Ttest_server::start_monitoring_connection() {
    fprintf(stdout, "\ncommand: %s\n", "start_monitoring_connection");
    if(STATE_sv_runs && STATE_accepting && 
        !STATE_server_monitors_connection_state) 
    {
        unique_lock<mutex> ulm(mtx_conn_monitoring, defer_lock);
        ulm.lock();
        STATE_monitoring_connection_state_enabled = true;
        ulm.unlock();
        fprintf(stdout, "\ncommand: %s\n", "start_monitoring_connection: issued");
        cv_conn_monitoring_enable.notify_one();
        wait_for_state_neq(&STATE_server_monitors_connection_state, false);
    }        
}

void Ttest_server::stop_monitoring_connection() {
    fprintf(stdout, "\ncommand: %s\n", "stop_monitoring_connection");
    if(            
        STATE_server_monitors_connection_state &&
        STATE_monitoring_connection_state_enabled) 
    {
        STATE_monitoring_connection_state_enabled = false;
        fprintf(stdout, "\ncommand: %s\n", "stop_monitoring_connection: issued");
        // wait_for_state_neq(&STATE_server_monitors_connection_state, true);
    }        
}

void Ttest_server::close_monitored_connection()
{   
    fprintf(stdout, "\ncommand: %s\n", "close_monitored_connection");
    if(STATE_sv_runs && server_accepted_fd != -1) {    
        fprintf(stdout, "\ncommand: %s\n", "close_monitored_connection: execution");        
        /* shutting down the monitored connection at the server's initiative */
        if(shutdown(server_accepted_fd, SHUT_RDWR ) < 0){
            ERROR_IN("shutdown: server_accepted_fd");
        }
        if(close(server_accepted_fd) < 0) 
        {
            ERROR_IN("close: server_accepted_fd");
        }                  
        do {
            //waiting for closing fd and free ip addres/port
            errno = 0;
        } while(fcntl(server_accepted_fd, F_GETFD) != -1 || errno != EBADF);
        server_accepted_fd = -1;
        fprintf(stdout, "\nserver: %s\n", "accepted connection closed");
        fprintf(stdout, "\nserver: %s\n", "test server disconnected");
        server_state = sv_state::SV_DISCONNETED;
    }
}

bool Ttest_server::in_fd_are_set_only(short flags) {
    return fds[0].revents & flags;
}

void Ttest_server::debug_print_poll_state(bool comment, bool show_empty) {

    #define PRINT_POLL(X, COMMENT)\
    do {\
        if((show_empty ? true : (fds[0].revents & X ))) {\
            fprintf(stdout, "[%s]", fds[0].revents & X ? #X : "");\
            if(comment) {\
                fprintf(stdout, " - {%p} %s\n", (void*) X , COMMENT );\
            }\
        }\
    } while(false);

    /*
    grep -rnw '/usr/inclue' -e 'POLLIN'    
    */
    bool comment_backup = comment;
    bool show_empty_backup = show_empty;
    comment = false;
    show_empty = true;
    size_t two_times = 2;
    do {
        // printing one line
        fprintf(stdout, "%s\n", "");
        //POSIX
        PRINT_POLL(POLLIN, "** There is data to read.  **");
        PRINT_POLL(POLLPRI, "** There is urgent data to read.  **");
        PRINT_POLL(POLLOUT, "** Writing now will not block.  **");

        // __USE_XOPEN || __USE_XOPEN2K8 (def in XPG4.2)
        PRINT_POLL(POLLRDNORM, "*XOpen: Normal data may be read. *");                    
        PRINT_POLL(POLLRDBAND, "*XOpen: Priority data may be read. *");                    
        PRINT_POLL(POLLWRNORM, "*XOpen: Writing now will not block. *");                    
        PRINT_POLL(POLLWRBAND, "*XOpen: Priority data may be written. *");                    

        // __USE_GNU
        PRINT_POLL(POLLMSG, "*linux: lack of comment*");                    
        PRINT_POLL(POLLREMOVE, "*linux: lack of comment*"); 
        PRINT_POLL(POLLRDHUP, "* linux(>=2.6.17): peer closed connection to RD *");
        
        //POSIX
        PRINT_POLL(POLLERR, "** Error condition.  **");
        PRINT_POLL(POLLHUP, "** Hung up.  **");                        
        PRINT_POLL(POLLNVAL, "** Invalid polling request.  **");
        fprintf(stdout, "%s\n", "");

        // printing detailed information
        if(comment_backup) {        
            comment = comment_backup;
            show_empty = show_empty_backup;
        }
        else {
            break;
        }
    }
    while(--two_times);
    fflush(stdout);
    #undef PRINT_POLL            
}

int Ttest_server::read_all_data_from_connection()
{
    return -1;    
    /*
    
    TODO
    
    */
    enum {BUFF_LEN = 1024};
    char buff[BUFF_LEN];
    int readval = 0;

    //TODO: ioctrl()
    
    readval = recv(server_accepted_fd, buff, sizeof(buff), MSG_DONTWAIT);
    if(readval == 0) {  //peer close connection probably
            fprintf(stdout, "%s\n", "\nserver: client probably close connection");            
            //TODO: print connection state            
    }
    else if(readval != -1) {
        buff[readval] = '\0';
        fprintf(stdout, "\nserver: 'client wrote:[ %s ]'\n", buff);            
    }
    else {
        //AGAIN  EAGAIN
        fprintf(stdout, "%s\n", "\nserver: client not ready to read EAGAIN");
    }    
    fprintf(stdout, "\nserver: %s\n", "all data readed");
    return 0;
}    


#undef ERROR_WHERE
#undef ERROR_IN