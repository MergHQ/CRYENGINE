#include "jsmnutil.h"

#include <jsmn.h>

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <assert.h>

const jsmntok_t* jsmnutil_next(const jsmntok_t tokens[])
{
	const jsmntok_t* it= tokens + 1;
	int i;

	switch (tokens[0].type)
	{
	case JSMN_OBJECT:
		for (i = 0; i < tokens[0].size; i++)
		{
			assert(it->type == JSMN_STRING);
			it = jsmnutil_next(it + 1);
		}
		break;

	case JSMN_ARRAY:
		for (i = 0; i < tokens[0].size; i++)
			it = jsmnutil_next(it);
		break;
	
	case JSMN_STRING:
	case JSMN_PRIMITIVE:
		break;
	}

	return it;
}

const jsmntok_t* jsmnutil_array(const jsmntok_t tokens[], int i)
{
	const jsmntok_t* it= tokens + 1;
	if (tokens[0].type != JSMN_ARRAY)
		return NULL;

	if (i < 0 || tokens[0].size <= i)
		return NULL;

	while (i--)
		it= jsmnutil_next(it);

	return it;
}

const jsmntok_t* jsmnutil_object (const char* js, const jsmntok_t tokens[], const char* key)
{
	size_t klen = strlen(key);
	const jsmntok_t* it = tokens + 1;
	int i;

	if (tokens[0].type != JSMN_OBJECT)
		return NULL;

	for (i = 0; i < tokens[0].size; i++)
	{
		const char* str = js + it->start;
		assert(it->type == JSMN_STRING);		
		if (it->end - it->start == klen && memcmp(str, key, klen) == 0)
			return it + 1;

		it= jsmnutil_next(it + 1);
	}

	return NULL;
}

const jsmntok_t* jsmnutil_xpath(const char* js, const jsmntok_t tokens[], ...)
{
	const jsmntok_t* it = tokens;
	va_list ap;
	va_start(ap, tokens);
		
	do
	{
		const char* key = va_arg(ap, const char*);
		if (key == NULL)
			break;

		switch (it->type)
		{
		case JSMN_OBJECT:
			it = jsmnutil_object(js, it, key);
			break;

		case JSMN_ARRAY:
			it = isdigit(key[0]) ? jsmnutil_array(it, atoi(key)) : NULL;
			break;

		case JSMN_PRIMITIVE:
		case JSMN_STRING:
			it = NULL;
			break;
		}


	} while (it != NULL);

	va_end(ap);
	return it;
}
