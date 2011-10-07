#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curses.h>
#include <sqlite3.h>
#include <curl/curl.h>

#include <yajl/yajl_tree.h>

#include "credentials.h"

int current_window_row,
    current_window_column,
    window_rows,
    window_columns;

void draw(char character_to_draw) { 
	move(current_window_row, current_window_column); // curses call to move cursor to row r, column c
	delch(); insch(character_to_draw); // curses calls to replace character under cursor by dc
	refresh(); // curses call to update screen
	current_window_row++; // go to next row
	// check for need to shift right or wrap around
	if (current_window_row == window_rows) {
		current_window_row = 0;
		current_window_column++;
		if (current_window_column == window_columns) current_window_column = 0;
	}
}


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

 
// It's a struct bro
struct MemoryStruct {
  char *memory;
  size_t size;
};
 
// Write memory callback for libcurl
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

// This is our main function... yeah!
int main(int argc, char *argv[]) {

	sqlite3 *database;
	int database_status;
	char *database_error_message = 0;

	yajl_val json_node;
	char json_error_message[1024];
	json_error_message[0] = 0;

  CURL *http_handle;

	struct MemoryStruct http_response;
	http_response.memory = malloc(1);
	http_response.size = 0;

	// Curses
	char typed_character;
	WINDOW *window;

	window = initscr(); // curses call to initialize window
	cbreak();           // curses call to set no waiting for Enter key
	noecho();           // curses call to set no echoing
	getmaxyx(window, window_rows, window_columns); // curses call to find size of window
	clear();            // curses call to clear screen, send cursor to position (0,0)
	refresh();          // curses call to implement all changes since last refresh

	current_window_row = 0; 
	current_window_column = 0;
	while (1) {
		typed_character = getch(); // curses call to input from keyboard
		if (typed_character == 'q') {
			break; // quit?
		}
		draw(typed_character); // draw the character
	}

	endwin();

	// Example of handling web requests
	http_handle = curl_easy_init();
	if (http_handle) {
		curl_easy_setopt(http_handle, CURLOPT_URL, "https://github.com/api/v2/json/issues/list/Raizlabs/AppBlade/open");
		curl_easy_setopt(http_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
		curl_easy_setopt(http_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
		curl_easy_setopt(http_handle, CURLOPT_WRITEDATA, (void *)&http_response);
		curl_easy_setopt(http_handle, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
		curl_easy_setopt(http_handle, CURLOPT_USERPWD, GITHUB_API_CREDENTIALS);
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
