#include "Lex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>


static const char *build_file = NULL;

static char *src = NULL;

static uint64_t line = 1;

static uint64_t column = 0;


static char LexCh()
{
	char ch = *++src;
	column++;
	return ch;
}

static void LexPutback()
{
	src--;
	column--;
}

static void LexNL()
{
	line++;
	column = 0;
}


uint64_t Lex(struct Token *tok)
{
	memset(tok, 0, sizeof(struct Token));
	char ch = LexCh();

	while(isspace(ch)) {
		if(ch == '\n')
			LexNL();

		ch = LexCh();
	}

	switch(ch)
	{
	case '\0':
		tok->token = TK_EOF;
		break;
	case '_':
	case 'a'...'z':
	case 'A'...'Z':
		tok->token = TK_IDENT;

		while(1) {
			if(!isalnum(ch) && ch != '_')
				break;

			if(tok->len > 255)
				LexError("Identifier too long (> 255)");

			tok->str[tok->len++] = ch;
			ch = LexCh();
		}

		tok->str[tok->len] = '\0';
		LexPutback();
		break;
	case '"':
		tok->token = TK_STRING;

		while(1) {
			if(LexCh() == '"')
				break;
			else
				LexPutback();

			if(tok->len > 255)
				LexError("Identifier too long (> 255)");

			ch = LexCh();
			tok->str[tok->len++] = ch;
		}

		tok->str[tok->len] = '\0';
		break;
	case '<':
		tok->token = TK_FILE;

		while(1) {
			if(LexCh() == '>')
				break;
			else
				LexPutback();

			if(tok->len > 255)
				LexError("File name too long (> 255)");

			ch = LexCh();
			tok->str[tok->len++] = ch;
		}

		tok->str[tok->len] = '\0';
		break;
	case '[':
		tok->token = TK_START;
		break;
	case ']':
		tok->token = TK_END;
		break;
	default:
		LexError("Unknown character in build file");
	}

	return tok->token;
}

void LexSource(const char *file_name)
{
	FILE *f = fopen(file_name, "r");

	if(f == NULL) {
		printf("Couldn't open build file '%s'\n", file_name);
		exit(1);
	}

	fseek(f, 0, SEEK_END);
	uint64_t size = ftell(f);
	fseek(f, 0, SEEK_SET);
	char *buf = malloc(size + 1);
	buf[size] = '\0';

	if(fread(buf, size, 1, f) != 1) {
		printf("Couldn't read build file '%s'\n", file_name);
		free(buf);
		fclose(f);
		exit(1);
	}

	fclose(f);

	for(uint64_t i = 0; i < size; i++) {
		if(!isascii(buf[i])) {
			printf("Build file '%s' includes non printable characters\n", file_name);
			free(buf);
			exit(1);
		}
	}

	src = buf - 1;
	build_file = file_name;
}

void LexError(const char *fmt, ...)
{
	printf("%s:%lu:%lu | ", build_file, line, column);
	va_list ap;
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
	putchar('\n');
	exit(1);
}

char *TokenName(struct Token *tok)
{
	char *str = malloc(256);

	switch(tok->token)
	{
	case TK_IDENT:
		snprintf(str, 255, "%s", tok->str);
		break;
	case TK_STRING:
		snprintf(str, 255, "\"%s\"", tok->str);
		break;
	case TK_FILE:
		snprintf(str, 255, "<%s>", tok->str);
		break;
	case TK_START:
		return "'['";
	case TK_END:
		return "']'";
	case TK_EOF:
		return "EOF";
	}

	return str;
}
