#include "Exec.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>


static char *cur_src_file = NULL;
static char *cur_obj_file = NULL;
static char *cur_obj_files = NULL;

static char *ConstFind(struct BuildFile *bf, char *name)
{
	if(cur_src_file != NULL && !strcmp(name, "src_file"))
		return cur_src_file;

	if(cur_obj_file != NULL && !strcmp(name, "obj_file"))
		return cur_obj_file;

	if(cur_obj_files != NULL && !strcmp(name, "obj_files"))
		return cur_obj_files;

	for(uint64_t i = 0; i < bf->const_count; i++) {
		if(!strcmp(bf->consts[i].name, name))
			return bf->consts[i].val;
	}

	return NULL;
}

static struct BuildItem *DepGet(struct BuildFile *bf, char *dep_name)
{
	for(uint64_t i = 0; i < bf->dep_count; i++) {
		if(!strcmp(bf->deps[i].name, dep_name))
			return &bf->deps[i];
	}

	return NULL;
}

static void ValidateBuildItem(struct BuildFile *bf, struct BuildItem *item)
{
	if(access(item->name, R_OK) == -1) {
		printf("File '%s' does not exist\n", item->name);
		exit(1);
	}

	for(uint64_t i = 0; i < item->dep_count; i++) {
		if(DepGet(bf, item->deps[i]) == NULL) {
			printf("File '%s' does not exist\n", item->deps[i]);
			exit(1);
		}
	}
}

static void ValidateBuildFile(struct BuildFile *bf)
{
	char *bin_dir = ConstFind(bf, "bin_dir");
	char *compile = ConstFind(bf, "compile");
	char *link = ConstFind(bf, "link");

	if(bin_dir == NULL) {
		printf("Constant `bin_dir` is not defined\n");
		exit(1);
	}

	if(compile == NULL) {
		printf("Constant `compile` is not defined\n");
		exit(1);
	}

	if(link == NULL) {
		printf("Constant `link` is not defined\n");
		exit(1);
	}

	if(access(bin_dir, R_OK | W_OK) == -1) {
		printf("Binary directory '%s' does not exist\n", bin_dir);
		exit(1);
	}

	for(uint64_t i = 0; i < bf->target_count; i++)
		ValidateBuildItem(bf, &bf->targets[i]);

	for(uint64_t i = 0; i < bf->dep_count; i++)
		ValidateBuildItem(bf, &bf->deps[i]);
}

static char *ObjectFile(char *bin_dir, char *target)
{
	uint64_t hash = 0xCBF29CE484222325;

	while(*target) {
		hash *= 0x100000001B3;
		hash ^= *target++;
	}

	char *str = malloc(274);
	snprintf(str, 274, "%s/%016lx.o", bin_dir, hash);
	return str;
}

static time_t ModifTime(char *file)
{
	struct stat s;

	if(stat(file, &s) == -1)
		return 0;

	return s.st_mtim.tv_sec;
}

static int ShouldRecompile(struct BuildFile *bf, time_t obj_time, struct BuildItem *item)
{
	if(ModifTime(item->name) >= obj_time)
		return 1;

	for(uint64_t i = 0; i < item->dep_count; i++) {
		if(ShouldRecompile(bf, obj_time, DepGet(bf, item->deps[i])))
			return 1;
	}

	return 0;
}

static int RunCmd(struct BuildFile *bf, char *compile)
{
	char *new_cmd = calloc(16384, 1);
	uint64_t j = 0;
	int len = strlen(compile);

	for(uint64_t i = 0; i < len; i++) {
		if(compile[i] == '%') {
			int did_copy = 0;

			for(uint64_t k = i + 1; k < len; k++) {
				if(compile[k] == '%') {
					char *name = malloc(k - i);
					memcpy(name, &compile[i + 1], k - i - 1);
					name[k - i - 1] = '\0';
					char *val = ConstFind(bf, name);
					free(name);
					int vlen = strlen(val);

					if(val != NULL) {
						memcpy(&new_cmd[j], val, vlen);
						j += vlen;
						i = k;
						did_copy = 1;
						break;
					}
				}
			}

			if(did_copy)
				continue;
		}

		new_cmd[j++] = compile[i];
	}

	printf("$ %s\n\n", new_cmd);
	int s = system(new_cmd);
	free(new_cmd);
	return s;
}


void RecompileTargets(struct BuildFile *bf)
{
	ValidateBuildFile(bf);
	char *bin_dir = ConstFind(bf, "bin_dir");
	char *compile = ConstFind(bf, "compile");
	char *link = ConstFind(bf, "link");
	char **obj_files = calloc(bf->target_count, sizeof(char *));
	uint8_t *recomp = calloc(bf->target_count / 8 + 1, sizeof(uint8_t));

	for(uint64_t i = 0; i < bf->target_count; i++) {
		struct BuildItem *item = &bf->targets[i];
		obj_files[i] = ObjectFile(bin_dir, item->name);
		time_t obj_time = ModifTime(obj_files[i]);

		if(ShouldRecompile(bf, obj_time, item))
			recomp[i / 8] |= 1 << (i % 8);
	}

	int status = 0;
	putchar('\n');

	for(uint64_t i = 0; i < bf->target_count; i++) {
		struct BuildItem *item = &bf->targets[i];

		if(((recomp[i / 8] >> (i % 8)) & 1) == 1) {
			cur_src_file = item->name;
			cur_obj_file = obj_files[i];
			status += RunCmd(bf, compile);
		}
	}

	if(status != 0) {
		printf("Fatal error, aborting build\n");
		exit(1);
	}

	cur_src_file = NULL;
	cur_obj_file = NULL;
	int len = 0;

	for(uint64_t i = 0; i < bf->target_count; i++)
		len += strlen(obj_files[i]) + 1;

	cur_obj_files = malloc(len);
	uint64_t j = 0;

	for(uint64_t i = 0; i < bf->target_count; i++) {
		int l = strlen(obj_files[i]);
		memcpy(&cur_obj_files[j], obj_files[i], l);
		j += l;
		cur_obj_files[j] = i == bf->target_count - 1 ? '\0' : ' ';
		j++;
	}

	if(RunCmd(bf, link) != 0) {
		printf("Fatal error, aborting build\n");
		exit(1);
	}
}
