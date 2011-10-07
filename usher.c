#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sqlite3.h>
#include <curl/curl.h>

#include <yajl/yajl_tree.h>

// Not quite sure why we need a callback here
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

size_t write_data( void *ptr, size_t size, size_t nmeb, void *stream) {
 return fwrite(ptr,size,nmeb,stream);
}
 
struct MemoryStruct {
  char *memory;
  size_t size;
};
 
 
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;
 
  mem->memory = realloc(mem->memory, mem->size + realsize + 1);
  if (mem->memory == NULL) {
    /* out of memory! */ 
    printf("not enough memory (realloc returned NULL)\n");
    exit(EXIT_FAILURE);
  }
 
  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;
 
  return realsize;
}

int main(int argc, char *argv[]) {

	sqlite3 *database;
	int database_status;
	char *database_error_message = 0;

	yajl_val json_node;
	char json_error_message[1024];
	json_error_message[0] = 0;

	char *http_result;
  CURL *http_handle;
  CURLcode http_code;

	struct MemoryStruct http_response;
	http_response.memory = malloc(1);
	http_response.size = 0;

	// Example of handling web requests
	http_handle = curl_easy_init();
	if (http_handle) {
    curl_easy_setopt(http_handle, CURLOPT_URL, "https://appblade.com/api/me?oauth_token=50667aaf0c73e9684be2c10889a4b26c37f15c97db9e8e5905c6ac03a8859f37");
		curl_easy_setopt(http_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
		curl_easy_setopt(http_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
		curl_easy_setopt(http_handle, CURLOPT_WRITEDATA, (void *)&http_response);
    curl_easy_perform(http_handle);
    curl_easy_cleanup(http_handle);
  }
	printf("%lu bytes retrieved\n", (long)http_response.size);
	printf("%s\n", http_response.memory);

	if (http_response.memory) {
    free(http_response.memory);
	}

	// Example of parsing JSON
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
			fprintf(stderr, "no such node: %s/%s\n", json_search_path[0], json_search_path[1]);
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

	// Example accessing a database
	database_status = sqlite3_open("tickets.db", &database);
	if (database_status) {
		printf("Can't open database: %s\n", sqlite3_errmsg(database));
		sqlite3_close(database);
		return 1;
	}

	// Example writing to a database
	database_status = sqlite3_exec(database, "create table notes (body text)", callback, 0, &database_error_message);
	if (database_status != SQLITE_OK) {
		printf("SQL error: %s\n", database_error_message);
	}

	sqlite3_close(database);
	return 0;
}
