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
atomic_bool Ttest_server::server_run_flag {false};
atomic_bool Ttest_server::connection_exist_flag {false};
mutex Ttest_server::mtx_run_srv;
mutex Ttest_server::mtx_listen_srv;
mutex Ttest_server::mtx_conn;
mutex Ttest_server::mtx_srv;
condition_variable Ttest_server::cv_srv_close;
condition_variable Ttest_server::cv_conn_close;
condition_variable Ttest_server::cv_srv_listen;


Ttest_server::Ttest_server()
    : 
    server_address_len {sizeof(server_address)},             
    server_accepted_fd {-1}    
{
    lock_guard<mutex> lg(mtx_run_srv);
    server_state = sv_state::SV_NOT_PREPARED;
    connection_state = conn_state::CONN_SV_NOT_READY;
    // define_test_server("127.0.0.1", 8080);
    old_server_ip_address = nullptr;
    server_listen_fd = -1;     

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
    if(!test_server_01_ptr) {
        if(server_run_flag) {
            stop_test_server();  //implicity close_connection()
        }
        delete(test_server_01_ptr);
    }
    test_server_01_ptr = nullptr;
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
    lock_guard<mutex> lg(mtx_run_srv);
    int ret;
    if(server_run_flag) {
        stop_test_server();
    }
    server_ip_address = ip_address;
    server_ip_port = ip_port;
    server_listen_fd = socket(AF_INET, SOCK_STREAM, 0);
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
}

void Ttest_server::run_test_server() 
{        
    lock_guard<mutex> lg(mtx_run_srv);
    int ret;
    // test_server_run_flag = true;    
    server_run_flag = true;
    fprintf(stdout, "\nserver: %s\n", "test server starting");
    ret = listen(server_listen_fd, 1);   //one pending connection only
    if(ret < 0) {
        ERROR_IN("listen");                      
    }
    
    // struct pollfd fds[1];  //member field
    
    while(server_run_flag)
    {
        fprintf(stdout, "\nserver: %s %s:%d ...\n", "wait for connection on", server_ip_address, server_ip_port);
        server_state = sv_state::SV_WAIT_FOR_CONNECTION;
        
        //TODO: 
        // unique_lock<mutex> ul(mtx_listen_srv);
        // cv_srv_listen.wait(ul);

        server_accepted_fd = accept(
            server_listen_fd, 
            (struct sockaddr*)& server_address, 
            &server_address_len);
        if(server_accepted_fd < 0) {
            ERROR_IN("accept");               
        }
        connection_exist_flag = true;
        
        server_state = sv_state::SV_CONNECTED; //client may not notice that, if run_test_server() is in separate thread
        fprintf(stdout, "\nserver: %s\n", "connection accepted");
        
        
        fds[0].fd = server_accepted_fd;
        fds[0].events = POLLIN | POLLOUT | POLLRDHUP;
        
        int old_revents = 0;
        
        // while(server_accepted_fd != -1) {
        while(connection_exist_flag) {
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

        } //while(connection_exist_flag) {
        
        if(close(server_accepted_fd) < 0) 
        {
            ERROR_IN("close: server_accepted_fd");
        }                  
        do {
            //waiting for closing fd and free ip addres/port
            errno = 0;
        } while(fcntl(server_accepted_fd, F_GETFD) != -1 || errno != EBADF);
        server_accepted_fd = -1;
        
        fprintf(stdout, "\nserver: %s\n", "connection closed");

        fprintf(stdout, "\nserver: %s\n", "test server disconnected");
        server_state = sv_state::SV_DISCONNETED;
        
        cv_conn_close.notify_one();

        if(!server_run_flag) {
            break;
        }

    } //while(server_run_flag)
    if(shutdown(server_listen_fd, SHUT_RDWR) == -1) {
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
    }while(fcntl(server_listen_fd, F_GETFD) != -1 || errno != EBADF);
    server_listen_fd = -1;
    server_state = sv_state::SV_STOPPED;

    cv_srv_close.notify_one();
}

void Ttest_server::close_connection()
{   
    unique_lock<mutex> ul(mtx_conn);
    connection_exist_flag = false;
    cv_conn_close.wait(ul, [this]{ return server_accepted_fd == -1;});    
}

void Ttest_server::stop_test_server()
{  
    unique_lock<mutex> ul(mtx_srv);
    server_run_flag = false;  //must be first, because connection accepting loop start again

    if(connection_exist_flag) {
        close_connection();
    }            
    cv_srv_close.wait(ul, [this]{ return server_listen_fd == -1;});
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