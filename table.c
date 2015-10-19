#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "table_utils.h"
#include "table.h"

struct column {

	char* name;
	int width;
	void* content;
	Type type;
};

struct table {

	int n_cols;
	int n_rows;
	Column* columns;
};

Table* new_table(int n_cols, char** column_names, Type* types) {
	
	int i;
	Table* tab = malloc(sizeof(Table));
	tab->n_cols = n_cols;
	tab->n_rows = 0;
	tab->columns = malloc(n_cols * sizeof(Column));
	
	for( i = 0; i < n_cols; i++ ) {
		
		char* col_name = (column_names != NULL) ? column_names[i]:"\0";
		tab->columns[i].name = malloc(strlen(col_name) + 1);
		strcpy(tab->columns[i].name, col_name);
		
		tab->columns[i].width = strlen(col_name);
		tab->columns[i].content = NULL;
		tab->columns[i].type = (types != NULL) ? types[i]:STRING;
	}
	
	return tab;
}

char* table_format_string(Table* tab) {
	
	int i;
	int len = 2*tab->n_cols;
	int size = len + strlen("\n");
	char* fstring = malloc(size + 1);
	memset(fstring, '\0', size);

	for( i = 0; i < tab->n_cols; i++ ) {
		strcat(fstring, "%s");
	}
	strcat(fstring, "%s\n");

	return fstring;
}

int count_lines(FILE* file) {

	int n_lines = 0;
	char buff[1024];
	while( fgets(buff, 1024, file) ) {
		if(!(strlen(buff) == 1 && *buff == '\n'))
			n_lines++;
	}
	rewind(file);
	
	return n_lines;
}

char* stringify_entry(void* col_content, int row, Type type) {
	
	char str[2048];
	char* ret;
	char *form = format_string(type);
	memset(str, '\0', 1024);

	if(type == CHAR)
		sprintf(str, form, ((char*)col_content)[row]);
	else if(type == STRING)
		sprintf(str, form, ((char**)col_content)[row]);
	else if(type == INT)
		sprintf(str, form, ((int*)col_content)[row]);
	else if(type == FLOAT)
		sprintf(str, form, ((float*)col_content)[row]);
	else if(type == DOUBLE)
		sprintf(str, form, ((double*)col_content)[row]);
	else fatal_error("width(...)", "Unknown type!");	
	
	ret = malloc(strlen(str) + 1);
	strcpy(ret, str);
	
	return ret;
}

int width(void* col_content, int row, Type type) {

	char* entry_str = stringify_entry(col_content, row, type);
	int len = strlen(entry_str);
	free(entry_str);

	return len;
}

/* Insert always reads the content as a string, then 
converts it to put in the table */
void insert(Table* tab, char* content, int col, int row) {
	
	Column column = tab->columns[col];
	Type type = column.type;
	
	if(type == CHAR) {
		((char*)column.content)[row] = *content;
	}
	else if(type == STRING) {
		int size = strlen(content) + 1;
		char* ptr = malloc(size);
		strcpy(ptr, content);
		((void**)column.content)[row] = (void*)ptr;
	}
	else if(type == INT) {
		((int*)column.content)[row] = atoi(content);
	}
	else if(type == FLOAT) {
		((float*)column.content)[row] = (float)atof(content);
	}
	else if(type == DOUBLE) {
		((double*)column.content)[row] = atof(content);
	}
	else fatal_error("width(...)", "Unknown type!");
	
	tab->columns[col].width = max(column.width, width(column.content, row, column.type));
}

void add_column(Table* tab, char* col_name, void* content, Type type){
	
	int i, max_width;
	
	if( content == NULL ){
		fatal_error("add_column(...)", "Cannot add NULL column!");
	}
	
	tab->n_cols++;
	tab->columns = realloc(tab->columns, tab->n_cols * sizeof(Column));
	
	tab->columns[tab->n_cols - 1].name = malloc( strlen(col_name) + 1 );
	strcpy(tab->columns[tab->n_cols - 1].name, col_name);
	
	tab->columns[tab->n_cols - 1].content = malloc( tab->n_rows * type_size(type) );
	memcpy(tab->columns[tab->n_cols - 1].content, content, tab->n_rows * type_size(type));
	
	tab->columns[tab->n_cols - 1].type = type;
	
	max_width = strlen(col_name);
	for( i = 0; i < tab->n_rows; i++ ) {
		max_width = max(max_width, width(content, i, type));
	}
	
	tab->columns[tab->n_cols - 1].width = max_width;
}

void read_file(Table* tab, char* file_name) {
	
	int i, col;
	int n_lines;
	int n_rows = tab->n_rows, n_cols = tab->n_cols;
	char buff[2048];
	FILE* table_file;
	
	memset(buff, '\0', 2048); 
	table_file = fopen(file_name, "r");

	n_lines = count_lines(table_file);
	tab->n_rows += n_lines;
	
	/* Realloc columns */
	for( col = 0; col < n_cols; col++ ) {
		tab->columns[col].content = realloc(tab->columns[col].content, 
									tab->n_rows * type_size(tab->columns[col].type)); 
	}
	
	for( i = 0; i < n_lines; i++ ) {
		for( col = 0; col < n_cols; col++ ) {
		
			fscanf(table_file, "%s", buff);		
			insert(tab, buff, col, n_rows);			
		}
		n_rows++;
	}
	fclose(table_file);
}

int* get_cols_width(Table* tab) {
	
	int i;
	int* cols_width = malloc(tab->n_cols * sizeof(int));
	for( i = 0; i < tab->n_cols; i++ ) {
		cols_width[i] = tab->columns[i].width;
	}
	return cols_width;
}

int print_content( void* col_content, int row, Type type ) {

	char* content = stringify_entry(col_content, row, type);
	if( content == NULL ) {
		content = "<null>";
	}

	fprintf(stdout, "%s", content);
	free(content);
	return width(col_content, row, type);
}

void print_table(Table* tab) {
	
	int i, j;
	int printed_len;
	int n_rows = tab->n_rows;
	int n_cols = tab->n_cols;
	int* cols_width = get_cols_width(tab);
	
	print_full_line(cols_width, n_cols);
	fprintf(stdout, "%c ", VLINE);
	for( i = 0; i < tab->n_cols; i++ ) {
		fprintf(stdout, "%s%n", tab->columns[i].name, &printed_len);
		print_spaces(cols_width[i] - printed_len + 1);
		printf("%c ", VLINE);
	}
	fprintf(stdout, "\n");
	print_full_line(cols_width, n_cols);
	
	for( j = 0; j < n_rows; j++ ) {
		printf("%c ", VLINE);
		for( i = 0; i < n_cols; i++ ) {
		
			void* content = tab->columns[i].content;
			Type type = tab->columns[i].type;
			printed_len = print_content( content, j, type );
			print_spaces(cols_width[i] - printed_len + 1);
			printf("%c ", VLINE);
		}
		fprintf(stdout, "\n");
	}
	print_full_line(cols_width, n_cols);
	free(cols_width);
}

void free_table(Table* tab) {
	
	int col, row;
	for( col = 0; col < tab->n_cols; col++ ) {
		free(tab->columns[col].name);
		for( row = 0; row < tab->n_rows; row++ ) {
			if( tab->columns[col].type == STRING)
				free(((void**)tab->columns[col].content)[row]);
		}
		free(tab->columns[col].content);
	}
	free(tab->columns);
	free(tab);
}

void* get_column(Table* tab, int col, int* n_elements){
	
	int n_rows = tab->n_rows;
	Column column = tab->columns[col];
	Type type = column.type;
	int size = n_rows * type_size(type);
	void* ptr;
	*n_elements = n_rows;

	ptr = malloc( size );
	ptr = memcpy( ptr, column.content, size );
	return ptr;
}

Type get_column_type(Table* tab, int col){
	return tab->columns[col].type;
}

void swap_columns(Table* tab, int col1, int col2){
	
	Column aux = tab->columns[col1];
	tab->columns[col1] = tab->columns[col2];
	tab->columns[col2] = aux;
}

void apply_to_column(Table* tab, void (*func)(Column), int col) {
	func(tab->columns[col]);
}

void apply_to_columns(Table* tab, void (*func)(Column)) {

	int col;
	int n_cols = tab->n_cols;
	for( col = 0; col < n_cols; col++ ) {
		apply_to_column(tab, func, col);
	}
}

