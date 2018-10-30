//
//  encryptor.cpp
//  CPSC_441_assign_3_server
//
//  Created by Keenan on 2018-10-16.
//  Copyright © 2018 Keenan. All rights reserved.
//

#include "encryptor.h"
#include <stdlib.h>
#include <string>
#include <string.h>
#include <iostream>
#include <stdexcept>

Encryptor::Encryptor(){
    //initialize map
    seqnum = 1;
    char* zero = new char();
    memcpy(zero, "\0", 1);
    encounteredHashes = {{0x00,zero}};  //set 0 to 0, because it seems like the kind of thing to cause problems
}
//just assignment operator, never has more than one instance so i didnt worry about copies
void Encryptor::operator=(const Encryptor& other) {
    encryptionMethod = other.encryptionMethod;
    seqnum = other.seqnum;
    encounteredHashes = other.encounteredHashes;
}

Encryptor::Encryptor(eType type) : Encryptor() {
    setEncrytionAlgorithm(type);
}

void Encryptor::setEncrytionAlgorithm(eType type){
    encryptionMethod = type;
}
char* Encryptor::transform(char* begin){
    //picks encryption or decryption based on first word
    char* in = begin;
    char* delim = (char*) "0x";
    while( !isalnum(*in) ) in++;    //find first letter / number

    bool isEncrypted = true;
    for(; *delim && *in; delim++, in++) if (*in != *delim) {
        isEncrypted = false;
        break;
    }
    if (isEncrypted)
        return decrypt( begin );
    else
        return encrypt( begin );
}
char* Encryptor::encrypt(char* in){
#ifdef INSIDE
    printf("in encrypt\n");
#endif
    int wordNum, totalSize;
    char** wordArr = splitWords(in, wordNum);
    totalSize = wordNum + (int) strlen(in);

    char* tmp = (char*) malloc(totalSize + 1);  //the output c string
    char* tm2 = (char*) malloc(totalSize + 1);  //space enough for the entire tweet to be one word (overkill)
    bzero(tmp, totalSize + 1);
    bzero(tm2, totalSize + 1);

 // iterate over all words in incoming c string array and hash each one,
 // then store in memory to be kept in map
    for(int i = 0; i <= wordNum; i++){
        uint16_t wordHash = selectHash(wordArr[i]);
        memcpy(tm2, tmp, totalSize + 1);
        sprintf(tmp, "%s 0x%04x", tmp, wordHash);
#ifdef FULLDEBUG
        printf("appending 0x%04x to tmp(%d)\n", wordHash, wordHash);
#endif
    }
    free( wordArr );

    for ( auto it = encounteredHashes.begin(); it != encounteredHashes.end(); ++it )
        std::cout << " " << it->first << ":" << it->second << std::endl;
    std::cout << std::endl;

    free(tm2);
    return tmp; // return concatenated hashes
}

uint16_t Encryptor::selectHash(char* in){
#ifdef INSIDE
    printf("in hash\n");
#endif

    switch (encryptionMethod) {
        case SEQUENCE:
           return sequential(in);
        case WORD_SUM:
            return wordSum(in);
        case CHECKSUM: default:
            return internetCheckSum(in);
    }
}

char* Encryptor::decrypt(char* in){
#ifdef INSIDE
    printf("in decrypt\n");
#endif
    int wordNum;
    char** words = splitWords(in, wordNum);
    std::string outputContainer = "";

    for (int wordIndex = 0; wordIndex <= wordNum; wordIndex++){
        //all words start with 0x, so we ignore those two
        std::string tmpWord = words[wordIndex];
        size_t delim_length = 0;
        tmpWord.shrink_to_fit();
        // convert to int
        uint16_t hash;
        try {
            hash = (uint16_t) std::stoi(tmpWord, &delim_length, 16);
        } catch( std::invalid_argument e) {
            printf("thats a big ol invalid arg: %d\n%s\n",hash, e.what());
        }

#ifdef FULLDEBUG
        std::cout << "printing the dictionary" << std::endl;
        for ( auto it = encounteredHashes.begin(); it != encounteredHashes.end(); ++it )
            std::cout << " " << it->first << ":" << it->second << std::endl;
        std::cout << std::endl;
        printf("searching for word which hashes to 0x%04x (%d)\n", hash, hash);
#endif
        //search map for hash
        char* foundWord;
        try{
            foundWord = encounteredHashes.at( hash );
        } catch (std::out_of_range& e) {
            foundWord = (char*) "[unknown]";
        }
        //add a space and the word to output
        outputContainer.append(" ");
        outputContainer.append(foundWord);
#ifdef FULLDEBUG
        printf("appended word %2d (hash 0x%04x): %s to outputContainer\n", wordIndex, hash, foundWord);
#endif
    }
    free( words );
    ulong len = outputContainer.length()+1;
    char* tmp = (char*) malloc(sizeof(char)*len);
    bzero(tmp, sizeof(char) * len );
    memcpy(tmp, outputContainer.c_str(), len);
    return tmp;
}

uint16_t Encryptor::sequential(char* in){

#ifdef INSIDE
    printf("in sequential\n");
#endif
    ulong wordlen = strlen(in) + 1;
    char* store = (char*) malloc(sizeof(char) * wordlen);
    bzero(store, sizeof(char) * wordlen);
    memcpy(store, in, wordlen);

    int hash;
    if ((hash = checkForWord(in)) == 0){
        hash = ++seqnum;
//        auto search = encounteredHashes.find(seqnum);
//        if (search == encounteredHashes.end())
        encounteredHashes.insert({seqnum,store});
    } else {
#ifdef FULLDEBUG
        printf("storing %s with hash 0x%04x\n",store,hash);
#endif
        free(store);
    }
    return hash;
}

uint16_t Encryptor::wordSum(char* in){

#ifdef INSIDE
    printf("in wordsum\n");
#endif
    ulong wordlen = strlen(in) + 1;
    char* store = (char*) malloc(sizeof(char) * wordlen);
    bzero(store, sizeof(char) * wordlen);
    memcpy(store, in, wordlen);

    int hash;
    // if in exists in encounteredHashes, use that index instead of generating one
    if ((hash = checkForWord(in)) == 0){
        hash = 0;
        //sum chars in word
        char* inptr = in;
        int cur;
        while((cur = *(inptr++))) hash += cur;

        // if hash results in collision, add 1 until a space is available
#ifdef FULLDEBUG
        printf("***looking for collisions with 0x%04x***\n", hash);
        for ( auto it = encounteredHashes.begin(); it != encounteredHashes.end(); ++it )
            std::cout << " " << it->first << ":" << it->second;
        std::cout << std::endl;
#endif
        while ( encounteredHashes.find(hash) != encounteredHashes.end() ){
            hash = (uint16_t) (hash + 1);
#ifdef FULLDEBUG
            printf("found, moving to hash: 0x%04x\n",hash);
#endif
        }

        encounteredHashes.insert({hash,store});
#ifdef FULLDEBUG
        printf("storing %s with hash 0x%04x\n",store,hash);
#endif
    } else {
#ifdef FULLDEBUG
        printf("found %s with hash 0x%04x, no store necessary.\n",store,hash);
#endif
        free(store);
    }
    return hash;

}

uint16_t Encryptor::internetCheckSum(char* start){
#ifdef INSIDE
    printf("in wordsum\n");
#endif
    char* in = start;
    ulong wordlen = strlen(in) + 1;
    char* store = (char*) malloc(sizeof(char) * wordlen);
    bzero(store, wordlen);
    memcpy(store, in, wordlen);
    uint16_t hash;

    // if in exists in encounteredHashes, use that index instead of generating one
    if ((hash = checkForWord(in)) == 0){
        //internet checksum
        uint32_t total = 0;
        while ( *in ){
            uint32_t num1 = *in,        // this char
                     num2 = *(in + 1);  // next char
            total += (num1 << 8) + num2;    // gives [firstcharbits][secondcharbits]
            total = total + (total >> 16);  // adds carry bits to ones place
            total &= UINT16_MAX;            // removes carries
            in += 2;
        }
        uint16_t out = (uint16_t) total;
        hash = 0xffff - out;
        hash = hash & 0xffff;

#ifdef FULLDEBUG
        printf("***looking for collisions with 0x%04x (%d)***\n", hash,hash);
        for ( auto it = encounteredHashes.begin(); it != encounteredHashes.end(); ++it )
            std::cout << " " << it->first << ":" << it->second << std::endl;
        std::cout << std::endl;
#endif
        while ( encounteredHashes.find(hash) != encounteredHashes.end() ){
            hash = (uint16_t) (hash + 1);

#ifdef FULLDEBUG
            printf("found, moving to hash: 0x%04x\n",hash);
#endif
        }
        hash = hash & 0xffff;

        encounteredHashes.insert({hash,store});
#ifdef FULLDEBUG
        printf("storing %s with hash 0x%04x\n",store,out);
#endif
    } else {
#ifdef FULLDEBUG
        printf("found %s with hash 0x%04x, no store necessary.\n",store,hash);
#endif
        free(store);
    }
    return hash;
}

char** Encryptor::splitWords(char* in, int& wordNum) {
#ifdef INSIDE
    printf("splitting words\n");
#endif
    std::string split = " ";
    std::string message = in;
    int i = 0, tokenCount = 0;
    /* counts number of spaces */
    while ( in[i] ){
        if (in[i] == split.c_str()[0] ) tokenCount++;
        i++;
    }
#ifdef FULLDEBUG
    printf("in has %d spaces, and %d characters\n",tokenCount,i);
#endif
    wordNum = 0;
    size_t first = 0, second = 0;

    char** retChar = (char**) malloc( (sizeof(char*))*(tokenCount + 1) );
    bzero(retChar, (sizeof(char*)) * (tokenCount+1) );
// splits into words based on spaces
    while ( (second = message.find(split, first)) != std::string::npos ){
        if (wordNum > tokenCount) break; //just in case
        std::string tok = message.substr(first, second-first);
        char* tmp = (char*) malloc( sizeof(char) * (tok.length() + 1) );
        bzero(tmp, sizeof(char) * (tok.length()+1) );
        memcpy( tmp, tok.c_str(), (tok.length()+1) );
        retChar[wordNum] = tmp;
#ifdef FULLDEBUG
        printf("adding %s to char** @ retChar[%d]\n", tmp,wordNum);
#endif
        wordNum++;
        first = second + 1; //continue after space after word
    }
    //one more, last word not followed by a space
    std::string tok = message.substr(first, message.length() - first);
    char* tmp = (char*) malloc( sizeof(char) * (tok.length()+1) );
    bzero(tmp, sizeof(char) * (tok.length()+1) );
    memcpy( tmp, tok.c_str(), (tok.length()+1) );
    check(tmp); //sometimes we got some weid characters popping up so this fixes it
    retChar[wordNum] = tmp;
#ifdef FULLDEBUG
    printf("adding %s to char** @ retChar[%d]\n", tmp,wordNum);
#endif

    retChar[wordNum+1] = NULL;  // null terminate array, just in case
#ifdef FULLDEBUG
    printf("wordCount: %d   TokenCount: %d\n",wordNum,tokenCount);
    printf("split:\n");
    int j = -1;
    while(++j <= wordNum)
        printf("retChar[%d] : %s\n",j,retChar[j]);
#endif
    return retChar;
}

uint16_t Encryptor::checkForWord(char* in) {
    for(auto i : encounteredHashes){
        uint16_t hash = i.first;
        char* value = i.second;
        if (!strcmp(in, value)) return hash;
    }

    return 0;   //nothing should hash here
}

void Encryptor::check (char* in) {
    int i = 0;
    while (in[i]){
        //if not usable ascii (invisible chars are the worst, so i remove them)
        // I think they get introduced from heap copies, but i dont really have the time to find out why
        if ((in[i] > 126 || in[i] < 33 ))
            /* this is a beep character, it doesnt print but it makes a noise
             which is SO FUN, so im using it to simplify the strings to std ascii*/
            in[i] = '\0';   //jk it just ends the word
        i++;
    }
}
