#pragma once

#include <stddef.h>
#include <stdint.h>


#define TK_IDENT  0
#define TK_STRING 1
#define TK_FILE   2
#define TK_START  3
#define TK_END    4
#define TK_EOF    5


struct Token
{
	uint64_t token;
	uint64_t   len;
	char       str[256];
};


uint64_t Lex(struct Token *tok);

void LexSource(const char *file_name);

void LexError(const char *fmt, ...);

char *TokenName(struct Token *tok);
