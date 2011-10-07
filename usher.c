#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sqlite3.h>

#include <yajl/yajl_tree.h>

static int callback(void *NotUsed, int argc, char **argv, char **azColName){
	NotUsed=0;
	int i;
	for(i = 0; i < argc; i++) {
		printf("%s = %s\n", azColName[i], argv[i] ? argv[i]: "NULL");
	}
	printf("\n");
	return 0;
}

/* 
 * WHAT THE FUCK YAJL
 * I have to hot-fix the shit, to get tree getting to work...
 * Holy whisky tango foxtrot batman!
 */
yajl_val yajl_tree_get(yajl_val n, const char ** path, yajl_type type) {
    if (!path) return NULL;
    while (n && *path) {
        unsigned int i, found = 0;

        if (n->type != yajl_t_object) return NULL;
        for (i = 0; i < n->u.object.len; i++) {
            if (!strcmp(*path, n->u.object.keys[i])) {
                n = n->u.object.values[i];
                found = 1;
                break;
            }
        }
        if (!found) return NULL;
        path++;
    }
    if (n && type != yajl_t_any && type != n->type) n = NULL;
    return n;
}

int main(int argc, char *argv[]) {

	sqlite3 *database;
	int database_status;
	char *database_error_message = 0;

	yajl_val json_node;
	char json_error_message[1024];
	json_error_message[0] = 0;

	json_node = yajl_tree_parse("{\"A\": \"B\", \"Foo\": {\"bar\": \"a\"}}\0", json_error_message, sizeof(json_error_message));
	if (json_node == NULL) {

		fprintf(stderr, "parse error: ");
		if (strlen(json_error_message)) {
			fprintf(stderr, " %s", json_error_message);
		} else {
			fprintf(stderr, "unknown error");
		}
		fprintf(stderr, "\n");

		return 1;

	} else {

		const char * json_search_path[] = { "Foo", "bar", (const char *) 0 };
		yajl_val json_value = yajl_tree_get(json_node, json_search_path, yajl_t_string);

		if (json_value) {
			printf("%s/%s: %s\n", json_search_path[0], json_search_path[1], YAJL_GET_STRING(json_value));
		} else {
			printf("no such node: %s/%s\n", json_search_path[0], json_search_path[1]);
		}

	}

	yajl_tree_free(json_node);

/*

	int distance = 100; 
	float power = 2.345f;
	double super_power = 56789.4532; 
	char initial = 'A';
	char first_name[] = "Zed"; 
	char last_name[] = "Shaw";
	int areas[] = {10, 12, 13, 14, 20};

	printf("You are %d miles away.\n", distance); 
	printf("You have %f levels of power.\n", power); 
	printf("You have %f awesome super powers.\n", super_power); 
	printf("I have an initial %c.\n", initial); 
	printf("I have a first name %s.\n", first_name); 
	printf("I have a last name %s.\n", last_name); 
	printf("My whole name is %s %c. %s.\n", first_name, initial, last_name);

	printf("The size of an int: %ld\n", sizeof(int)); 
	printf("The size of areas (int[]): %ld\n", sizeof(areas)); 
	printf("The number of ints in areas: %ld\n", sizeof(areas) / sizeof(int)); 
	printf("The first area is %d, then 2nd %d.\n", areas[0], areas[1]);

*/

	database_status = sqlite3_open("tickets.db", &database);
	if (database_status) {
		printf("Can't open database: %s\n", sqlite3_errmsg(database));
		sqlite3_close(database);
		return 1;
	}

	database_status = sqlite3_exec(database, "create table notes (body text)", callback, 0, &database_error_message);
	if (database_status != SQLITE_OK) {
		printf("SQL error: %s\n", database_error_message);
	}

	sqlite3_close(database);
	return 0;
}
