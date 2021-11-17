#ifndef _MY_SQLITE3_H_
#define _MY_SQLITE3_H_

#if defined(__cplusplus)
extern "C" {
#endif

#define DATABASE "stu.db"

#include <sqlite3.h>

int do_insert(sqlite3 *db);
int do_delete(sqlite3 *db);
int do_query(sqlite3 *db);
int do_update(sqlite3 *db);

int sqlite3_create_table(sqlite3 *db, const char *table_name, const char *table_item[], int item_num);
int sqlite3_delete_table(sqlite3 *db, const char *table_name);
int sqlite3_do_delete(sqlite3 *db, const char *table_name, const char *delete_item, const char *delete_value);

#if defined(__cplusplus)
}
#endif

#endif