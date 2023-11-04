#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "Lex.h"
#include "Parse.h"
#include "Exec.h"


void PrintItem(struct BuildItem *item)
{
	printf("\t<%s> [ ", item->name);

	for(uint64_t i = 0; i < item->dep_count; i++)
		printf("<%s> ", item->deps[i]);

	printf("]\n");
}

int main(int argc, char *argv[])
{
	struct BuildFile *bf = ParseBuildFile("Sovite.build");

	if(argc > 1 && !strcmp(argv[1], "clean")) {
		char *bin_name = NULL;

		for(uint64_t i = 0; i < bf->const_count; i++) {
			if(!strcmp(bf->consts[i].name, "bin_dir")) {
				bin_name = bf->consts[i].val;
				break;
			}
		}

		if(bin_name[strlen(bin_name) - 1] == '/')
			bin_name[strlen(bin_name) - 1] = '\0';

		char *cmd = calloc(256, 1);
		snprintf(cmd, 256, "rm -f %s/*.o", bin_name);
		printf("$ %s\n", cmd);
		system(cmd);
		return 0;
	}

	/*
	printf("Build file constants:\n");

	for(uint64_t i = 0; i < bf->const_count; i++)
		printf("\t%s : \"%s\"\n", bf->consts[i].name, bf->consts[i].val);

	printf("Build file targets:\n");

	for(uint64_t i = 0; i < bf->target_count; i++)
		PrintItem(&bf->targets[i]);

	printf("Build file dependencies:\n");

	for(uint64_t i = 0; i < bf->dep_count; i++)
		PrintItem(&bf->deps[i]);
	*/

	RecompileTargets(bf);
	return 0;
}
