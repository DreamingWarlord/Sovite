#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Parse.h"
#include "Lex.h"


static struct Token tok_s;

static struct Token *tok = &tok_s;


static uint64_t ParseDepList(char ***depsp)
{
	if(tok->token != TK_START)
		LexError("Expected '[', got %s", TokenName(tok));

	uint64_t count = 0;
	char **deps = calloc(256, sizeof(char *));

	while(1) {
		if(Lex(tok) == TK_END)
			break;

		if(tok->token != TK_FILE)
			LexError("Expected dependency file name, got %s", TokenName(tok));

		if(count > 255)
			LexError("Exceeded maximum dependency count (> 255)");

		deps[count++] = strdup(tok->str);
	}

	Lex(tok);
	*depsp = deps;
	return count;
}

static void ParseBuildItem(struct BuildItem *item)
{
	if(tok->token != TK_FILE)
		LexError("Expected file name, got %s", TokenName(tok));

	item->name = strdup(tok->str);

	if(Lex(tok) == TK_START)
		item->dep_count = ParseDepList(&item->deps);
}

static uint64_t ParseBuildItemList(struct BuildItem **listp)
{
	if(tok->token != TK_START)
		LexError("Expected '[', got %s", TokenName(tok));

	uint64_t count = 0;
	struct BuildItem *list = calloc(256, sizeof(struct BuildItem));
	Lex(tok);

	while(1) {
		if(tok->token == TK_END)
			break;

		if(count > 255)
			LexError("Too many dependencies (> 255)");

		ParseBuildItem(&list[count++]);
	}

	Lex(tok);
	*listp = list;
	return count;
}

static void ParseConstDef(struct BuildConst *bc)
{
	if(tok->token != TK_IDENT)
		LexError("Expected constant name, got %s", TokenName(tok));

	bc->name = strdup(tok->str);

	if(Lex(tok) != TK_STRING)
		LexError("Expected constant string, got %s", TokenName(tok));

	bc->val = strdup(tok->str);
	Lex(tok);
}


struct BuildFile *ParseBuildFile(const char *file_name)
{
	LexSource(file_name);
	Lex(tok);
	struct BuildFile *bf = calloc(1, sizeof(struct BuildFile));
	bf->consts = calloc(256, sizeof(struct BuildConst));

	while(1) {
		if(tok->token == TK_EOF)
			LexError("Expected targets, got EOF");

		if(tok->token == TK_IDENT && !strcmp(tok->str, "targets"))
			break;

		if(bf->const_count > 255)
			LexError("Too many constants (> 255)");

		ParseConstDef(&bf->consts[bf->const_count++]);
	}

	Lex(tok);
	bf->target_count = ParseBuildItemList(&bf->targets);

	if(tok->token != TK_IDENT || strcmp(tok->str, "dependencies"))
		LexError("Expected dependencies, got %s", TokenName(tok));

	Lex(tok);
	bf->dep_count = ParseBuildItemList(&bf->deps);

	if(tok->token != TK_EOF)
		LexError("Expected end of file, got %s", TokenName(tok));

	return bf;
}
