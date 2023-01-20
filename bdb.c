#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <db.h>

#include "bdb.h"

/* This file try to manage Berkeley DB access in the same way 
 * that postfix:
 *  - Detects file change
 *  - read only access
 *  - GET access only
 */

static int _bdb_reopen(struct bdb *bdb)
{
	int err;

	if (bdb->db != NULL) {
		bdb->db->close(bdb->db, 0);
		bdb->db = NULL;
	}

	if (db_create(&bdb->db, NULL, 0) != 0) {
		bdb->db = NULL;
		bdb->status = BDB_ERROR;
		return 0;
	}

	err = bdb->db->open(bdb->db, NULL, bdb->file, NULL, DB_UNKNOWN, DB_RDONLY, 0);
	if (err != 0) {
		bdb->db->close(bdb->db, 0);		
		bdb->db = NULL;
		switch (err) {
		case ENOENT:
			bdb->status = BDB_NOT_EXISTS;
			break;
		case DB_OLD_VERSION:
#ifdef DB_CHKSUM_FAIL
		case DB_CHKSUM_FAIL:
#endif
			bdb->status = BDB_FORMAT;
			break;
		default:
			bdb->status = BDB_ERROR;
		}
		return 0;
	}

	return 1;
}

void bdb_close(struct bdb *bdb)
{
	if (bdb->db != NULL) {
		bdb->db->close(bdb->db, 0);
	}
	free(bdb->file);
	free(bdb);
}

struct bdb *bdb_open(const char *filename)
{
	struct bdb *bdb;

	bdb = calloc(1, sizeof(*bdb));
	if (bdb == NULL) {
		return NULL;
	}

	bdb->file = strdup(filename);
	if (bdb->file == NULL) {
		free(bdb);
		return NULL;
	}

	return bdb;
}

/* return: NULL: no match; freeable null terminated string: match */
char *bdb_get(struct bdb *bdb, const char *key)
{
	int fd;
	int status;
	char *ret;
	DBT db_key;
	DBT db_value;
	struct stat file_open;
	struct stat file_name;

	/* check file change */
	if (stat(bdb->file, &file_name) < 0) {
		/* No access */
		if (bdb->db != NULL) {
			bdb->db->close(bdb->db, 0);
			bdb->db = NULL;
			bdb->mtime = -1;
		}
		bdb->status = BDB_NOT_EXISTS;
		return NULL;
	}

	if (bdb->db == NULL) {

		/* open file if it is not opened */
		if (_bdb_reopen(bdb) == 0) {
			return NULL;
		}

		/* Get dict FD */
		if (bdb->db->fd(bdb->db, &fd) != 0) {
			bdb->status = BDB_ERROR;
			return NULL;
		}

		/* check stat */
		if (fstat(fd, &file_open) < 0) {
			bdb->db->close(bdb->db, 0);
			bdb->db = NULL;
			bdb->status = BDB_ERROR;
			return NULL;
		}
		bdb->mtime = file_open.st_mtime;

	} else {

		/* Get dict FD */
		if (bdb->db->fd(bdb->db, &fd) != 0) {
			bdb->status = BDB_ERROR;
			return NULL;
		}

		/* Reopen */
		if (bdb->mtime != file_name.st_mtime) {
			if (_bdb_reopen(bdb) == 0) {
				return NULL;
			}

			/* Get dict FD */
			if (bdb->db->fd(bdb->db, &fd) != 0) {
				bdb->status = BDB_ERROR;
				return NULL;
			}
		}
	}

	/* Acquire a shared lock. */
	while ((status = flock(fd, LOCK_SH)) < 0 && errno == EINTR)
		usleep(100000); /* 100 000 microseconds = 100 milliseconds = 0.1 second */

	/* Perform lookup according with end 0 detection */
	switch (bdb->has_zero) {
	case 0: /* I don't known */
	case 1: /* Have zero terminated keys. */
		memset(&db_key, 0, sizeof(db_key));
		memset(&db_value, 0, sizeof(db_value));
		db_key.data = (char *)key;
		db_key.size = strlen(key) + 1;
		status = bdb->db->get(bdb->db, NULL, &db_key, &db_value, 0);
		if (status == 0) {
			bdb->has_zero = 1;
		} else if (status != DB_NOTFOUND) {
			bdb->status = BDB_TEMP_ERROR;
		}
		if (bdb->has_zero == 1) {
			break;
		}
	case 2: /* Doesn't Have zero terminated keys. */
		memset(&db_key, 0, sizeof(db_key));
		memset(&db_value, 0, sizeof(db_value));
		db_key.data = (char *)key;
		db_key.size = strlen(key);
		status = bdb->db->get(bdb->db, NULL, &db_key, &db_value, 0);
		if (status == 0) {
			bdb->has_zero = 2;
		} else if (status != DB_NOTFOUND) {
			bdb->status = BDB_TEMP_ERROR;
		}
	}

	/* in success case, duplicate data. If strdup fail, we return NULL
	 * as the lookup doesn't match. Don't care about the zero terminated
	 * values. strndup stops on the first encountered 0, or the max length
	 */
	ret = NULL;
	if (status == 0) {
		ret = strndup(db_value.data, db_value.size);
		bdb->status = BDB_OK;
	} else if (status == DB_NOTFOUND) {
		bdb->status = BDB_OK;
	}

	/* Release lock */
	flock(fd, LOCK_UN);

	/* Return value */
	return ret;
}

int bdb_status(struct bdb *bdb)
{
	return bdb->status;
}
