/*
 * This is a file that implements the operation on TCP sockets that are used by
 * all of the programs used in this assignment.
 *
 * *** YOU MUST IMPLEMENT THESE FUNCTIONS ***
 *
 * The parameters and return values of the existing functions must not be changed.
 * You can add function, definition etc. as required.
 */
#include "connection.h"
#include <sys/socket.h>     
#include <netinet/in.h>     
#include <arpa/inet.h>      
#include <stdio.h>          
#include <unistd.h>         
#include <sys/select.h>     

// function used to check for error 
int check_error(int res, char *msg){
    if(res == -1){
        perror(msg);
        return -1;
    }
    return res;
}

int tcp_create_socket(){ 
    int fd;
    fd = socket(AF_INET, SOCK_STREAM, 0); 
    if(check_error(fd, "socket") == -1) return -1;
    return fd;
}

int tcp_connect( char* hostname, int port ){
    int fd, wc;
    struct sockaddr_in dest_addr;
    
    fd = tcp_create_socket(); 
    if(fd == -1) return -1;

    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port); 

    wc = inet_pton(AF_INET, hostname, &dest_addr.sin_addr.s_addr);

    if(check_error(wc, "inet_pton") == -1){
        tcp_close(fd);
        return -1 ;}
    
    if(wc == 0){ 
        fprintf(stderr, "Invalid network address: %s\n", hostname);
        close(fd);
        return -1 ;
    }

    wc = connect(fd, (struct sockaddr*)&dest_addr, sizeof(struct sockaddr_in)); 
    if(check_error(wc, "connect") == -1){
        tcp_close(fd);
        return -1;
    }
    return fd; 
}

int tcp_read( int sock, char* buffer, int n){
    int rc;
    rc = read(sock, buffer, n);

    if(check_error(rc, "read") == -1) return -1;
    if(rc == 0){
        printf("Connection closed\n");
    }
    
    return rc;
}

int tcp_write( int sock, char* buffer, int bytes ){
    int wc;
    wc = write(sock, buffer, bytes);    

    if(check_error(wc, "write") == -1) return -1;

    return wc;
}

int tcp_write_loop( int sock, char* buffer, int bytes ){
    int w, wc = 0;

    while(wc < bytes){ 
        w = write(sock, &buffer[wc], bytes - wc); 
        if(check_error(w, "write_loop") == -1) return -1;
        wc += w;
    }
    return wc;
}

void tcp_close( int sock ){
    if(close(sock) != 0) check_error(-1, "close");
}

int tcp_create_and_listen( int port ){
    int fd, rc;
    struct sockaddr_in server_addr;
    
    fd = tcp_create_socket(); 
    if(fd == -1) return -1;

    server_addr = (struct sockaddr_in){
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr.s_addr = INADDR_ANY, 
    };
 
    rc = bind(fd, (struct sockaddr*)&server_addr, sizeof(struct sockaddr_in));
    if(check_error(rc, "bind") == -1){ 
        tcp_close(fd);
        return -1;
    }

    rc = listen(fd, 128); 
    if(check_error(rc, "listen") == -1){ 
        tcp_close(fd);
        return -1;
    }
    return fd;
}

int tcp_accept( int server_sock ){
    int fd;
    fd = accept(server_sock, NULL, NULL);
    if(check_error(fd, "accept") == -1){ 
        tcp_close(fd);
        return -1;
    }
    return fd;
}

int tcp_wait( fd_set* waiting_set, int wait_end ){
    int number_fds = 0;

    number_fds = select(wait_end, waiting_set, NULL, NULL, NULL); 
    if(check_error(number_fds, "select") == -1) return -1;

    return number_fds;
}

int tcp_wait_timeout( fd_set* waiting_set, int wait_end, int seconds ){
    int number_fds = 0;
    struct timeval timer;
    timer.tv_sec = seconds;
    timer.tv_usec = 0; 

    number_fds = select(wait_end, waiting_set, NULL, NULL, &timer);
    if(check_error(number_fds, "select") == -1) return -1;

    return number_fds;
}