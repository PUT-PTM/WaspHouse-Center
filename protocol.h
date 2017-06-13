#include <string.h>
#include "jsmn.h"

#ifndef WASPHOUSECENTER_PROTOCOL_H
#define WASPHOUSECENTER_PROTOCOL_H

#endif //WASPHOUSECENTER_PROTOCOL_H

typedef struct {
    int systemID;     	// 3 znaki
    int roomID; 		// 3 znaki
    int order;  	    // 3 znaki
    int value;			// 3 znaki
} JSON; // ³¹cznie 54 znaki example: {"order":100,"systemID":111,"roomID":222,"value":100}

int charArrayToJSON(JSON *structure, char *tab);
int parseTokenToInteger(char* json, jsmntok_t jsmntok);
char *JSONToCharArray(JSON json);
