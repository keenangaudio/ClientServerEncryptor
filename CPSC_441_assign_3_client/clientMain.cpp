//
//  clientMain.cpp
//  CPSC_441_assign_3_server
//
//  adapted from
//
//  wordlengthclient.c
//  by Carey Williamson on January 13, 2012
//
//  by Keenan on 2018-10-15.
//
//  original info:

/* TCP-based client example of socket programming.    */
/* This client interacts with the word length server, */
/* which needs to be running first.                   */
/*                                                    */
/* Written by Carey Williamson       January 13, 2012 */


#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

/* Some generic error handling stuff */
extern int errno;
void perror(const char *s);

/* Manifest constants used by client program */
#define MAX_HOSTNAME_LENGTH 64
#define MAX_TWEET_LENGTH 14100
#define BYNAME 1
#define MYPORTNUM 12345   /* must match the server's port! */

#define FLAG_ARG 1
#define HOSTNAME_ARG 2

/* Menu selections */
#define ALLDONE 0
#define ENTER 1

/* headers */
void handleArgs(int argc, const char * argv[]);
void initClient(int argc, const char * argv[]);
void parseHostName(int argc, const char * argv[]);
void interact();

/* global variables */
int sockfd, sockfd2;
char c;
struct sockaddr_in server;
struct hostent *hp;
char hostname[MAX_HOSTNAME_LENGTH];
char message[MAX_TWEET_LENGTH];
char messageback[MAX_TWEET_LENGTH];
int choice, len;
ssize_t bytes;

/* Prompt the user to enter a word */
void printmenu()
{
    printf("\n");
    printf("Please choose from the following selections:\n");
    printf("  1 - Enter a word\n"); // ENTER
    printf("  0 - Exit program\n"); // ALLDONE
    printf("Your desired menu selection? ");
}

/* Main program of client */
int main(int argc, const char * argv[])
{
    handleArgs(argc, argv);
    initClient(argc, argv);
    
    /* Print welcome banner */
    printf("ready to send message to server\n\n");
    printmenu();
    scanf("%d", &choice);

    /* main loop: read a word, send to server, and print answer received */
    while( choice != ALLDONE)
    {
        if( choice == ENTER )
        {
            interact();
        }
        else printf("Invalid menu selection. Please try again.\n");

        printmenu();
        scanf("%d", &choice);
    }
    char* out = (char*) "QUIT";
    send(sockfd, out, 4, NULL);
    /* Program all done, so clean up and exit the client */
    close(sockfd);
    exit(0);
}

void initClient (int argc, const char * argv[]) {
    /* Initialization of server sockaddr data structure */
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(MYPORTNUM);
    server.sin_addr.s_addr = htonl(INADDR_ANY);

    parseHostName(argc, argv);

    /* create the client socket for its transport-level end point */
    if( (sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1 )
    {
        fprintf(stderr, "looks like the socket() call failed!\n");
        exit(1);
    }

    /* connect the socket to the server's address using TCP */
    if( connect(sockfd, (struct sockaddr *)&server, sizeof(struct sockaddr_in)) == -1 )
    {
        fprintf(stderr, "looks like the connect() call failed!\n");
        perror(NULL);
        exit(1);
    }

}
void parseHostName(int argc, const char * argv[]) {
    //if hostname -h is specified
    if (!strcmp(argv[FLAG_ARG], "-h")) {
        strcpy(hostname, argv[HOSTNAME_ARG]);
        hp = gethostbyname(hostname);
        if (hp == NULL){
            fprintf(stderr, "%s: unknown host\n", hostname);
            exit(1);
        }
        /* copy the IP address into the sockaddr structure */
        bcopy(hp->h_addr, &server.sin_addr, hp->h_length);
    } else if (!strcmp(argv[FLAG_ARG], "-ip")) {   //if -ip is specified
        server.sin_addr.s_addr = inet_addr(argv[HOSTNAME_ARG]);
    }
}

void handleArgs(int argc, const char * argv[]) {
    /* DEBUG: print all arguments (including 0, file name) */
#ifdef DEBUG
    printf("recieved args\n");
    for (int i = 0; i < argc; i++)
        printf("%s ", argv[i]);
    printf("\n\n");
#endif
    // check for 2 arguments and the first is either -h or -ip
    if (argc != 3 || (strcmp(argv[FLAG_ARG],"-h") && strcmp(argv[FLAG_ARG],"-ip")) ) {
        printf("Invalid arguments. try sending a hostname or IP address");
        exit(1);
    }
}

void interact(){
    /* get rid of newline after the (integer) menu choice given */
    c = getchar() ;

    /* prompt user for the input */
    printf("Enter your tweet: ");
    len = 1;
    message[0] = ' ';   //initializes as space, will be ignored by server anyways
    while( (c = getchar()) != '\n' ){
        message[len] = c;
        len++;
    }
    /* make sure the message is null-terminated in C */
    message[len] = '\0';

    /* send it to the server via the socket */
    send(sockfd, message, len, 0);

    /* see what the server sends back */
    if( (bytes = recv(sockfd, messageback, MAX_TWEET_LENGTH, 0)) > 0 )
    {
        /* make sure the message is null-terminated in C */
        messageback[bytes] = '\0';
        printf("Encrypted tweet: ");
        printf("%s\n", messageback);
    }
    else
    {
        /* an error condition if the server dies unexpectedly */
        printf("Sorry, dude. Server failed!\n");
        close(sockfd);
        exit(1);
    }
}
