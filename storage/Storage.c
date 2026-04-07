#include "Storage.h"
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

//extern, used to id files as my own db files
#define DB_MAGIC "GSQLDB"
#define DB_MAGIC_SIZE 6
#define DB_VERSION 1U

//read write helpers, read raw binary
static int write_u32(FILE *file, uint32_t value) {
    return fwrite(&value, sizeof(value), 1, file) == 1;
}
static int write_i32(FILE *file, int32_t value) {
    return fwrite(&value, sizeof(value), 1, file) == 1;
}
static int write_f64(FILE *file, double value) {
    return fwrite(&value, sizeof(value), 1, file) == 1;
}
static int read_u32(FILE *file, uint32_t *value) {
    return fread(value, sizeof(*value), 1, file) == 1;
}
static int read_i32(FILE *file, int32_t *value) {
    return fread(value, sizeof(*value), 1, file) == 1;
}
static int read_f64(FILE *file, double *value) {
    return fread(value, sizeof(*value), 1, file) == 1;
}

//used to calc len write len then write bytes
static int write_string(FILE *file, const char *value) {
    uint32_t length = 0;
    if (value != NULL) {
        size_t raw_length = strlen(value);
        if (raw_length > UINT32_MAX) {
            return 0;
        }
        length = (uint32_t)raw_length;
    }
    if (!write_u32(file, length)) {
        return 0;
    }
    if (length == 0) {
        return 1;
    }

    return fwrite(value, 1, length, file) == length;
}

//opposite read len alloc then read
static char *read_string(FILE *file) {
    uint32_t length = 0;
    char *buffer;

    if (!read_u32(file, &length)) {
        return NULL;
    }

    buffer = malloc((size_t)length + 1);
    if (buffer == NULL) {
        return NULL;
    }

    if (length > 0 && fread(buffer, 1, length, file) != length) {
        free(buffer);
        return NULL;
    }

    buffer[length] = '\0';
    return buffer;
}

static void free_loaded_table(Table *table) {
    size_t i;
    size_t j;

    if (table == NULL) {
        return;
    }
    free(table->name);

    for (i = 0; i < table->column_count; i++) {
        free(table->columns[i].name);
    }
    free(table->columns);

    for (i = 0; i < table->row_count; i++) {
        Row *row = &table->rows[i];
        for (j = 0; j < row->value_count; j++) {
            if (row->values[j].type == VAL_TEXT) {
                free(row->values[j].as.s_val);
            }
        }
        free(row->values);
    }
    free(table->rows);
}

static void free_loaded_database(Database *db) {
    size_t i;

    if (db == NULL) {
        return;
    }

    for (i = 0; i < db->table_count; i++) {
        free_loaded_table(&db->tables[i]);
    }
    free(db->tables);
    db->tables = NULL;
    db->table_count = 0;
}

int db_save(Database *db, const char *path) {
    char tmp_path[1024];
    FILE *file;
    size_t i;
    size_t j;
    size_t r;

    if (db == NULL || path == NULL) {
        return 0;
    }
    if (snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", path) >= (int)sizeof(tmp_path)) {
        return 0;
    }
    file = fopen(tmp_path, "wb");
    if (file == NULL) {
        return 0;
    }
    if (fwrite(DB_MAGIC, 1, DB_MAGIC_SIZE, file) != DB_MAGIC_SIZE ||
        !write_u32(file, DB_VERSION) ||
        !write_u32(file, (uint32_t)db->table_count)) {
        fclose(file);
        remove(tmp_path);
        return 0;
    }

    for (i = 0; i < db->table_count; i++) {
        Table *table = &db->tables[i];

        if (!write_string(file, table->name) ||
            !write_u32(file, (uint32_t)table->column_count) ||
            !write_u32(file, (uint32_t)table->row_count)) {
            fclose(file);
            remove(tmp_path);
            return 0;
        }
        for (j = 0; j < table->column_count; j++) {
            ColumnsSchema *column = &table->columns[j];

            if (!write_string(file, column->name) ||
                !write_u32(file, (uint32_t)column->type)) {
                fclose(file);
                remove(tmp_path);
                return 0;
            }
        }
        for (r = 0; r < table->row_count; r++) {
            Row *row = &table->rows[r];

            if (!write_u32(file, (uint32_t)row->value_count)) {
                fclose(file);
                remove(tmp_path);
                return 0;
            }
            for (j = 0; j < row->value_count; j++) {
                Value *value = &row->values[j];
                if (!write_u32(file, (uint32_t)value->type)) {
                    fclose(file);
                    remove(tmp_path);
                    return 0;
                }

                switch (value->type) {
                    case VAL_INT:
                        if (!write_i32(file, value->as.i_val)) {
                            fclose(file);
                            remove(tmp_path);
                            return 0;
                        }
                        break;
                    case VAL_FLOAT:
                        if (!write_f64(file, value->as.f_val)) {
                            fclose(file);
                            remove(tmp_path);
                            return 0;
                        }
                        break;
                    case VAL_TEXT:
                        if (!write_string(file, value->as.s_val)) {
                            fclose(file);
                            remove(tmp_path);
                            return 0;
                        }
                        break;
                    default:
                        fclose(file);
                        remove(tmp_path);
                        return 0;
                }
            }
        }
    }
    // bassically save to disk
    if (fflush(file) != 0 || fsync(fileno(file)) != 0 || fclose(file) != 0) {
        remove(tmp_path);
        return 0;
    }
    if (rename(tmp_path, path) != 0) {
        remove(tmp_path);
        return 0;
    }
    return 1;
}
//read db from disk
int db_load(Database *db, const char *path) {
    char magic[DB_MAGIC_SIZE];
    uint32_t version = 0;
    uint32_t table_count = 0;
    Database loaded = {0};
    FILE *file;
    size_t i;
    size_t j;
    size_t r;

    if (db == NULL || path == NULL) {
        return 0;
    }
    file = fopen(path, "rb");
    if (file == NULL) {
        return errno == ENOENT;
    }

    if (fread(magic, 1, DB_MAGIC_SIZE, file) != DB_MAGIC_SIZE ||
        memcmp(magic, DB_MAGIC, DB_MAGIC_SIZE) != 0 ||
        !read_u32(file, &version) ||
        version != DB_VERSION ||
        !read_u32(file, &table_count)) {
        fclose(file);
        return 0;
    }

    loaded.tables = calloc(table_count, sizeof(Table));
    if (table_count > 0 && loaded.tables == NULL) {
        fclose(file);
        return 0;
    }
    loaded.table_count = table_count;

    for (i = 0; i < loaded.table_count; i++) {
        Table *table = &loaded.tables[i];
        uint32_t column_count = 0;
        uint32_t row_count = 0;

        table->name = read_string(file);
        if (table->name == NULL ||
            !read_u32(file, &column_count) ||
            !read_u32(file, &row_count)) {
            free_loaded_database(&loaded);
            fclose(file);
            return 0;
        }
        table->column_count = column_count;
        table->row_count = row_count;
        table->columns = calloc(table->column_count, sizeof(ColumnsSchema));
        table->rows = calloc(table->row_count, sizeof(Row));

        if ((table->column_count > 0 && table->columns == NULL) ||
            (table->row_count > 0 && table->rows == NULL)) {
            free_loaded_database(&loaded);
            fclose(file);
            return 0;
        }
        for (j = 0; j < table->column_count; j++) {
            uint32_t stored_type = 0;

            table->columns[j].name = read_string(file);
            if (table->columns[j].name == NULL || !read_u32(file, &stored_type)) {
                free_loaded_database(&loaded);
                fclose(file);
                return 0;
            }
            if (stored_type > COL_TYPE_UNKNOWN) {
                free_loaded_database(&loaded);
                fclose(file);
                return 0;
            }
            table->columns[j].type = (ColumnType)stored_type;
        }

        for (r = 0; r < table->row_count; r++) {
            Row *row = &table->rows[r];
            uint32_t value_count = 0;

            if (!read_u32(file, &value_count) || value_count != table->column_count) {
                free_loaded_database(&loaded);
                fclose(file);
                return 0;
            }

            row->value_count = value_count;
            row->values = calloc(row->value_count, sizeof(Value));
            if (row->value_count > 0 && row->values == NULL) {
                free_loaded_database(&loaded);
                fclose(file);
                return 0;
            }

            for (j = 0; j < row->value_count; j++) {
                uint32_t stored_type = 0;

                if (!read_u32(file, &stored_type) || stored_type > VAL_FLOAT) {
                    free_loaded_database(&loaded);
                    fclose(file);
                    return 0;
                }

                row->values[j].type = (ValueType)stored_type;
                switch (row->values[j].type) {
                    case VAL_INT: {
                        int32_t value = 0;
                        if (!read_i32(file, &value)) {
                            free_loaded_database(&loaded);
                            fclose(file);
                            return 0;
                        }
                        row->values[j].as.i_val = value;
                        break;
                    }
                    case VAL_FLOAT:
                        if (!read_f64(file, &row->values[j].as.f_val)) {
                            free_loaded_database(&loaded);
                            fclose(file);
                            return 0;
                        }
                        break;
                    case VAL_TEXT:
                        row->values[j].as.s_val = read_string(file);
                        if (row->values[j].as.s_val == NULL) {
                            free_loaded_database(&loaded);
                            fclose(file);
                            return 0;
                        }
                        break;
                }
            }
        }
    }

    if (fgetc(file) != EOF) {
        free_loaded_database(&loaded);
        fclose(file);
        return 0;
    }

    fclose(file);
    db_free(db);
    *db = loaded;
    return 1;
}


//not entirely mine, thank god for online docs and ai
