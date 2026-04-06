#ifndef STORAGE_H
#define STORAGE_H
#include "../runtime/Databse.h"


int db_save(Database *db, const char *path);
int db_load(Database *db, const char *path);

#endif