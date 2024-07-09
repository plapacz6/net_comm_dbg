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

#define MUTEX_TRACE
#ifdef MUTEX_TRACE
#define LOCK_UNLOCK(MTX, COMMANDS) do {\
                         std::cout << __LINE__ << ": ul" << #MTX << ".lock()" << std::endl;\
                         ulst_##MTX.lock();\
                         COMMANDS;\
                         ulst_##MTX.unlock();} while(0);\
                         std::cout << __LINE__ << ": ul" << #MTX << ".unlock()" << std::endl;

#define SET_CURR_ST(A, B, C, D) do {\
                         std::cout << __LINE__ << ": ul" << "c" << ".lock()" << std::endl;\
                         ulst_c.lock();\
                         st.current.set(A, B, C, D);\
                         ulst_c.unlock();} while(0);\
                         std::cout << __LINE__ << ": ul" << "c" << ".unlock()" << std::endl;
#else                                                  
#define LOCK_UNLOCK(MTX, COMMANDS) do {\
                         ulst_##MTX.lock();\
                         COMMANDS;\
                         ulst_##MTX.unlock();} while(0);

#define SET_CURR_ST(A, B, C, D) do {\                         
                         ulst_c.lock();\
                         st.current.set(A, B, C, D);\
                         ulst_c.unlock();} while(0);
#endif

#ifdef MUTEX_TRACE
#define ulst_c_lock()   std::cout << "lock c: " << __LINE__ << std::endl; ulst_c.lock()
#define ulst_d_lock()   std::cout << "lock d: " << __LINE__ << std::endl; ulst_d.lock()
#define ulst_c_unlock()   std::cout << "unlock c: " << __LINE__ << std::endl; ulst_c.unlock()
#define ulst_d_unlock()   std::cout << "unlock d: " << __LINE__ << std::endl; ulst_d.unlock()
#else
#define ulst_c_lock()    ulst_c.lock()
#define ulst_d_lock()    ulst_d.lock()
#define ulst_c_unlock()  ulst_c.unlock()
#define ulst_d_unlock()  ulst_d.unlock()
#endif

Ttest_server* Ttest_server::test_server_01_ptr {nullptr};


mutex Ttest_server::mtx_srv_state;

mutex Ttest_server::mtx_srv_run;
mutex Ttest_server::mtx_srv_def;
mutex Ttest_server::mtx_srv_accept;
// mutex Ttest_server::mtx_conn_close;
// mutex Ttest_server::mtx_conn_monitoring;

mutex Ttest_server::mtx_cmd_run;
mutex Ttest_server::mtx_cmd_acc;
mutex Ttest_server::mtx_cmd_Nrun;
mutex Ttest_server::mtx_cmd_Nacc;
mutex Ttest_server::mtx_cmd_Ncon;

// condition_variable Ttest_server::cv_conn_close;
// condition_variable Ttest_server::cv_conn_monitoring_enable;
condition_variable Ttest_server::cv_srv_accept_enable;
condition_variable Ttest_server::cv_srv_run_enable;
condition_variable Ttest_server::cv_srv_def;

condition_variable Ttest_server::cv_st_run;
condition_variable Ttest_server::cv_st_acc;
condition_variable Ttest_server::cv_st_Nrun;
condition_variable Ttest_server::cv_st_Nacc;    
condition_variable Ttest_server::cv_st_Ncon;





Ttest_server::Ttest_server()
    : 
    server_address_len {sizeof(server_address)},             
    server_accepted_fd {-1}    
{       
    // printmsg(', "in");  //for global not may work because server_log not initialized

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

    {
        bool c_st;
        bool d_st;
        unique_lock<mutex> ulst_c(st.mtx_curr, defer_lock);
        unique_lock<mutex> ulst_d(st.mtx_dem, defer_lock);        
        cout << "Ttest_server() : lock" << endl;
        lock(ulst_c, ulst_d);
        st.current.set(0, 0, 0, 0);
        st.demand.set(3, 3, 3, 3);
        cout << "Ttest_server() : unlock" << endl;
        ulst_c.unlock();
        ulst_d.unlock();        
    }    
    // printmsg('0', "out");
}

Ttest_server::~Ttest_server() {    
    printmsg('1', "in");    

    bool c_st;
    bool d_st;
    unique_lock<mutex> ulst_c(st.mtx_curr, defer_lock);
    unique_lock<mutex> ulst_d(st.mtx_dem, defer_lock);

    LOCK_UNLOCK(c, 
        c_st = st.current.eq(2, 2, 2, 1); //current: con
    );
    close_monitored_connection();
    do {
        LOCK_UNLOCK(c,
            c_st = st.current.eq(2, 2, 2, 0); //current: !con
        );
        sleep(1);
    } while(!c_st);
    LOCK_UNLOCK(c, 
        c_st = st.current.eq(2, 2, 1, 2); //current: acc
    );    
    do {
        LOCK_UNLOCK(c,
            c_st = st.current.eq(2, 2, 2, 0); //current: !acc
        );
        sleep(1);
    } while(!c_st);    
    stop_accepting_connections();
    LOCK_UNLOCK(c, 
        c_st = st.current.eq(2, 1, 2, 2); //current : run
    );    
    do {
        LOCK_UNLOCK(c,
            c_st = st.current.eq(2, 2, 2, 0); //current: !run            
        );
        sleep(1);
    } while(!c_st);    
    stop_test_server();

    {
        cout << "~Ttest_server() : lock" << endl;
        lock(ulst_c, ulst_d);
        st.current.set(0, 0, 0, 0);
        st.demand.set(3, 3, 3, 3);
        cout << "~Ttest_server() : unlock" << endl;
        ulst_c.unlock();
        ulst_d.unlock();
    }        
    if(server_listen_fd != -1) {
        destroy_socket();
    }

    printmsg('1', "out");        
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
    bool c_st;
    bool d_st;
    unique_lock<mutex> ulst_c(st.mtx_curr, defer_lock);
    unique_lock<mutex> ulst_d(st.mtx_dem, defer_lock);
    
    printmsg('d', "define_test_server");

    /* logic to guarantee correct use */    

    LOCK_UNLOCK(c, 
        c_st = st.current.eq(2, 0, 2, 2); //if: !run
    );
    
    if(!c_st) {
        printmsg('d', "I can't define server when it runs. call stop_test_server().");    
        return;
    }
        
    SET_CURR_ST(0, 2, 2, 2); //current set: !def    
    
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
        fprintf(server_log, "\ndefine: %s%d%s\n", "waiting ", TIME_WAIT_curr_value + 1, " sec for the socket's TIME_WAIT to pass");    
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
    printmsg('d', "test server defined");    
    old_server_ip_address = server_ip_address;    
    
    SET_CURR_ST(1, 2, 2, 2);        
    cv_srv_def.notify_one();
}

void Ttest_server::listen_fd_run() {
    int ret = listen(server_listen_fd, 1);   //one pending connection only
    if(ret < 0) {
        ERROR_IN("listen");                      
    }
}

void Ttest_server::monitor_connection_init() {                
    fds[0].fd = server_accepted_fd;
    fds[0].events = POLLIN | POLLOUT | POLLRDHUP;                
    old_revents = 0;               
}

void Ttest_server::monitor_connection() {
    // struct pollfd fds[1];  //<- member field
    old_revents = fds[0].revents;
    fds[0].revents = 0;
    int poll_ret = poll(fds, 1, 1000); 
    // int poll_ret = poll(fds, 1, -1); 
    assert(poll_ret > 0);
}

bool Ttest_server::monitor_connection_no_change() {
    return (old_revents == fds[0].revents);
}

bool Ttest_server::check_if_clientd_disconnects() {
    //TODO:
    //loopup RD, WR data

    //ioclt()  - check if server_accepted_fd still open
    // if(in_fd_are_set_only(POLLRDHUP))
    return false;
}

void Ttest_server::accepted_fd_connect() {
    server_accepted_fd = accept(
        server_listen_fd, 
        (struct sockaddr*)& server_address, 
        &server_address_len);
    if(server_accepted_fd < 0) {
        ERROR_IN("accept");               
    }
}

void Ttest_server::close_accepted_fd() {
    if(shutdown(server_accepted_fd, SHUT_RDWR ) < 0) {
        ERROR_IN("shutdown: server_accepted_fd");
    }
    if(close(server_accepted_fd) < 0) {
        ERROR_IN("close: server_accepted_fd");
    }                  
    do {
        //waiting for closing fd and free ip addres/port
        errno = 0;
    } while(fcntl(server_accepted_fd, F_GETFD) != -1 || errno != EBADF);                                        
    server_accepted_fd = -1;
}

void Ttest_server::destroy_socket() {
    printmsg('s', "closing socket...");
    
    if(shutdown(server_listen_fd, SHUT_RDWR) == -1) {  //debug
        ERROR_IN("shutdown: server_ip_addres, SHUT_RDWR");
    }    
    if(close(server_listen_fd) < 0){
        perror("test_server: close(server_listen_fd)");
        exit(1);
    }   
    printmsg('s', "test server stopped");
    
    do {
        //waiting for closing fd and free ip addres/port
        errno = 0;
    } while(fcntl(server_listen_fd, F_GETFD) != -1 || errno != EBADF);
    server_listen_fd = -1;
}

void Ttest_server::test_server_connection_loop()
{       
    bool c_st;
    bool d_st;
    bool d_st_0; //def
    bool d_st_1; //run
    bool d_st_2; //acc
    bool d_st_3; //con
    bool d_st_00; //!def
    bool d_st_11; //!run
    bool d_st_22; //!acc
    bool d_st_33; //!con

    unique_lock<mutex> ulst_c(st.mtx_curr, defer_lock);  //state
    unique_lock<mutex> ulst_d(st.mtx_dem, defer_lock);   //state

    unique_lock<mutex> ula(mtx_srv_def, defer_lock);     //cv
    unique_lock<mutex> ulrs(mtx_srv_run, defer_lock);    //cv
    unique_lock<mutex> uldef(mtx_srv_accept, defer_lock);  //cv






    printmsg('s', "test server");





    while(1) {  //while !def
        printmsg('s', "waiting for definition of ip address ...");
        uldef.lock();
        cv_srv_def.wait(uldef, [&]{
            LOCK_UNLOCK(c,
                c_st = st.current.eq(1, 2, 2, 2);
            );
            LOCK_UNLOCK(d, 
                d_st_00 = st.demand.eq(2, 0, 2, 2);                
            );
            return (c_st || d_st_00);            
        });
        uldef.unlock();
        printmsg('s', "received: def or !run");

        if(d_st_00) { //demand: !run
            SET_CURR_ST(0, 0, 0, 0); //current: !def, !run, !acc, !con
            cv_st_Nrun.notify_one();      

            // destroy_socket();
            printmsg('s', "received command: !run");
            return;
        }
        if(!c_st) {  //current: !def        
            printmsg('s', "addres ip/port not defined");            
            continue;
        }
        else {
            printmsg('s', "ip address defined");
            break;
        }
    }
    

    
    printmsg('s', "waiting for start command...");
    /* worker:  server loop (after create socket and bind ip addres)*/        
    ulrs.lock();
    cv_srv_run_enable.wait(ulrs, [&]{
        LOCK_UNLOCK(d,                        
            d_st_1 = st.demand.eq(2, 1, 2, 2);  //demand: run
        );
        return d_st_1;   //only notify_one()
    });
    ulrs.unlock();
    
    if(d_st_1) {  //demand: run 
                    
        SET_CURR_ST(2, 1, 2, 2); //current set : run
        cv_st_run.notify_one();        

        printmsg('s', "test server starting");

        printmsg('s', "start listening");            
        listen_fd_run();

        while(1) //while LISTEN
        {   
            printmsg('s', "waiting for: accepting enable");                      
            /* connection accepting loop */                
            ula.lock();
            cv_srv_accept_enable.wait(ula, [&]{
                LOCK_UNLOCK(d,                    
                    d_st_2 = st.demand.eq(2, 2, 1, 2);   //demand: acc                  
                    d_st_11 = st.demand.eq(2, 0, 2, 2);  //demand : !run                                                                
                );
                return (d_st_2 || d_st_11);
            });           
            ula.unlock(); 

            //assert(current: !con)

            if(d_st_11) { //demand: !run
                SET_CURR_ST(2, 0, 2, 2); //current set: !run                    
                cv_st_Nrun.notify_one();
                
                printmsg('s', "'not run' issued'");                    
                break; //while LISTEN : listening --> off, -->!run -->def enable
            }  
            if(!d_st_2) {
                SET_CURR_ST(2, 2, 0, 2); //current set: !acc     
                cv_st_Nacc.notify_one();
                
                printmsg('s', "'accepting not enable' issued'");                                
                continue; //!acc                    
            }


            printmsg('s', "'accepting enable' issued'");            
            size_t listen_loop_number_set;                

            
            SET_CURR_ST(2, 2, 1, 2); //current set: acc
            LOCK_UNLOCK(d,                  
                listen_loop_number_set = listen_loop_number;                   
            );  
            cv_st_acc.notify_one();                                               

            printmsg('s', "accepting enable");                                                            
            
            /* connection accepting loop */
            /* listen_loop_number_set == 0 --> infinity*/
            for(size_t listen_loop_counter = listen_loop_number_set;                 
                !listen_loop_number_set ? 1 : listen_loop_counter;                                
                listen_loop_counter = (
                    !listen_loop_number_set 
                    ? ++listen_loop_counter 
                    : --listen_loop_counter)
                )                 
            {
                LOCK_UNLOCK(d,                     
                    d_st_2 = st.demand.eq(2, 2, 0, 2);  //demand: !acc                            
                    d_st_11 = st.demand.eq(2, 0, 2, 2); //demand: !run                        
                );
                if(d_st_2 || d_st_11) { //demand: !acc  OR  !run
                    break;
                }


                fprintf(server_log, "\nserver: %s %s:%d ...\n", "wait for connection on", server_ip_address, server_ip_port);
                
                SET_CURR_ST(2, 2, 2, 0); //set : !con                                    

                //accept (TODO: wait 10 sec !!!!)
                accepted_fd_connect();
                                    
                SET_CURR_ST(2, 2, 2, 1); //set: con                                                        

                if(listen_loop_number_set) { fprintf(server_log, "\nserver: %s %zu/%zu %s\n", "connection", (listen_loop_number_set - listen_loop_counter), listen_loop_number_set, "accepted");}
                else { fprintf(server_log, "\nserver: %s %zu : %s\n", "connection", listen_loop_counter, "accepted");}
                                
                monitor_connection_init();
                
                /* monitoring connetion state */    
                
                fprintf(server_log, "\nserver: %s\n", "connection monitoring start");                    
                while(1){ //while CONN

                    fprintf(server_log, "%c", '.');

                    monitor_connection();
                    
                    //checking here becouse of continue
                    LOCK_UNLOCK(d,                                                 
                        d_st_33 = st.demand.eq(2, 2, 2, 0); //demand: !con                            
                        d_st_22 = st.demand.eq(2, 2, 0, 2); //demand: !acc                            
                        d_st_11 = st.demand.eq(2, 0, 2, 2); //demand: !run
                    );                                                
                    if(d_st_33 || d_st_22 || d_st_11) {
                        break; //while CONN
                    }

                    if(monitor_connection_no_change()) {
                        continue;
                    }                          
                    print_connection_state();                          
                    
                    if(check_if_clientd_disconnects()) {}

                } //while CONN
                
                fprintf(server_log, "\nserver: %s\n", "connection monitoring loop broken");                    

                close_accepted_fd();
                                
                SET_CURR_ST(2, 2, 2, 0); //set: !con                                                  
                cv_st_Ncon.notify_one();      

                fprintf(server_log, "\nserver: %s\n", "connection monitoring stopped");
                






            } //for(listen_loop_counter)
            fprintf(server_log, "\nserver: %s\n", "accepting connection loop broken");            
            
            SET_CURR_ST(2, 2, 0, 2); //set: !acc                                            
            cv_st_Nacc.notify_one();

            fprintf(server_log, "\nserver: %s\n", "accepting connection stopped");   

            if(d_st_11) {  //demand : !run
                break;  
            }
            
        } //while LISTEN
        fprintf(server_log, "\nserver: %s\n", "listening finished");
        fflush(server_log);    

    }//if !run

    SET_CURR_ST(0, 0, 2, 2); //set: !def, !run               
    cv_st_Nrun.notify_one();     

    fprintf(server_log, "\nserver: %s\n", "ready to close socket");
    fflush(server_log);    

    // destroy_socket();
    return;
}





void Ttest_server::wait_for_state_neq(int d, int r, int a, int c)
{
    // bool c_st;        
    // unique_lock<mutex> ulst_c(st.mtx_curr, defer_lock);
    // do {
    //     ulst_c_lock();
    //     c_st = st.current.eq(d, r, a, c);
    //     ulst_c_unlock();        
    //     sleep(1);
    //     fprintf(server_log, "%c", '.');
    //     fflush(server_log);
    // } while(c_st);    
    // fprintf(server_log, "%c", '\n');
}

void Ttest_server::start_test_server() 
{
    bool c_st;
    bool d_st;
    unique_lock<mutex> ulst_c(st.mtx_curr, defer_lock);
    unique_lock<mutex> ulst_d(st.mtx_dem, defer_lock);
    
    fprintf(server_log, "\ncommand: %s\n", "start_test_server");
    LOCK_UNLOCK(d,     
        st.demand.set(2, 1, 2, 2);    
    );
    fprintf(server_log, "\ncommand: %s\n", "start_test_server: issued");
    cv_srv_run_enable.notify_one();

    // wait_for_state_neq(2, 1, 2, 2);        
    unique_lock<mutex> ul_cmd_run(mtx_cmd_run);
    cv_st_run.wait(
        ul_cmd_run,
        [&] {
            LOCK_UNLOCK(c,
                c_st = st.current.eq(2, 1, 2, 2);
            );
            return c_st;
        }
    );
    ul_cmd_run.unlock();
    fprintf(server_log, "\ncommand: %s\n", "start_test_server: state achieve");       
}

void Ttest_server::stop_test_server()
{      
    bool c_st;
    bool d_st;
    unique_lock<mutex> ulst_c(st.mtx_curr, defer_lock);
    unique_lock<mutex> ulst_d(st.mtx_dem, defer_lock);


    LOCK_UNLOCK(c,
        c_st = st.current.eq(0, 2, 2, 2);
    );    
    if(c_st) {   //not def and command !run
        cv_srv_def.notify_one();
        return;
    }

    unique_lock<mutex> ul_cmd_Nrun(mtx_cmd_Nrun, defer_lock);

    fprintf(server_log, "\ncommand: %s\n", "stop_test_server");               
    LOCK_UNLOCK(d, 
        st.demand.set(2, 0, 2, 2);    
    );
    fprintf(server_log, "\ncommand: %s\n", "stop_test_server: issued");           

    // wait_for_state_neq(2, 0, 2, 2);        

    cv_srv_accept_enable.notify_one();    
    
    ul_cmd_Nrun.lock();
    cv_st_Nrun.wait(
        ul_cmd_Nrun,
        [&] {
            LOCK_UNLOCK(c,
                c_st = st.current.eq(2, 2, 0, 2);
            );
            return c_st;
        }
    );
    ul_cmd_Nrun.unlock();
    fprintf(server_log, "\ncommand: %s\n", "stop_test_server: state partialy achieve");       

    cv_srv_run_enable.notify_one();

    // wait_for_state_neq(0, 0, 0, 0);
        
    ul_cmd_Nrun.lock();
    cv_st_Nrun.wait(
        ul_cmd_Nrun,
        [&] {
            LOCK_UNLOCK(c,
                c_st = st.current.eq(2, 0, 2, 2);
            );
            return c_st;
        }
    );
    ul_cmd_Nrun.unlock();
    fprintf(server_log, "\ncommand: %s\n", "stop_test_server: state achieve");       
}   

void Ttest_server::start_accepting_connections(size_t number)
{    
    bool c_st;
    bool d_st;
    unique_lock<mutex> ulst_c(st.mtx_curr, defer_lock);
    unique_lock<mutex> ulst_d(st.mtx_dem, defer_lock);

    fprintf(server_log, "\ncommand: %s\n", "start_accepting_connections");
    LOCK_UNLOCK(c, 
        c_st = st.current.eq(1, 1, 0, 2);
    );
    if(!c_st) {
        fprintf(server_log, "\ncommand: %s\n", "start_accepting_connection: not suitable in this moment");
        return;
    }

    LOCK_UNLOCK(d,             
        st.demand.set(2, 2, 1, 2);
        listen_loop_number = number;    
    );
    fprintf(server_log, "\ncommand: %s\n", "start_accepting_connections: issued");
    cv_srv_accept_enable.notify_one();
    
    // wait_for_state_neq(2, 2, 1, 2);    
    unique_lock<mutex> ul_cmd_acc(mtx_cmd_acc);
    cv_st_acc.wait(
        ul_cmd_acc,
        [&] {
            LOCK_UNLOCK(c,
                c_st = st.current.eq(2, 2, 1, 2);
            );
            return c_st;
        }
    );
    ul_cmd_acc.unlock();    
    fprintf(server_log, "\ncommand: %s\n", "start_accepting_connections: state achieve");       
}

void Ttest_server::stop_accepting_connections()
{
    bool c_st;
    bool d_st;
    unique_lock<mutex> ulst_c(st.mtx_curr, defer_lock);
    unique_lock<mutex> ulst_d(st.mtx_dem, defer_lock);

    fprintf(server_log, "\ncommand: %s\n", "stop_accepting_connections");

    LOCK_UNLOCK(c, 
        c_st = st.current.eq(1, 1, 1, 2);
    );
    if(!c_st) {
        fprintf(server_log, "\ncommand: %s\n", "stop_accepting_connections: not suitable in this moment");
        return;
    }
    
    LOCK_UNLOCK(d,     
        st.demand.set(2, 2, 0, 2);        
    );
    fprintf(server_log, "\ncommand: %s\n", "stop_accepting_connections: issued");

    // wait_for_state_neq(2, 2, 0, 2);    
    unique_lock<mutex> ul_cmd_Nacc(mtx_cmd_Nacc);
    cv_st_Nacc.wait(
        ul_cmd_Nacc,
        [&] {
            LOCK_UNLOCK(c,
                c_st = st.current.eq(2, 2, 0, 2);
            );
            return c_st;
        }
    );
    ul_cmd_Nacc.unlock();   
    fprintf(server_log, "\ncommand: %s\n", "stop_accepting_connections: state achieve");       
}

void Ttest_server::start_monitoring_connection() {
    bool c_st;
    bool d_st;
    unique_lock<mutex> ulst_c(st.mtx_curr, defer_lock);
    unique_lock<mutex> ulst_d(st.mtx_dem, defer_lock);

    LOCK_UNLOCK(c, 
        st.opt.show_POLL_monitor = 1;
    );
    fprintf(server_log, "\ncommand: %s\n", "start_monitoring_connection");
}

void Ttest_server::stop_monitoring_connection() {
    bool c_st;
    bool d_st;
    unique_lock<mutex> ulst_c(st.mtx_curr, defer_lock);
    unique_lock<mutex> ulst_d(st.mtx_dem, defer_lock);
    
    LOCK_UNLOCK(c, 
        st.opt.show_POLL_monitor = 0;
    );    
    fprintf(server_log, "\ncommand: %s\n", "stop_monitoring_connection");
}

void Ttest_server::close_monitored_connection()
{   
    bool c_st;
    bool d_st;
    unique_lock<mutex> ulst_c(st.mtx_curr, defer_lock);
    unique_lock<mutex> ulst_d(st.mtx_dem, defer_lock);

    fprintf(server_log, "\ncommand: %s\n", "close_monitored_connection");

    LOCK_UNLOCK(c, 
        c_st = st.current.eq(1, 1, 1, 1);
    );
    if(!c_st) {
        fprintf(server_log, "\ncommand: %s\n", "close_monitored_connection: not suitable in this moment");
        return;
    }    
    
    LOCK_UNLOCK(d, 
        st.demand.set(2, 2, 2, 0);
    );    

    fprintf(server_log, "\ncommand: %s\n", "close_monitored_connection: issued");
    // wait_for_state_neq(2, 2, 2, 0);
    unique_lock<mutex> ul_cmd_Ncon(mtx_cmd_Ncon);
    cv_st_Ncon.wait(
        ul_cmd_Ncon,
        [&] {
            LOCK_UNLOCK(c,
                c_st = st.current.eq(2, 2, 2, 0);
            );
            return c_st;
        }
    );
    ul_cmd_Ncon.unlock();      
    fprintf(server_log, "\ncommand: %s\n", "close_monitored_connection: state achieve");    
}

bool Ttest_server::in_fd_are_set_only(short flags) {
    return fds[0].revents & flags;
}

void Ttest_server::print_connection_state() {
    if(st.opt.show_POLL_monitor) {                            
        /* f(comment, not comment for empty position)*/
        debug_print_poll_state(true, false);
    }
}

void Ttest_server::debug_print_poll_state(bool comment, bool show_empty) {

    #define PRINT_POLL(X, COMMENT)\
    do {\
        if((show_empty ? true : (fds[0].revents & X ))) {\
            fprintf(server_log, "[%s]", fds[0].revents & X ? #X : "");\
            if(comment) {\
                fprintf(server_log, " - {%p} %s\n", (void*) X , COMMENT );\
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
        fprintf(server_log, "%s\n", "");
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
        fprintf(server_log, "%s\n", "");

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
    fflush(server_log);
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
            fprintf(server_log, "%s\n", "\nserver: client probably close connection");            
            //TODO: print connection state            
    }
    else if(readval != -1) {
        buff[readval] = '\0';
        fprintf(server_log, "\nserver: 'client wrote:[ %s ]'\n", buff);            
    }
    else {
        //AGAIN  EAGAIN
        fprintf(server_log, "%s\n", "\nserver: client not ready to read EAGAIN");
    }    
    fprintf(server_log, "\nserver: %s\n", "all data readed");
    return 0;
}    


/**********************************************************/


/**********************************************************/
FILE *server_log;
void printmsg(const char source, const char *msg) {
    const char *t;
    switch(source) {
        case 's':
            t = "server";
            break;
        case 'c':
            t = "connection";
            break;
        case 'd' :
            t = "define";
            break;
        case '0':
            t = "constructor";
            break;
        case '1':
            t = "destructor";
            break;
    }
    fprintf(server_log, "\n%s : %s\n", t, msg);
    fflush(server_log);
}
/**********************************************************/


#undef ERROR_WHERE
#undef ERROR_IN
#undef LOCK_UNLOCK
#undef SET_CURR_ST