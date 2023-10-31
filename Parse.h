#pragma once

#include <stddef.h>
#include <stdint.h>


struct BuildItem
{
	char         *name;
	char        **deps;
	uint64_t dep_count;
};

struct BuildConst
{
	char *name;
	char  *val;
};

struct BuildFile
{
	uint64_t     target_count;
	uint64_t        dep_count;
	uint64_t      const_count;
	struct BuildItem *targets;
	struct BuildItem    *deps;
	struct BuildConst *consts;
};


struct BuildFile *ParseBuildFile(const char *file_name);
