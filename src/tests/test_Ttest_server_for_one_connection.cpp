#include <gtest/gtest.h>
#include "../Ttest_server_for_one_connection.h"
#include <unistd.h>
#include <cstdio>
#include <fcntl.h>
#include <cstring>
#include <iostream>
#include "../helper.h"
using namespace std;
using namespace testing;

class pipe_printmsg : public Test {
    protected:
    int pipefd[2];
    
    void SetUp() override {
        if(pipe(pipefd) == -1) {
            ASSERT_TRUE(false) << "can't create pipe()";
        }    
        cout << "file desriptor falgs: " << fcntl(pipefd[1], F_GETFD) << endl;
        cout << "file status flags   : " << fcntl(pipefd[1], F_GETFL) << endl;

        check_fd_mode(pipefd[1]);

        server_log = fdopen(pipefd[1], "w");
        cout << "fileno(server_log) : " << fileno(server_log) << endl;
        // int server_log_fd = server_log->_fileno;
        // server_log->_fileno = pipefd[1];        
        // // cout << "dup2()" << dup2(server_log->_fileno, pipefd[1]) << endl;        
    }
    void TearDown() override {
        fflush(stdout);                
    }
};

TEST_F(pipe_printmsg, test_01) {
    
    char buff_in[1024];
    char buff_out[1024];

    printmsg('c', "connection_msg");  
               
    // const char* msg1 = "hello";
    // strncpy(buff_out, msg1, strlen(msg1));
    // write(pipefd[1], buff_out, strlen(msg1));
    
    int readed = read(pipefd[0], buff_in, 1023);    
    buff_in[readed] = 0;
    
    EXPECT_STREQ("\nconnection : connection_msg\n", buff_in);
    
    // dup2(pipefd[1], server_log->_fileno);cout << "server_log->_fileno : " << server_log->_fileno << endl;
    // server_log->_fileno = server_log_fd;
    cout << "buff_out: [" << buff_out << "]" << endl;
    cout << "buff_in : [" << buff_in << "]" << endl;
    close(pipefd[0]);
    
    // close(pipefd[1]);    //fclose does that        
    fclose(server_log); 
}

TEST_F(pipe_printmsg, test_02) {
    char buff_in[1024];
    char buff_out[1024];
    check_fd_mode(pipefd[0]);
    FILE *logIN = fdopen(pipefd[0], "r");

    printmsg('s', "server_msg");  
    
    fclose(server_log); //<-- neaded here to eof for logIN

    int readed = fread(buff_in, sizeof(char), 1023, logIN);
    buff_in[readed] = 0;

    EXPECT_STREQ("\nserver : server_msg\n", buff_in);
        
    cout << "buff_out: [" << buff_out << "]" << endl;
    cout << "buff_in : [" << buff_in << "]" << endl;
    fclose(logIN);
}

int main(int argc, char** argv) {
    InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}