#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include "lyrics_process.h"

/*****     打开歌词文件，读数据到缓冲区*/

static char *open_lrc(char *addr) {
	FILE *fptr = NULL;
	char *lycris_buf = NULL;  // the address of malloc buf which saved lycris content
	int size = 0;

	if (addr == NULL) {
		printf("error: addr or size is NULL!\n");
	} else {
		fptr = fopen(addr, "rb");
		if (fptr == NULL) {
			printf("open lycris [%s] error!\n", addr);
		} else {
			fseek(fptr, 0, SEEK_END);
			size = ftell(fptr);
			rewind(fptr);
			lycris_buf = (char *)malloc(size + 1);
			bzero(lycris_buf, size + 1);
			if (lycris_buf == NULL) {

				printf("malloc lycris_buf error\n");
			} else {

				fread(lycris_buf, size, 1, fptr);
			}
			fclose(fptr);
		}
	}
	return lycris_buf;
}

/****** 创建双向链表节点*************/
static LRC *create_lrc_node(uint time, char *lrc) {
	LRC *node = (LRC *)malloc(sizeof(LRC));
	if (node == NULL) {
		printf("内存分配失败，创建链表节点失败\n");
		return NULL;
	}
	node->prev = NULL;
	node->next = NULL;
	node->time = time;
	strcpy(node->lrc, lrc);
	return node;
}

/****************   增加一个歌词信息节点至链表结尾*/

static LRC *add_lrc_to_link(LRC *head, uint time, char *lrc) {
	LRC *nextpr = head;
	LRC *node = NULL;

	if (nextpr == NULL) {  // 创建链表
		head = create_lrc_node(time, lrc);
	} else {  // 增加一个链表节点至链表结尾
		while (nextpr->next != NULL) {
			nextpr = nextpr->next;
		}
		node = create_lrc_node(time, lrc);
		if (node != NULL) {
			node->prev = nextpr;
			nextpr->next = node;
		}
	}

	return head;
}


/***********遍历输出链表*/
void print_lrc_link(LRC_PTR lrc) {
	uint i = 0;

	for (i = 0; i < lrc.lrc_arry_size; i++) {
		//printf("第%d个节点：\n", i);
		printf("时间: %dms 歌词: %s\n", lrc.lrc_arry[i]->time, lrc.lrc_arry[i]->lrc);
	}
}

/*********** 链表的元素按时间大小从小到大排序***************************/
static void inorder_link(LRC_PTR *lrc) {
	LRC *nextpr, *temp;
	uint j, k;

	nextpr = lrc->lrc_head;
	lrc->lrc_arry_size = 0;
	while (nextpr) {
		lrc->lrc_arry_size++;
		nextpr = nextpr->next;
	}
	lrc->lrc_arry = calloc(lrc->lrc_arry_size, sizeof(lrc->lrc_arry));
	nextpr = lrc->lrc_head;
	for (j = 0; j < lrc->lrc_arry_size; j++) {
		*(lrc->lrc_arry + j) = nextpr;		//保存链表各元素地址
		nextpr = nextpr->next;
	}
	for (j = 0; j < lrc->lrc_arry_size - 1; j++) {	//按照各链表元素的id从小到大排序，将各元素放入指针数组中
		for (k = j + 1; k < lrc->lrc_arry_size; k++) {
			if (lrc->lrc_arry[j]->time > lrc->lrc_arry[k]->time) {
				temp = *(lrc->lrc_arry + k);
				lrc->lrc_arry[k] = lrc->lrc_arry[j];
				lrc->lrc_arry[j] = temp;
			}
		}
	}
	lrc->lrc_head = *(lrc->lrc_arry);
	for (j = 0; j < (lrc->lrc_arry_size - 1); j++) {	//按照指针数组中的各链表元素的地址的顺序重新给链表安排指针域
		lrc->lrc_arry[j]->next = lrc->lrc_arry[j + 1];
	}
	lrc->lrc_arry[lrc->lrc_arry_size - 1]->next = NULL;
}


/************* 判断是否是正确的时间标签****************/
static char judge_time_label(const char *label) {
	// 合法时间标示例 [00:07.41
	if (label) {  // label非空
		if (strlen(label) == 9) {
			if ((label[0] == '[') && (label[3] == ':') && (label[6] == '.') && (label[1] >= '0' && label[1] <= '9')
			    && (label[2] >= '0' && label[2] <= '9') && (label[4] >= '0' && label[4] <= '9')
			    && (label[5] >= '0' && label[5] <= '9') && (label[7] >= '0' && label[7] <= '9')
			    && (label[8] >= '0' && label[8] <= '9')) {
				return 1;
			}
		}
	}
	return 0;
}

/********************计算时间标签****************/
static uint calculate_time_label(const char *label) {
	uint mtime = 0;
	uint minute = 0;
	uint second = 0;
	uint msecond = 0;

	sscanf(label, "[%d:%d.%d", &minute, &second, &msecond);
	mtime = minute * 60000 + second * 1000 + msecond * 10;
	return mtime;
}


/************************** 处理歌词文件一行信息***********/
static LRC *dispose_line(char *line, LRC *head) {
	uint i = 0;
	uint argc = 0;
	uint mtime = 0;
	char *lrc_text = NULL;
	char *argv[lrc_time_labels] = {NULL};

	argv[argc] = strtok(line, "]");
	while ((argv[++argc] = strtok(NULL, "]")) != NULL) {
		if (argc >= lrc_time_labels) {
			printf("lrc: too many labels\n");
			break;
		}
	}
	// 取出歌词
	if ((judge_time_label(argv[0]) != 0) && (argv[argc - 1] != NULL) && argc) {
		lrc_text = argv[argc - 1];
	}
	for (i = 0; i < argc && argc > 1; i++) {
		if (judge_time_label(argv[i])) {  // 判断时间标签
			mtime = calculate_time_label(argv[i]);
			if (lrc_text && (strlen(lrc_text) > 1)) {
				head = add_lrc_to_link(head, mtime, lrc_text);
			}
		}
	}
	return head;
}

/*********将缓冲区内的'\n'换成'\0',并在每次替换时处理歌词文件行信息************/
static void get_dispose_line(char *lycris_buf, LRC_PTR *lrc) {
	char *line_ptr = NULL;

	while ((line_ptr = strtok(lycris_buf, "\n")) != NULL) {
		lycris_buf += strlen(lycris_buf) + 1;
		lrc->lrc_head = dispose_line(line_ptr, lrc->lrc_head);
	}
}

/*********  处理歌词文件****************/
LRC *dispose_lrc(char *name, LRC_PTR *lrc) {
	char *lycris_buf = NULL;
	memset(lrc, 0, sizeof(LRC_PTR));  // init lrc struct
	lycris_buf = open_lrc(name);
	if (lycris_buf != NULL) {
		get_dispose_line(lycris_buf, lrc);
		free(lycris_buf);
		inorder_link(lrc);
	} else {
		printf("open_lrc error in dispose_lrc func\n");
		return NULL;
	}
	return lrc->lrc_head;
}

/*****释放歌词数组****************************/
void free_lrc_arry(LRC_PTR *lrc) {
	int i;
	if (lrc->lrc_arry != NULL) {
		for (i = 0; i < lrc->lrc_arry_size; i++) {
			free(lrc->lrc_arry[i]);
		}
		free(lrc->lrc_arry);
	}
	lrc->lrc_arry = NULL;
}

//--------------------------读取文件里面的歌曲名字--------------------------//
Name * create_node1(const char *filename) {
	Name *newnode = (Name *)malloc(sizeof(Name));
	newnode->data = (char *)malloc(strlen(filename) + 1);
	strcpy(newnode->data, filename);
	newnode->next = NULL;
	return newnode;
}

Name * add_node(Name * head, char* newdata) {
	Name *newnode = create_node1(newdata);
	if (head == NULL) {
		return newnode;
	}
	Name *temp = head;
	while (temp->next != NULL) {
		temp = temp->next;
	}
	temp->next = newnode;
	return head;
}


void free_list(Name *head) {
	Name *current = head;
	while (current != NULL) {
		Name *temp = current;
		current = current->next;
		free(temp->data);
		free(temp);
	}
}


Name * read_audio_files(const char* folder_path) {
	DIR *dir;
	struct dirent *dp;
	Name *head = NULL;
	if ((dir = opendir(folder_path)) == NULL) {
		printf("Open directory fails.\n");
		exit(1);
	}
	while ((dp = readdir(dir)) != NULL) {
		if (strstr(dp->d_name, ".mp3")) {
			head = add_node(head, dp->d_name);
		}
	}
	closedir(dir);
	return head;
}

