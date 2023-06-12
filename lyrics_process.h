#ifndef _LYRICS_PROCESS_H_
#define _LYRICS_PROCESS_H_

#define lrc_length 255
#define lrc_time_labels 255

typedef unsigned int uint;
typedef unsigned char uchar;

/*******************************************************
 *歌词信息结构体(链表)
 ********************************************************/
typedef struct lylics
{
	uint time;              //歌词对应的时间
	char lrc[lrc_length];   //歌词
	struct lylics *next, *prev;    //下一句歌词/上一句歌词
}LRC;

/*******************************************************
 *歌词句柄(链表+指针数组)
 ********************************************************/
typedef struct lylics_ptr
{
	LRC *lrc_head;      //链表首地址
	LRC **lrc_arry;     //存放链表节点的指针数组的地址
	uint lrc_arry_size; //指针数组的大小
}LRC_PTR;

/*******************************************************
 *功能：     遍历输出链表
 *参数：		歌词句柄：lrc
 *返回值：	无
 ********************************************************/
extern void print_lrc_link(LRC_PTR lrc);

/*******************************************************
 *功能：     处理歌词文件
 *参数：		歌词文件名 ：name
  歌词句柄：lrc
 *返回值：	歌词信息结构体(链表)
 ********************************************************/
extern LRC *dispose_lrc(char *name, LRC_PTR *lrc);

/*******************************************************
 *功能：     处理歌词文件
 *参数：		歌词句柄：lrc
 *返回值：	NULL
 ********************************************************/
extern void free_lrc_arry(LRC_PTR *lrc);


typedef struct node {
	char* data;
	struct node* next;        
} Name ;
Name * create_node1(const char *filename);
Name * add_node(Name * head, char* newdata);
void free_list(Name * head);
Name * read_audio_files(const char* folder_path);
#endif
