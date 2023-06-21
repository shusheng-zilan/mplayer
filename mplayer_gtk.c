#include <gtk/gtk.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "othermain.h"
#include "mplayer_gtk.h"
#include <dirent.h>
#include "lyrics_process.h"

static void chang_background(GtkWidget *widget, int w, int h, const gchar *path)
{
	gtk_widget_set_app_paintable(widget, TRUE);		//允许窗口可以绘图
	gtk_widget_realize(widget);	
	/* 更改背景图时，图片会重叠
	 * 这时要手动调用下面的函数，让窗口绘图区域失效，产生窗口重绘制事件（即 expose 事件）。
	 */
	gtk_widget_queue_draw(widget);
	
	GdkPixbuf *src_pixbuf = gdk_pixbuf_new_from_file(path, NULL);	// 创建图片资源对象
	// w, h是指定图片的宽度和高度
	GdkPixbuf *dst_pixbuf = gdk_pixbuf_scale_simple(src_pixbuf, w, h, GDK_INTERP_BILINEAR);
	GdkPixmap *pixmap = NULL;
	
	/* 创建pixmap图像; 
	 * NULL：不需要蒙版; 
	 * 123： 0~255，透明到不透明
	 */
	gdk_pixbuf_render_pixmap_and_mask(dst_pixbuf, &pixmap, NULL, 175);
	// 通过pixmap给widget设置一张背景图，最后一个参数必须为: FASLE
	gdk_window_set_back_pixmap(widget->window, pixmap, FALSE);
	
	// 释放资源
	g_object_unref(src_pixbuf);
	g_object_unref(dst_pixbuf);
	g_object_unref(pixmap);
}


//------------------------------------主窗口----------------------------------------
int my_course( MPLAYER *player)
{
	pthread_t pth_id;
	pthread_create(&pth_id, NULL, window_my_gtk,player);
	sleep(4);
	//----------------------------------定义第一个管道------------------------------
	pipe(player->fd);//先创建无名管道
	pid_t pid = 0;
	pid = fork();
	
	if (pid <0)
	{
		perror("fork");
	}
	//-------------------------------- 创建进程 及子进程--------------------------------
	else if (pid == 0)//子进程
	{
		dup2(player->fd[1],1);
		//-----------------------------调用execlp启动mplayer----------------------------
		my_player(player);
	}
	
	control(player);//父进程建立命名管道
	
	pthread_t pth_id3;
	pthread_create(&pth_id3, NULL, send_player,(void *)player);
	g_thread_create((GThreadFunc)my_read, player,FALSE, NULL);
	g_thread_create((GThreadFunc)the_lrc, player, FALSE, NULL);
	return 0;
}

//---------------------------------------------------------------------------------------------------
void*  send_player(void *arg)//向mplayer无限的去发  
{
	MPLAYER *player = (MPLAYER *)arg;
	while(1)
	{
		if (player->signal_i == 0)
		{
			write(player->fd1, "get_time_pos\n", strlen("get_time_pos\n"));//获取当前播放的时间命令
			usleep(100*1000);
			write(player->fd1, "get_time_length\n", strlen("get_time_length\n"));//获取总时长
			usleep(100*1000);
			write(player->fd1, "get_percent_pos\n", strlen("get_percent_pos\n"));//获取当前播放进度
			usleep(100*1000);
			write(player->fd1, "get_meta_album\n", strlen("get_meta_album\n"));//获取专辑名
			usleep(100*1000);
			write(player->fd1, "get_file_name\n", strlen("get_file_name\n"));//获取当前歌曲文件名
			usleep(100*1000);
			write(player->fd1, "get_meta_artist\n", strlen("get_meta_artist\n"));//获取歌手名
			usleep(100*1000);
		}
	}
}


void my_player(MPLAYER *player)//开启mplayer
{
	const char* music_dir = "../Music/";
	Name* name_list = read_audio_files(music_dir);
	while (name_list != NULL)
	{
		char path[100] = "../Music/";
		char *file_name = name_list->data;
		strcat(path, file_name);
		execlp("mplayer","mplayer","-ac", "mad", "-slave","-quiet","-idle","-input","file=./song_fifo",path,NULL);
		perror("execlp");
		name_list = name_list->next;
	}
	free_list(name_list);
	exit(-1);	
}



//-----------------------------------------------父进程控制中心--------------------------------------
void control(MPLAYER *player)
{
	mkfifo("./song_fifo", 0666);
	player->fd1 = open("./song_fifo",O_WRONLY);
	char open_song_name[128]="";
	strcpy(open_song_name,player->song_name);
}



//————————————————————————————————————————————————主窗口----------------------------------------------

void window_my_gtk(MPLAYER *player)
{
	player->main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_widget_set_size_request(player->main_window, 802, 482);
	gtk_container_set_border_width(GTK_CONTAINER(player->main_window), 1);
	chang_background(player->main_window , 800, 480, "./background/111.png");
	gtk_widget_add_events(player->main_window, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_MOTION_MASK);
	//移动鼠标
	g_signal_connect(player->main_window, "button-press-event",G_CALLBACK(deal_mouse_press),player);
	//关闭当前主窗口时退出
	g_signal_connect(player->main_window, "destroy", G_CALLBACK(on_main_window_destroy),NULL);
	
	//----------------------------表格布局--------------------------------------------
	//创建table表格布局容器
	player->table = gtk_table_new(48, 80, TRUE);
	//将表格布局容器添加到主窗口中
	gtk_container_add(GTK_CONTAINER(player->main_window),player->table);
	
	//-----------------搜索框
	player->search_entry = gtk_entry_new();
	gtk_table_attach_defaults(GTK_TABLE(player->table), player->search_entry, 57, 74, 4, 7);
	
	player->search_button = gtk_button_new_with_label("搜索");
	gtk_table_attach_defaults(GTK_TABLE(player->table), player->search_button, 74, 80, 4, 7);
	g_signal_connect(player->search_button, "clicked", G_CALLBACK(search_song), player);
	
	//------------------------------按钮---------------------------------------------
	//创建button_off
	player->button_off = gtk_button_new_with_label("");
	gtk_table_attach_defaults(GTK_TABLE(player->table),player->button_off,64,72, 41,44);
	player->image = gtk_image_new_from_pixbuf(NULL);
	load_image(player->image, "./button/off.png",80,30);  
	gtk_button_set_relief(GTK_BUTTON(player->button_off),GTK_RELIEF_NONE);
	gtk_button_set_image(GTK_BUTTON(player->button_off),player->image);

	//创建button_open
	player->button_open = gtk_button_new_with_label("");
	gtk_table_attach_defaults(GTK_TABLE(player->table),player->button_open,36,46, 37,47);
	player->image02 = gtk_image_new_from_pixbuf(NULL);
	load_image(player->image02, "./button/open.png",100,100);  
	gtk_button_set_relief(GTK_BUTTON(player->button_open),GTK_RELIEF_NONE);
	gtk_button_set_image(GTK_BUTTON(player->button_open),player->image02);	
	
	//创建button_back
	player->button_back = gtk_button_new_with_label("");
	gtk_table_attach_defaults(GTK_TABLE(player->table),player->button_back,29,36, 39,45);
	player->image01 = gtk_image_new_from_pixbuf(NULL);
	load_image(player->image01, "./button/back.png",60,60);  
	gtk_button_set_relief(GTK_BUTTON(player->button_back),GTK_RELIEF_NONE);
	gtk_button_set_image(GTK_BUTTON(player->button_back),player->image01);
	
	
	//创建button_front
	player->button_front = gtk_button_new_with_label("");
	gtk_table_attach_defaults(GTK_TABLE(player->table),player->button_front,48,54, 39,45);
	player->image03 = gtk_image_new_from_pixbuf(NULL);
	load_image(player->image03, "./button/front.png",60,60);  
	gtk_button_set_relief(GTK_BUTTON(player->button_front),GTK_RELIEF_NONE);
	gtk_button_set_image(GTK_BUTTON(player->button_front),player->image03);
	
	//创建button_menu
	player->button_menu = gtk_button_new_with_label("");
	gtk_table_attach_defaults(GTK_TABLE(player->table),player->button_menu,3,9, 40,47);
	player->image04 = gtk_image_new_from_pixbuf(NULL);
	load_image(player->image04, "./button/menu.png",50,60);  
	gtk_button_set_relief(GTK_BUTTON(player->button_menu),GTK_RELIEF_NONE);
	gtk_button_set_image(GTK_BUTTON(player->button_menu),player->image04);
	
	//------------------------------------创建分栏列表--------------------------------------
	gchar *titles[1] ={"歌单列表"};
	player->clist = gtk_clist_new_with_titles(1,titles);
	gtk_table_attach_defaults(GTK_TABLE(player->table),player->clist,57,74, 7,37);
	gtk_clist_set_column_width(GTK_CLIST(player->clist),0,110);
	gtk_clist_set_column_justification(GTK_CLIST(player->clist),0,GTK_JUSTIFY_LEFT);
	get_menu(player);
	
	//------------------------------文本---------时间-----------------------------------------------
	//创建文本
	player->lable_song_time = gtk_label_new("0:0");
	sungtk_widget_set_font_color(player->lable_song_time,"white",NULL);
	gtk_table_attach_defaults(GTK_TABLE(player->table),player->lable_song_time,50,57, 31,34);
	
	
	//创建文本
	player->lable_now_time = gtk_label_new("0:0");
	sungtk_widget_set_font_color(player->lable_now_time,"white",NULL);
	gtk_table_attach_defaults(GTK_TABLE(player->table),player->lable_now_time,25,31, 31,34);
	
	//--------------------------------------------进度条----------------------------
	//创建一个进度条控件
	player->progress_bar = gtk_progress_bar_new();
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(player->progress_bar),"");
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(player->progress_bar),0.0);
	gtk_table_attach_defaults(GTK_TABLE(player->table),player->progress_bar,25,55, 35,36);
	player->ii =1;//由于默认播放的音乐编号为1，所以将ii设为初始1
	
	//创建一个进度条控件
	player->progress_bar2 = gtk_progress_bar_new();
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(player->progress_bar2),"");
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(player->progress_bar2), 0.14);
	gtk_progress_bar_set_orientation(GTK_PROGRESS_BAR(player->progress_bar2), GTK_PROGRESS_BOTTOM_TO_TOP);
	gtk_table_attach_defaults(GTK_TABLE(player->table),player->progress_bar2,75,76, 7,37);
	//------------------------------歌曲名称及歌词、演唱者信息 文本----------------------------------------
	
	//创建文本
	player->lable_song_name = gtk_label_new("111");
	gtk_label_set_text(GTK_LABEL(player->lable_song_name),"歌曲名称");
	set_label_font_size(player->lable_song_name, 17);
	gtk_table_attach_defaults(GTK_TABLE(player->table),player->lable_song_name,31,48, 7,11);
	sungtk_widget_set_font_color(player->lable_song_name,"white",NULL);
	
	//创建文本
	player->lable_song_artist = gtk_label_new("  歌手  ");
	gtk_label_set_text(GTK_LABEL(player->lable_song_artist),"歌手");
	gtk_table_attach_defaults(GTK_TABLE(player->table),player->lable_song_artist,31,48, 11,14);
	sungtk_widget_set_font_color(player->lable_song_artist,"white",NULL);
	
	//创建文本
	player->lable_song_lrc = gtk_label_new("  歌词  ");
	gtk_label_set_text(GTK_LABEL(player->lable_song_lrc),"歌词");
	set_label_font_size(player->lable_song_lrc, 17);
	gtk_table_attach_defaults(GTK_TABLE(player->table),player->lable_song_lrc,25,55, 15,30);
	sungtk_widget_set_font_color(player->lable_song_lrc,"white",NULL);
	
	//-------------------------------------动起来-------------------------------------------
	g_signal_connect(player->button_open,"clicked", G_CALLBACK(callback), player);//暂停控制
	g_signal_connect(player->button_off,"clicked", G_CALLBACK(callback1), player);//声音控制
	g_signal_connect(player->clist,"select_row", G_CALLBACK(my_select_row), player);//点击分栏列表
//	g_signal_connect(player->button_menu,"clicked", G_CALLBACK(deal_button), player);//菜单备用键
	g_signal_connect(player->button_front,"clicked", G_CALLBACK(next_song), player);//上一曲
	g_signal_connect(player->button_back,"clicked", G_CALLBACK(last_song), player);//下一曲
	//------------------------------------------------------------------------------------
	
	
	gtk_widget_show_all(player->main_window);
	gtk_main();
	return;
}

/////-----------歌曲的列表
int get_menu(MPLAYER *player)
{
	Name *head = read_audio_files("../Music"); // 调用 read_audio_files() 函数获取文件名链表
	int i = 0;
	player->max = 0;
	while (head != NULL ) {
		if (strlen(head->data) < 3) {
			continue;
		}
		player->song_list[i] = (char*)malloc(strlen(head->data) + 1);
		strcpy(player->song_list[i], head->data);
		gtk_clist_append(GTK_CLIST(player->clist), &player->song_list[i]);
		i++;
		player->max++;
		head = head->next;
	}
	free_list(head);
	
	return 0;
}

//---------------------------------------从无名管道中读取数据----------------------------------------------
gpointer my_read(gpointer arg)
{
	MPLAYER *player = (MPLAYER *)arg;
	
	while(1)
	{
		
		if (player->signal_i == 0)
		{
			char ever_msg[256] = "";//用于接传来的命令返回信息
			char time_now[64] = "";//字符串形式的时间
			char time_tal[64] = "";//将要向时间文本中写入的时间字符串
			int now_time=0, now_time_s=0,tal_time=0;
			char song_msg[128] = "";
			char singer_msg[128] = "";
			char song[128] = "";//歌曲名
			
			read(player->fd[0], ever_msg, sizeof(ever_msg));//从无名管道中读取返回信息
			int value =0;//度
			if(strncmp(ever_msg, "ANS_TIME_POSITION=", 18) ==0)//现在时间
			{
				sscanf(ever_msg, "%*18s%d.%d", &now_time_s,&now_time);//裁剪
				sprintf(time_now, "%d:%d", now_time_s/60, now_time_s%60);//组包
				player->lrc_time = (now_time_s/60)*60000 + (now_time_s%60)*1000;//赋值
				player->now_time_s = now_time_s;
				gdk_threads_enter();	// 进入多线程互斥区域
				gtk_label_set_text(GTK_LABEL(player->lable_now_time), time_now);//将现在时间写入gtk页面中的文本
				gdk_threads_leave();	// 退出多线程互斥区域	
			}
			
			if (strncmp(ever_msg, "ANS_PERCENT_POSITION=", 21) ==0)//进度
			{
				sscanf(ever_msg, "%*21s%d", &value);//裁剪
				//列表顺序播放
				if( (value*0.01) == 0.99)//判断进度条是不是达到1  到了1 进行下一曲
				{
					char song_bufs[256] = "";
					sprintf(song_bufs, "loadfile ../Music/%s\n", player->song_list[player->ii+1]);//组包成命令
					player->ii ++;
					write(player->fd1, song_bufs, strlen(song_bufs));//将列表顺序播放的命令发给mplayer
					
				}
				int x = rand() % 30;
				gdk_threads_enter();	// 进入多线程互斥区域
				gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(player->progress_bar),value*0.01);//写入进度条
				if (x <= 10)
					gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(player->progress_bar2),player->yy);
				gdk_threads_leave();	// 退出多线程互斥区域
				
			}
			
			if(strncmp(ever_msg, "ANS_LENGTH=", 11) ==0)//歌曲总时长
			{
				sscanf(ever_msg, "%*11s%d", &tal_time);//裁剪
				sprintf(time_tal, "%d:%d", tal_time/60, tal_time%60);//组包
				player->tal_time = tal_time;
				gdk_threads_enter();	// 进入多线程互斥区域
				gtk_label_set_text(GTK_LABEL(player->lable_song_time), time_tal);//将时间写入页面的总时间文本内
				gdk_threads_leave();	// 退出多线程互斥区域
				
			}
			
			if(strncmp(ever_msg, "ANS_FILENAME=", 13) ==0)//当前播放歌曲名称（文件名称）
			{
				char file_name[128] = "";
				sscanf(ever_msg, "%*14s%[^']", song_msg);//裁剪
				sprintf(player->song, "%s", song_msg);
				sscanf(song_msg, "%s.mp3", song);
				
				sscanf(song, "%[^.]", song);//去除。mp3后缀
				
				sprintf(player->buf, "../lrc/%s.lrc", song);//重新组包
				
				if(strcmp(file_name, song_msg) != 0)//判断是否改变（切歌）
				{
					strcpy(file_name, song_msg);//切歌更改歌曲名
					player->file_flag = 1;
				}
				gdk_threads_enter();	// 进入多线程互斥区域
				gtk_label_set_text(GTK_LABEL(player->lable_song_name), player->song);
				gdk_threads_leave();	// 退出多线程互斥区域
			}
			if(strncmp(ever_msg, "ANS_META_ARTIST=", 16) ==0)//获取歌手信息
			{
				sscanf(ever_msg, "%*17s%[^']", singer_msg);
				sprintf(player->singer, "%s", singer_msg);
				gdk_threads_enter();	// 进入多线程互斥区域
				gtk_label_set_text(GTK_LABEL(player->lable_song_artist), player->singer);
				gdk_threads_leave();	// 退出多线程互斥区域
			}
		}	
	}
}

//--------------------对于右边文本框，实现拖动进度条的功能 以及调节音量的功能----------------------------//
gboolean deal_mouse_press(GtkWidget *widget, GdkEventButton *event,gpointer user_data)  
{
	MPLAYER *player = (MPLAYER *)user_data;
	long nowtime = 0;
	int taltime = 0;
	long newtime = 0;
	int volume = 0;
	if ((event->x >250) && (event->x <550) && ((event->y >340) && (event->y < 370)))
	{
		player->xx = (gint)((((((event->x)/10)) - 25)*100)/30)*(0.01);
		taltime = player->tal_time ;
		newtime = (int)((player->xx)*taltime);
		newtime = newtime - player->now_time_s;
		int i = 0;
		i = player->ii;
		char thetime[128] = "";
		sprintf(thetime, "seek %ld\n", newtime);
		write(player->fd1, thetime, strlen(thetime));
	}
	else if  ((event->x >740) && (event->x <770) && ((event->y >70) && (event->y < 370)))
	{
		printf("event->y    %lf\n", event->y);
		player->yy = 1 - ((((event->y)/10) - 7)/30);
		printf("yy = %lf\n", player->yy);
		volume = (int)(player->yy *100);
		
		char thetime[128] = "";
		sprintf(thetime, "volume %d 1\n", volume);
		printf("thetime =  %s\n", thetime);
		write(player->fd1, thetime, strlen(thetime));
	}
}


// 退出主窗口时的回调函数
void on_main_window_destroy(GtkWidget *widget, gpointer data) {
	// 杀死Mplayer进程
	system("killall mplayer");
	
	// 退出GTK+主循环
	gtk_main_quit();
}

