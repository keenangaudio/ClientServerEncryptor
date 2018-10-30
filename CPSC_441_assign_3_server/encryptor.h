//
//  encryptor.h
//  CPSC_441_assign_3
//
//  Created by Keenan on 2018-10-16.
//  Copyright © 2018 Keenan. All rights reserved.
//

#ifndef encryptor_h
#define encryptor_h

#include <stdio.h>
#include <stdint.h>
#include <unordered_map>
#include <array>

#define MAX_MESSAGE_LENGTH 14100  // a tweet has 140 characters + '\n'

#define SEQUENCE 0
#define WORD_SUM 1
#define CHECKSUM 2

/* Optional verbose debugging output */
//#define DEBUG 1
//#define FULLDEBUG 1
//#define INSIDE 1

// this is so my program knows how to decode the bonus (so its in te dictionary)
#define INITIAL_COMMIT (char*) " There is NO business like SHOW business"

typedef unsigned short int eType;
typedef unsigned long int ulong;

class Encryptor {
public:
    Encryptor();
    Encryptor(eType type);
    void setEncrytionAlgorithm(eType type);
    void operator=(const Encryptor& other);
    /* decides whether to decrypt or encrypt, based on input format */
    char* transform(char*);
private:
    /* stores encryption type according to above macros */
    eType encryptionMethod;
    /* keeps track of the sequence hash */
    uint16_t seqnum;
    /* the dictionary, using an unordered_map to increase speed */
    std::unordered_map< uint16_t, char* > encounteredHashes;

    /*  splits the input string into words and returns string array
     *  with wordNum + 1 elements
     */
    char** splitWords(char*, int& wordNum);
    char* encrypt(char*);
    char* decrypt(char*);

    /* this fixes strings by making sure the characters are valid */
    void check(char*);

    uint16_t selectHash(char*);
    /*  searches dictionary for a word matching in
     */
    uint16_t checkForWord(char* in);
    /*  The following functions are the respective hashing
     *  algorithms with collision detection
     */
    uint16_t sequential(char*);
    uint16_t wordSum(char*);
    uint16_t internetCheckSum(char*);

};

#endif /* encryptor_h */
