#include <string.h>
#include <malloc.h>
#include "jsmn.h"
#include "protocol.h"

void freeData(Data data) {
    free(data.values);
    free(data.metaData);
}

void freeDevice(Device device) {
    free(device.name);
}

int json_to_device(Device *device, Data *data, char *json) {
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
        if (jsoneq(json, &t[i], "name") == 0) {
            device->name = parseTokenToString(json, t[i+1]);
            i++;
        } else if (jsoneq(json, &t[i], "id") == 0) {
            device->id = parseTokenToInteger(json, t[i + 1]);
            i++;
        } else if (jsoneq(json, &t[i], "roomID") == 0) {
            device->roomID = parseTokenToInteger(json, t[i+1]);
            i++;
        } else if (jsoneq(json, &t[i], "metaData") == 0) {
            data->metaData = parseTokenToString(json, t[i+1]);
            i++;
        } else if (jsoneq(json, &t[i], "valuesSize") == 0) {
            data->valuesSize = parseTokenToInteger(json, t[i+1]);
            i++;
        } else if (jsoneq(json, &t[i], "values") == 0) {
            int j;
            data->values = malloc(sizeof(float) * t[i+1].size);
            for (j = 0; j < t[i+1].size; j++) {
                data->values[j] = parseTokenToFloat(json, t[i+j+2]);
            }

            i += t[i+1].size + 1;
        } else {
            //printf("Unexpected key: %.*s\n", t[i].end-t[i].start, json + t[i].start);
        }
    }
}


int parseTokenToInteger(char* json, jsmntok_t jsmntok) {
    char* array = parseTokenToString(json, jsmntok);
    return char_array_to_integer(array, jsmntok.end - jsmntok.start);
}

float parseTokenToFloat(char* json, jsmntok_t jsmntok) {
    char* array = parseTokenToString(json, jsmntok);
    float value = 0;
    int dotAt = 0;
    int i = 0;
    while(array[dotAt] != '.')
        dotAt++;
    value += char_array_to_integer(fetch(array, 0, dotAt), dotAt);
    dotAt++;
    int lessThan0 = 1;
    if(value < 0) {
        lessThan0 = -1;
    }
    while(dotAt + i < jsmntok.end - jsmntok.start) {
        float pow = 1;
        for(int j = dotAt; j < i + dotAt + 1; j++) {
            pow /= 10;
        }
        value += (array[dotAt+i] - 48) * pow * lessThan0;
        i++;
    }
    return value;
}

char* parseTokenToString(char* json, jsmntok_t jsmntok) {
    int size = jsmntok.end - jsmntok.start;
    char *tab = malloc(size * sizeof(char));
    for(int a = 0; a < size; a++) {
        tab[a] = json[a + jsmntok.start];
    }
    return tab;
}

char* fetch(char* array, int from, int to) {
    int size = to - from;
    char *tab = malloc(size * sizeof(char));
    for(int a = 0; a < size; a++) {
        tab[a] = array[a + from];
    }
    return tab;
}

int char_array_to_integer(char* array, int size) {
    int value = 0;
    int i = size;
    value += (array[i-1] - 48);
    i--;
    while(i > 0) {
        if(array[i-1] == '-') {
            value = -value;
            return value;
        }
        int pow = 1;
        for(int j = 0; j < size - i; j++) {
            pow  *= 10;
        }
        value += (array[i-1] - 48) * pow;
        i--;
    }
    return value;
}

char* getJSON(Device device, Data data) {
    char *json = malloc(sizeof(char) * 64);
    for(int i = 0; i < 52 + 79 + 3; i++) {
        json[i] = ' ';
    }
    int i = 0;
    replaceString(json, i, "{", 1); i+= 1;
    replaceString(json, i, deviceToJSON(device), 52); i+= 52;
    replaceString(json, i, ",", 1); i+= 1;
    replaceString(json, i, dataToJSON(data), 79); i+= 79;
    replaceString(json, i, "}", 1);
    return json;
}

// zwraca tablice znakow od dlugosci = 52
char *deviceToJSON(Device device) {
    int size = 52;
    char *json = malloc(sizeof(char) * size);
    int i = 0;
    replaceString(json, i, "\"device\":{\"name\":\"", 18); i += 18;
    replaceString(json, i, device.name, 10); i += 10;
    replaceString(json, i, "\",\"id\":", 7); i += 7;
    replaceString(json, i, integerToString(device.id), 3); i += 3;
    replaceString(json, i, ",\"roomID\":", 10); i += 10;
    replaceString(json, i, integerToString(device.roomID), 3); i += 3;
    replaceString(json, i, "}", 1);
    return json;
}

// zwraca tablice znakow od dlugosci = 79
char *dataToJSON(Data data) {
    int size = 79;
    char *json = malloc(sizeof(char) * size);
    int i = 0;
    replaceString(json, i, "\"data\":{\"metaData\":\"", 20); i += 20;
    replaceString(json, i, data.metaData, 10); i += 10;
    replaceString(json, i, "\",\"valuesSize\":", 15); i += 15;
    replaceString(json, i, integerToString(data.valuesSize), 1); i += 1;
    replaceString(json, i, ",\"values\":[", 11); i += 11;
    for(int j = 0; j < data.valuesSize; j++) {
        int chars = 0;
        int k = (int)data.values[j];
        if(k < 0) chars++;
        while(k > 0) {
            k /= 10;
            chars++;
        } chars += 7;
        replaceString(json, i, floatToString(data.values[j]), chars); i += chars;

        if(j < data.valuesSize-1) {
            replaceString(json, i, ",", 1); i += 1;
        }
    }
    replaceString(json, i, "]", 1); i++;
    replaceString(json, i, "}", 1);
    return json;
}

char *integerToString(int integer) {
    int chars = 0;
    int k = integer;
    while(k > 0) {
        k /= 10;
        chars++;
    }
    char *string = malloc(sizeof(char) * (chars));
    sprintf (string, "%d", integer);
    return string;
}

char *floatToString(float f) {
    int chars = 0;
    int k = (int)f;
    while(k > 0) {
        k /= 10;
        chars++;
    } chars += 7;
    char *string = malloc(sizeof(char) * (chars));
    sprintf (string, "%f", f);
    return string;
}

void replaceString(char *first, int startAt, char *second, int secondSize) {
    for(int i = 0; i < secondSize; i++) {
        first[i + startAt] = second[i];
    }
}
