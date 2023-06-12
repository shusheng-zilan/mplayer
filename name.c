#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include "othermain.h"
#include "mplayer_gtk.h"
#include "lyrics_process.h"
// 定义新的链表节点结构体
Name * create_node1(const char *filename) {
	Name *newnode = (Name *)malloc(sizeof(Name));
	newnode->data = (char *)malloc(strlen(filename) + 1);
	strcpy(newnode->data, filename);
	newnode->next = NULL;
	return newnode;
}

Name * add_node(Name * head, char* newdata) {
	Name *newnode = create_node1(newdata);
	if (head == NULL)
		return newnode;
	Name *temp = head;
	while (temp->next != NULL)
		temp = temp->next;
	temp->next = newnode;
	return head;
}

void free_list(Name * head) {
	while (head != NULL) {
		Name *temp = head;
		head = head->next;
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
int main()
{
	const char *music_dir = "../Music/";
	Name* file_node = read_audio_files(music_dir);
	while(file_node != NULL)
	{
		printf("%s\n", file_node->data);
		file_node = file_node ->next;
	}
}
//void my_player(MPLAYER player) {
//	const char *music_dir = "../Music/";
//	Name* file_node = read_audio_files(music_dir);
//	
//	// 构建 mplayer 命令行参数
//	while (file_node != NULL) {
//		execlp("mplayer","mplayer","-ac", "mad", "-slave","-quiet","-idle","-input","file=./song_fifo",file_node->data,NULL);
//		
//		perror("execlp");
//		
//		file_node = file_node->next;
//		if (file_node == NULL)
//			file_node = read_audio_files(music_dir);
//	}
//	free_list(file_node); // 释放所有已分配的内存
//	
//	exit(-1);
//	
//}
//int main() {
//	const char *music_dir = "/home/zilan/Music/";
//	AudioFile *audio_list = read_audio_files(music_dir);
//	AudioFile *current_audio = audio_list;
//	char a;
//	printf("Press any key to start...\n");
//	getchar(); // 等待玩家按键以便开始播放
//	printf("Playing audio files...\n");
//	int i  = 1;
//	while (i < 10)
//	{ // 循环遍历播放列表
//		i++;
//		printf("%s\n", current_audio->filename); // 打印当前要播放的文件名
//		if (current_audio->next == NULL) {
//			printf("No more audio files to play.\n");
//			break;
//		} 
//		else 
//		{
//			current_audio = current_audio->next; // 否则继续向下播放
//		}
//		printf("%s\n", current_audio->filename); // 打印当前要播放的文件名
//		if (current_audio->next == NULL)
//			current_audio = audio_list; // 如果到了列表末尾，则重新跳回第一个文件名
//		else
//			current_audio = current_audio->next; // 否则继续向下播放
//	}
//	free_audio(audio_list); // 释放内存，并退出
//	exit(0);
//}
int x(MPLAYER *player)
{
	const char* music_dir = "../Music/";
	Name* name_list = read_audio_files(music_dir);
	
}
