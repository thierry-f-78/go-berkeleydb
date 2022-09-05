#ifndef __BDB_H__
#define __BDB_H__

#include <db.h>

enum bdb_type {
	BDB_OK = 0,     /* Its alright */
	BDB_NOT_EXISTS, /* DB does not exists or do not have access */
	BDB_TEMP_ERROR, /* Temporary error */
	BDB_ERROR,      /* Unrecoverable error */
	BDB_FORMAT,     /* Do not reconize the database format */
};

struct bdb {
	DB *db;
	char *file;
	int has_zero; /* 0: I don't known, 1: yes, 2: no */
	enum bdb_type status; /* 0: ok, 1: file not exists, 2: right error, 3: open error, 4: not opend */
};

typedef struct bdb bdb;

struct bdb *bdb_open(const char *filename);
void bdb_close(struct bdb *bdb);
char *bdb_get(struct bdb *bdb, const char *key);
int bdb_status(struct bdb *bdb);

#endif /* __BDB_H__ */
