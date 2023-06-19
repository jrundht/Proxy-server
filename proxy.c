/*
 * This is the main program for the proxy, which receives connections for sending and receiving clients
 * both in binary and XML format. Many clients can be connected at the same time. The proxy implements
 * an event loop.
 *
 * *** YOU MUST IMPLEMENT THESE FUNCTIONS ***
 *
 * The parameters and return values of the existing functions must not be changed.
 * You can add function, definition etc. as required.
 */
#include "xmlfile.h"
#include "connection.h"
#include "record.h"
#include "recordToFormat.h"
#include "recordFromFormat.h"

#include <arpa/inet.h>
#include <sys/errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <sys/select.h>

#define BUF_SIZE 10000

typedef struct Client{
    int socket;
    char id;
    char type;         
    char buffer[BUF_SIZE];
    int bufLen;
    struct Client *next; 
}Client;

Client *head = NULL;

void usage(char* cmd){
    fprintf( stderr, "Usage: %s <port>\n"
                     "       This is the proxy server. It takes as imput the port where it accepts connections\n"
                     "       from \"xmlSender\", \"binSender\" and \"anyReceiver\" applications.\n"
                     "       <port> - a 16-bit integer in host byte order identifying the proxy server's port\n"
                     "\n",
                     cmd );
    exit( -1 );
}

/*Connecting clients to create a linked list*/
void setNextClient(Client *new_client){
    if(head == NULL){
        head = new_client;
    }else{
        Client *current = head;
        while(current->next != NULL){
            current = current->next;
        }
        current->next = new_client;
    }
}

int numClients(){
    int ant = 0;
    Client *current = head;
    while(current != NULL ){
        ant++;
        current = current->next;
    }
    return ant;
}

/* When a sender/receiver socket has closed, remove the client from the 
 * list and free it
 */
void removeClientFromList(Client *client){
    if(client == NULL){                    
        return; 
    }

    if(client == head){
        head = client->next;
        free(client);
        return;
    }

    Client *tmp = head;
    while(tmp->next != NULL && tmp->next != client){
        tmp = tmp->next;
    }

    if(tmp->next == client){
        tmp->next = tmp->next->next;
    }
    free(client);
}

/*find the client whose socket has activity*/
Client *findClient(int fd){
    Client *current = head;
    while(current != NULL){
        if(current->socket == fd){
            return current;
        }
        current = current->next;
    }
    return NULL;
}

/*find the client that is supposed to receive msg*/
Client *findClientById(char id){
    Client *current = head;
    while(current != NULL){
        if(current->id == id){
            return current;
        }
        current = current->next;
    }
    return NULL;
}

void updateMaxfd(int *max_fd){
    int max_socket = *max_fd;
    Client *client = head;
    while(client != NULL){
        if(client->socket > max_socket){
            max_socket = client->socket;
        }
        client = client->next;
    }
    *max_fd = max_socket;
}

/*
 * This function is called when a new connection is noticed on the server
 * socket.
 * The proxy accepts a new connection and creates the relevant data structures.
 *
 * *** The parameters and return values of this functions can be changed. ***
 */
void handleNewClient(int server_sock){
    Client *new_client = malloc(sizeof(Client));
    if(new_client == NULL){
        fprintf(stderr, "Failed at allocating heap memory for Client");
        exit(-1);
    }    

    int client_fd, rc = 0;
    char buf[10];

    memset(new_client, 0, sizeof(Client));
    memset(&buf, 0 , 10);

    client_fd = tcp_accept(server_sock); 
    if(client_fd == -1) return;                
    if(client_fd < 0) free(new_client);      

    printf("Accepted new client: %d\n", client_fd);

    while(rc < 2){
        rc += tcp_read(client_fd, buf+rc, 1);  
    }
    if(rc != 2) fprintf(stderr, "tcp_read: did not read enough data: %d\n", rc); 

    new_client->socket = client_fd;
    new_client->type =  buf[0];
    new_client->id = buf[1];
    printf("type: %c\n", new_client->type);
    printf("id: %c\n\n", new_client->id);

    new_client->next = NULL;    
    setNextClient(new_client);                 
}

 /* This function is called when a connection is broken by one of the connecting
 * clients. Data structures are clean up and resources that are no longer needed
 * are released.
 *
 * *** The parameters and return values of this functions can be changed. ***
 */
void removeClient( Client* client ){
    printf("Removing client: %c, socket: %d\n\n", client->id, client->socket);
    tcp_close(client->socket);
    removeClientFromList(client);
}

/*
 * This function is called when the proxy received enough data from a sending
 * client to create a Record. The 'dest' field of the Record determines the
 * client to which the proxy should send this Record.
 *
 * If no such client is connected to the proxy, the Record is discarded without
 * error. Resources are released as appropriate.
 *
 * If such a client is connected, this functions find the correct socket for
 * sending to that client, and determines if the Record must be converted to
 * XML format or to binary format for sendig to that client.
 *
 * It does then send the converted messages.
 * Finally, this function deletes the Record before returning.
 *
 * *** The parameters and return values of this functions can be changed. ***
 */
void forwardMessage(Record *msg){ 
    int bufSize, wc = 0;
    char *buffer;
    if(!msg->has_dest){                         
        deleteRecord(msg);
        return;
    }

    Client *destClient = findClientById(msg->dest);
    if(destClient == NULL){                                     
        fprintf(stderr, "Client: %c not found\n\n", msg->dest);
        deleteRecord(msg);
        return;
    }
    if(destClient->type == 'X'){
        buffer = recordToXML(msg, &bufSize);
    }else{
        buffer = recordToBinary(msg, &bufSize);
    }

    if(buffer != NULL) wc = tcp_write(destClient->socket, buffer, bufSize); 
    if(wc == 0) fprintf(stderr, "Receiver closed before getting message");   

    free(buffer);
    deleteRecord(msg);
}

/*
 * This function is called whenever activity is noticed on a connected socket,
 * and that socket is associated with a client. This can be sending client
 * or a receiving client.
 *
 * The calling function finds the Client structure for the socket where acticity
 * has occurred and calls this function.
 *
 * If this function receives data that completes a record, it creates an internal
 * Record data structure on the heap and calls forwardMessage() with this Record.
 *
 * If this function notices that a client has disconnected, it calls removeClient()
 * to release the resources associated with it.
 *
 * *** The parameters and return values of this functions can be changed. ***
 */
void handleClient( Client* client ){
    Record *record;
    int rc;
    int writtenToRecord = 0;

    char tcpReadBuf[BUF_SIZE];
    memset(tcpReadBuf, 0, BUF_SIZE);
    rc = tcp_read(client->socket, tcpReadBuf, BUF_SIZE); 

    if(rc < 1){            
        removeClient(client); 
        return;
    }

    char buffer[BUF_SIZE*2 + 1];
    memset(buffer, 0, BUF_SIZE);

    if(client->bufLen > 0){
        memcpy(buffer, client->buffer, client->bufLen);
    }
    
    memcpy(buffer + client->bufLen, tcpReadBuf, rc);
    rc += client->bufLen;

    buffer[rc] = '\0';
    client->bufLen = 0;

    while(writtenToRecord < rc){
        if(client->type == 'X'){
            record = XMLtoRecord(buffer, rc, &writtenToRecord); 

        }else if(client->type == 'B'){
            record = BinaryToRecord(buffer, rc, &writtenToRecord); 

        }else{
            fprintf(stderr, "Unrecognized format: %c\n", client->type);
            removeClient(client);
            return;
        }
        if(record == NULL){ 
            client->bufLen = rc - writtenToRecord;
            memmove(client->buffer, buffer + writtenToRecord, client->bufLen);
            return;
        }
        forwardMessage(record);
    }
}

int main( int argc, char* argv[] ){ 
    int port;
    int server_sock, max_fd;

    if(argc != 2) usage(argv[0]);
    port = atoi(argv[1]);
    
    server_sock = tcp_create_and_listen(port);
    if(server_sock < 0) exit(-1);
    max_fd = server_sock;

    do{
        fd_set file_descriptors;
        FD_ZERO(&file_descriptors);
        FD_SET(server_sock, &file_descriptors);
        updateMaxfd(&max_fd);

        Client *current = head;
        while(current != NULL){
            FD_SET(current->socket, &file_descriptors);
            current = current->next;
        }
        
        if(tcp_wait(&file_descriptors, max_fd + 1) < 0) exit(-1); 

        if(FD_ISSET(server_sock, &file_descriptors)){  
            handleNewClient(server_sock); 

        }else{                                         
            for(int fd = 0; fd <= max_fd; fd++){    
                if (FD_ISSET(fd, &file_descriptors)){
                    Client *client = findClient(fd);
                    if (client != NULL){
                        handleClient(client);         
                    }else{
                        fprintf(stderr, "Could not find the client");
                    }
                }
            }
        }
    }while(numClients() > 0); 

    tcp_close( server_sock );

    return 0;
}