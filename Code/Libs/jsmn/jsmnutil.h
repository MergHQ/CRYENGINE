#ifndef _JSMNUTIL_H_INCLUDED
#define _JSMNUTIL_H_INCLUDED

#include <jsmn.h>

#ifdef __cplusplus
extern "C" {
#endif

const jsmntok_t* jsmnutil_next(const jsmntok_t tokens[]);
const jsmntok_t* jsmnutil_array(const jsmntok_t tokens[], int i);
const jsmntok_t* jsmnutil_object(const char* js, const jsmntok_t tokens[], const char* key);
const jsmntok_t* jsmnutil_xpath(const char* js, const jsmntok_t tokens[], ...);

#ifdef __cplusplus
}
#endif

#endif //_JSMNUTIL_H_INCLUDED