#ifndef _NAME_H
#define _NAME_H

typedef struct node {
	char* data;
	struct node* next;        
} Name ;
Name * create_node1(const char *filename);
Name * add_node(Name * head, char* newdata);
void free_list(Name * head);
Name * read_audio_files(const char* folder_path);
#endif 
