#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "my_sqlite3.h"

// int do_insert(sqlite3 *db)
// {
//     int id;
//     char name[32] = {0};
//     char ipaddr[32] = {0};
//     char macaddr[32] = {0};
//     int score;
//     char sql[128] = {0};
//     char *errmsg;

//     printf("Input NODE_NAME: ");
//     scanf("%s", &name);
//     getchar();

//     printf("Input IP_ADDR: ");
//     scanf("%s", &ipaddr);
//     getchar();

//     printf("Input OUT_PORT: ");
//     scanf("%d", &score);
//     getchar();

//     printf("Input MAC_ADDR: ");
//     scanf("%s", &macaddr);
//     getchar();

//     sprintf(sql, "insert into stu values('%s', '%s', %d, '%s');", name, ipaddr, score, macaddr);

//     printf("%s\n", sql);

//     if(sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK){
//         printf("%s\n", errmsg);
//     }else{
//         printf("Insert success.\n");
//     }
// }

// int do_delete(sqlite3 *db)
// {
//     int id;
//     char sql[128] = {0};
//     char *errmsg;

//     printf("Input id: ");
//     scanf("%d", &id);
//     getchar();

//     sprintf(sql, "delete from stu where id=%d", id);

//     if(sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK){
//         printf("%s\n", errmsg);
//     }else{
//         printf("Delete success.\n");
//     }
// }

// int callback(void *para, int f_num, char **f_value, char **f_name)
// {
//     int i = 0;

//     for(i=0; i<f_num; i++){
//         printf("%-11s", f_value[i]);
//     }
//     putchar(10);

//     return 0;
// }

// int do_query(sqlite3 *db)
// {
//     char sql[128] = {0};
//     char *errmsg;

//     sprintf(sql, "select * from stu;");

//     if(sqlite3_exec(db, sql, callback, NULL, &errmsg) != SQLITE_OK){
//         printf("%s\n", errmsg);
//     }else{
//         printf("Query success.\n");
//     }
// }

int do_query(sqlite3 *db)
{
    char sql[128] = {0};
    char *errmsg;
    char **resultp;
    int nrow;
    int ncloum;
    int index;

    sprintf(sql, "select * from stu;");

    if(sqlite3_get_table(db, sql, &resultp, &nrow, &ncloum, &errmsg) != SQLITE_OK){
        printf("%s\n", errmsg);
    }else{
        printf("Query success.\n");
    }

    index = ncloum;
    for(int i = 0; i < nrow; i++)
    {
        for(int j=0; j<ncloum; j++)
        {
            printf("%-11s ", resultp[index++]);
        }
        putchar(10);
    }
    
    return 0;
}

int do_update(sqlite3 *db)
{
    int id;
    int score;
    char sql[128] = {0};
    char *errmsg;

    printf("Input update id: ");
    scanf("%d", &id);
    getchar();

    printf("Update score: ");
    scanf("%d", &score);
    getchar();

    sprintf(sql, "update stu set score = %d where id = %d", score, id);

    if(sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK){
        printf("%s\n", errmsg);
    }else{
        printf("Update success.\n");
    }
}

int sqlite3_delete_table(sqlite3 *db, const char *table_name)
{   
    char sql[128] = {0};
    char *errmsg;

    if(table_name != NULL){
        sprintf(sql, "drop table %s;", table_name);

        printf("%s\n", sql);

        if(sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK){
            printf("error: %s\n", errmsg);
        }else{
            printf("Delete %s table success.\n", table_name);
        }
    }else{
        return -1;
    }
}

int sqlite3_do_delete(sqlite3 *db, const char *table_name, const char *delete_item, const char *delete_value)
{
    int id;
    char sql[128] = {0};
    char *errmsg;

    if(delete_item == NULL){
        // delete_item为空表示删除整个table
        sprintf(sql, "delete from %s;", table_name);

        printf("%s\n", sql);

        if(sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK){
            printf("error: %s\n", errmsg);
        }else{
            printf("Delete %s table success.\n", table_name);
        }
    }else{
        // 删除table中的某一行数据
        sprintf(sql, "delete from %s where %s=%s", table_name, delete_item, delete_value);
        if(sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK){
        printf("%s\n", errmsg);
        }else{
            printf("Delete success.\n");
        }
    }
}

int sqlite3_create_table(sqlite3 *db, const char *table_name, const char *table_item[], int item_num)
{   
    // 打开或创建一个数据库
    if(sqlite3_open(DATABASE, &db) != SQLITE_OK){
        printf("%s\n", sqlite3_errmsg(db));
        return -1;
    }else{
        printf("Open %s success.\n", DATABASE);
    }

    // 创建一张数据库表格
    char sql[128] = {0};
    char *errmsg;   

    snprintf(sql, sizeof(sql), "create table %s", table_name);

    strcat(sql, "(");
    for(int i=0; i<item_num; i++)
    {
        strcat(sql, table_item[i]);

        if(i != item_num - 1)
            strcat(sql, ", ");
    }
    strcat(sql, ");");

    printf("%s\n", sql);

    if(sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK){
        printf("error: %s\n", errmsg);
    }else{
        printf("create table or open success.\n");
    }
}
