#include <string.h>
#include "jsmn.h"

#ifndef WASPHOUSECENTER_PROTOCOL_H
#define WASPHOUSECENTER_PROTOCOL_H

#endif //WASPHOUSECENTER_PROTOCOL_H

static int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
    if (tok->type == JSMN_STRING && (int) strlen(s) == tok->end - tok->start &&
        strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
        return 0;
    }
    return -1;
}


typedef struct {
    char *metaData;
    int valuesSize;
    float *values;
} Data;

typedef struct {
    char* name;
    int id;
    int roomID;
} Device;


int json_to_device(Device *device, Data *data, char *json);
int parseTokenToInteger(char* json, jsmntok_t jsmntok);
int char_array_to_integer(char* array, int size);
char *parseTokenToString(char* json, jsmntok_t jsmntok);
char *fetch(char *array, int from, int to);
char *deviceToJSON(Device device);
char *dataToJSON(Data data);
float parseTokenToFloat(char* json, jsmntok_t jsmntok);
void replaceString(char *first, int startAt, char *second, int secondSize);
void freeDevice(Device device);
void freeData(Data data);
char *getJSON(Device device, Data data);
char *integerToString(int integer);
char *floatToString(float f);
