#include <string.h>
#include <malloc.h>
#include "jsmn.h"
#include "protocol.h"

int jsoneq(char *json, jsmntok_t *tok, char *s) {
	if (tok->type == JSMN_STRING && (int) strlen(s) == tok->end - tok->start &&
			strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
		return 0;
	}
	return -1;
}

int charArrayToJSON(JSON *structure, char *json) {
    int i;
    int r;
    jsmn_parser p;
    jsmntok_t t[32];

    jsmn_init(&p);
    r = jsmn_parse(&p, json, strlen(json), t, sizeof(t) / sizeof(t[0]));
    if (r < 0) {
        printf("Failed to parse JSON: %d\n", r);
        return 1;
    }

    if (r < 1 || t[0].type != JSMN_OBJECT) {
        printf("Object expected\n");
        return 1;
    }

    /* Loop over all keys of the root object */
    for (i = 1; i < r; i++) {
    	if (jsoneq(json, &t[i], "order") == 0) {
    		structure->order = parseTokenToInteger(json, t[i+1]);
            i++;
        } else if (jsoneq(json, &t[i], "systemID") == 0) {
        	structure->systemID = parseTokenToInteger(json, t[i+1]);

            i++;
        } else if (jsoneq(json, &t[i], "roomID") == 0) {
        	structure->roomID = parseTokenToInteger(json, t[i+1]);
            i++;
        } else if (jsoneq(json, &t[i], "value") == 0) {
        	structure->value = parseTokenToInteger(json, t[i+1]);
        	i++;
        } else {
            //printf("Unexpected key: %s\n", t[i].end-t[i].start, json + t[i].start); sprawdzic czemu
        }
    }
    return 0;
}

int parseTokenToInteger(char* json, jsmntok_t jsmntok) {
    char tab[4];
    tab[0] = json[jsmntok.start];
    tab[1] = json[jsmntok.start + 1];
    tab[2] = json[jsmntok.start + 2];
    char *reszta;
    return strtol(tab, &reszta, 0);
}

char *JSONToCharArray(JSON structure) {
    char *json = malloc(sizeof(char) * 55);
    sprintf(json, "{\"order\":%d,\"systemID\":%d,\"roomID\":%d,\"value\":%d}\r\n", structure.order, structure.systemID, structure.roomID, structure.value);
    return json;
}
