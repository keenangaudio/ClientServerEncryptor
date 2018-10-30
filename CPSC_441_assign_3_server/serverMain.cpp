//
//  serverMain.cpp
//  CPSC_441_assign_3_server
//
//  modified from: (tbh i completely refactored it and ended up rewriting most of it)
//
//  wordlengthserver.c
//  by Carey Williamson on January 13, 2012
//
//  by Keenan on 2018-10-15.
//
// original info:

/* TCP-based server example of socket programming.    */
/* The server receives an input word (e.g., "dog")    */
/* and returns the length of the word (e.g., "3").    */
/*                                                    */
/* Usage:   cc -o wordlengthserver wordlengthserver.c */
/*          ./wordlengthserver                        */
/*                                                    */
/* Written by Carey Williamson       January 13, 2012 */


#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <signal.h>
#include <string.h>
#include <stdint.h>

#include "encryptor.h"
/* C error handling */
extern int errno;
void perror(const char *s);

/* Global manifest constants */
#define MYPORTNUM 12345
#define CLOSE_CHILD 3456172
#define MAX_RETRY 200

/* arguments */
#define FLAG_ARG 1
#define NUM_ARGS 2

/* Optional verbose debugging output */
//#define DEBUG 1
//#define FULLDEBUG 1
//#define INSIDE 1

/* Global variables */
int serverSock, babyserverSock;
struct sockaddr_in server;
static struct sigaction act;
char messagein[MAX_MESSAGE_LENGTH];
char messageout[MAX_MESSAGE_LENGTH * 7];    //all single characters would produce 0x#### and a space (~6:1 ratio)
char c;
Encryptor e;
static int retry = 0;

/* function headers, to avoid using a header file */
bool interact(void);    // returns true if recieves a QUIT command
bool checkQuit(char in[]);
void initServer(void);
void safetyNet (void);
bool acceptConnection(void);
void serverListen(void);
void encode(void);
eType handleArgs(int argc, const char * argv[]);

/* This is a signal handler to do graceful exit if needed */
extern "C" void catcher( int sig )
{
    printf("YIKES THIS IS NOT GOUDA\n");
    if (sig != CLOSE_CHILD) close(serverSock);
    close(babyserverSock);
    exit(0);
}

/* Main program for server */
int main(int argc, const char * argv[])
{
    {   // because i can't spare that 16 bits of stack
        eType tp = handleArgs(argc, argv);
        e = *(new Encryptor(tp));
        e.setEncrytionAlgorithm (tp) ;

        //pre hash the bonus words so you can see it work (tries a couple times)
        if (tp == CHECKSUM) {
            for (int i = 0; i < 5; i++) e.transform(INITIAL_COMMIT);
        }
    }
    safetyNet();

    initServer();

    int pid;
    /* Main loop: server loops forever listening for requests */
    while( 1 ) {
        if ( !acceptConnection() ) continue;
        pid = fork();
        if (pid == 0) {
            printf("created baby server...\n");
        /* interact returns true if client issues a quit command */
            if (interact()) catcher( CLOSE_CHILD );
        } else close(babyserverSock);

    }
}

bool interact() {
    /* obtain the message from this client */
    while( recv(babyserverSock, messagein, MAX_MESSAGE_LENGTH, 0) > 0 )
    {
        /* print out the received message */
        printf("recieved tweet: %s\n", messagein);

        /* check for quit */
        if (checkQuit(messagein)) {
            close(babyserverSock);
            exit(0);
            return true;
        }
        encode();

#ifdef DEBUG
        printf("server about to send message: %s\n", messageout);
#endif

        /* send the result message back to the client */
        send(babyserverSock, messageout, strlen(messageout), 0);

        /* clear out message strings again to be safe */
        bzero(messagein, MAX_MESSAGE_LENGTH);
        bzero(messageout, MAX_MESSAGE_LENGTH);
    }

    close(babyserverSock);
    exit(0);
}

void initServer() {
    /* Initialize server sockaddr structure */
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(MYPORTNUM);
    server.sin_addr.s_addr = htonl(INADDR_ANY);

    /* set up the transport-level end point to use TCP */
    if( (serverSock = socket(PF_INET, SOCK_STREAM, 0)) == -1 )
    {
        fprintf(stderr, "UH OH: socket() call failed!\n");
        exit(1);
    }

    /* bind a specific address and port to the end point */
    if( bind(serverSock, (struct sockaddr *)&server, sizeof(struct sockaddr_in) ) == -1 )
    {
        fprintf(stderr, "UH OH: bind() call failed!\n");
        exit(1);
    }

    serverListen();

    /* initialize message strings just to be safe (null-terminated) */
    bzero(messagein, MAX_MESSAGE_LENGTH);
    bzero(messageout, MAX_MESSAGE_LENGTH);

    fprintf(stderr, "Server started\n\n");
    fprintf(stderr, "listening on port %d...\n\n", MYPORTNUM);
}

bool checkQuit(char in[]) {
    //compares input char[] to 'QUIT' and returns true if so
    int i = -1;
    char quit[] = "QUIT";
    while ( ++i < 4 )
        if ( in[i] != quit[i]) return false;

    //send(serverSock, "quitting...\n", 12, 0);
    printf("quitting...\n\n");
    return true;
}

void safetyNet () {
    //carey wrote this next part, it looks to me like something I'll be thankful to leave in here, so i am
    /* Set up a signal handler to catch some weird termination conditions. */
    act.sa_handler = catcher;
    sigfillset(&(act.sa_mask));
    sigaction(SIGPIPE, &act, NULL);

}

bool acceptConnection() {
    /* accept a connection, this is where serverSock is initialized */
    if( (babyserverSock = accept(serverSock, NULL, NULL)) == -1 ) {
        /*these next lines are kinda annoying, because the server is
        automatically gonna try again */
        //fprintf(stderr, "UH OH: accept() call failed!\n");
        //fprintf(stderr, "%d\n", errno);
        return false;
    }
    return true;
}

void serverListen() {
    /* start listening for incoming connections from clients */
    if( listen(serverSock, 5) == -1 )
    {
        fprintf(stderr, "UH OH: listen() call failed!\n");
        exit(1);
    }

}

void encode() {
    /* calls Encyptor and places output in message out buffer */
    sprintf(messageout, "%s", e.transform(messagein) );
}

eType handleArgs(int argc, const char * argv[]) {
    /* DEBUG: print all arguments (including 0, file name) */
#ifdef FULLDEBUG
    printf("recieved args\n");
    for (int i = 0; i < argc; i++)
        printf("%s ", argv[i]);
    printf("\n\n");
#endif
    eType ret = CHECKSUM;
    // check for at least NUM_ARGS arguments and the first is either -seq , -sum, or -chksum
    if (argc < NUM_ARGS ) {
        printf("Invalid arguments. default hash is internet checksum...");
        ret = CHECKSUM;
    }
    else if (!strcmp(argv[FLAG_ARG],"-seq")) ret = SEQUENCE;
    else if (!strcmp(argv[FLAG_ARG],"-sum")) ret = WORD_SUM;
    else if (!strcmp(argv[FLAG_ARG],"-chksum")) ret = CHECKSUM;
    return ret;
}
