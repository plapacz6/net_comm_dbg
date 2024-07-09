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

#include <gtest/gtest.h>
#include "../client_c.h"
// #define _GNU_SOURCE
#include <thread>

#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <cassert>

#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
// #include <sys/select.h>
#include <poll.h>
#include <sys/time.h>

using namespace std;
using namespace testing;

#include "../Ttest_server_for_one_connection.h"


Ttest_server& test_server = Ttest_server::create_test_server();

class client_c_fixture_01 : public Test
{
    protected:
        
    // static void SetUpTestCase()
    // {
    // }
    // static void TearDownTestCase() 
    // {
    // }
    void SetUp() override
    {   
        cout << "fixture SetUp()" << endl;    
        // sleep(3);        
        cout << "fixture SetUp() end" << endl;    
    }    
    void TearDown() override
{       cout << "fixture TearDown()" << endl;    

        // test_server.close_monitored_connection();    
        // test_server.stop_accepting_connections();                
 
        cout << "fixture TearDown() end" << endl;
    }
};

TEST_F(client_c_fixture_01, connect_01) 
{           
    test_server.start_accepting_connections(2);
    sleep(3);
    int client_fd = connect_to_server(
        const_cast<char*>(test_server.server_ip_address), test_server.server_ip_port);

    sleep(3);
    ASSERT_NE(client_fd, -1);  //error
    ASSERT_NE(client_fd, 0);  //stdin
    ASSERT_NE(client_fd, 1);  //stdout
    ASSERT_NE(client_fd, 2);  //stderr    

    close(client_fd);
    sleep(3);

    test_server.close_monitored_connection();
    sleep(3);    
    
    client_fd = connect_to_server(
        const_cast<char*>(test_server.server_ip_address), test_server.server_ip_port);                
    sleep(3);           
    ASSERT_NE(client_fd, -1);  //error
    ASSERT_NE(client_fd, 0);  //stdin
    ASSERT_NE(client_fd, 1);  //stdout
    ASSERT_NE(client_fd, 2);  //stderr    

    // test_server.stop_accepting_connections();                
    sleep(3);    
}


TEST_F(client_c_fixture_01, disconnect_01) 
{
    test_server.start_accepting_connections(1);
    sleep(3);        
    int client_fd = connect_to_server(
        const_cast<char*>(test_server.server_ip_address), test_server.server_ip_port);    
            
    sleep(3);
    int ret = disconnect_from_server(client_fd);
    sleep(3);        
    ASSERT_EQ(0, ret);
    enum {buff_size = 256};
    char buff[buff_size];
    char msg[] = "test";
    strncpy(buff, msg, strlen(msg) < buff_size ? strlen(msg) - 1 : buff_size - 1);
    buff[buff_size - 1] = 0;
    ret = write(client_fd, buff, strlen(msg));
    int err = errno;
    ASSERT_EQ(-1, ret);
    ASSERT_EQ(EBADF, err);  // EDESTADDRREQ ???
    cout << "test_2_end" << endl;
    sleep(3);    
}

extern FILE * server_log;

int main(int argc, char** argv)
{
    int main_result;
    InitGoogleTest(&argc, argv);

    server_log = server_log = fdopen(1, "w"); //stdout;
    
    thread server_thread = thread(&Ttest_server::test_server_connection_loop, &test_server);
    // server_thread.detach();  //! <- thread worker use object body, so object must exist    

    test_server.define_test_server("127.0.0.1", 8080);
    test_server.start_test_server();

    main_result = RUN_ALL_TESTS(); //<- warning: ThreadStnitizer : main_result    
    
    server_thread.join();            

    test_server.destroy_socket(); // <- run loop must be broken before thread.join()
    
    return main_result;
}