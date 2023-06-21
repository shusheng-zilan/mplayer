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
#include "lyrics_process.h"
//修改图片的函数
void load_image(GtkWidget *tmp, const gchar *file_name, gint w, gint h)
{
	//清空原有图片既清空控件内容
	gtk_image_clear(GTK_IMAGE(tmp));
	//创建一个图片资源控件
	GdkPixbuf *src_pixbuf = gdk_pixbuf_new_from_file(file_name,NULL);
	//修改图片大小
	GdkPixbuf *dst_pixbuf = gdk_pixbuf_scale_simple(src_pixbuf,w,h,GDK_INTERP_BILINEAR);
	//将图片资源设置到图片控件中
	gtk_image_set_from_pixbuf(GTK_IMAGE(tmp),dst_pixbuf);
	//释放图片资源
	g_object_unref(src_pixbuf);
	g_object_unref(dst_pixbuf);
	
	return;
}
/* 设置label字体的大小
 * label: 需要操作的label
 * size:  字体的大小
 */
void set_label_font_size(GtkWidget *label, int size)
{
	PangoFontDescription *font;  	// 字体指针
	font = pango_font_description_from_string("");          //参数为字体名字，任意即可
	// #define PANGO_SCALE 1024
	// 设置字体大小   
	pango_font_description_set_size(font, size*PANGO_SCALE);
	gtk_widget_modify_font(label, font);  // 改变label的字体大小
	pango_font_description_free(font);	  // 释放字体指针分配的空间
}

//---------------------------------------------------进度条-----------------------------------------
void progress_bar_add( gpointer user_data,gdouble bar)
{
	gdouble value =0.0;
	//给进度条设置新的值
	value += (0.1*bar);
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(user_data),value);
}



//////-------------------------------暂停播放
void callback(GtkWidget *widget, gpointer data)
{
	int fd = ((MPLAYER *)data)->fd1;
	char cmd[128] = "";
	static gboolean is_playing = TRUE;
	GtkWidget *image = gtk_button_get_image(GTK_BUTTON(widget));
	
	if (is_playing) {
		load_image(image, "./button/close.png", 100, 100);
		is_playing = FALSE;
	} else {
		load_image(image, "./button/open.png", 100, 100);
		is_playing = TRUE;
	}
	write(fd, "pause\n", strlen("pause\n"));
}

//------------------------------------------------静音----------------------------------------------
void callback1(GtkWidget *widget, gpointer data)
{
	int fd =0;
	fd = ((MPLAYER *)data)->fd1;
	char cmd[128] ="";//cmd为向子进程发送的控制mplayer信号 
	
	static gboolean is_press = TRUE;
	GtkWidget *image = gtk_button_get_image( GTK_BUTTON(widget) ); // 获得按钮上面的图标
	
	if(TRUE == is_press){
		load_image(image, "./button/on.png",80,30);	// 自动图标换图标
		is_press = FALSE;
		strcpy(cmd,"mute 1\n");
		((MPLAYER *)data)->signal_a = 0;
	}
	else
	{
		load_image(image, "./button/off.png",80,30);	// 暂停图标换图标
		is_press = TRUE;
		strcpy(cmd,"mute 0\n");
		((MPLAYER *)data)->signal_a = 1;
	}
	if (((MPLAYER *)data)->signal_i == 0)
	{
		write(fd,cmd,strlen(cmd));
	}
}


//----------------------------------点击分栏列表 获取所点击目标---------------------------------

void my_select_row(GtkCList *clist, gint arg1, gint arg2, GdkEvent *arg3, MPLAYER *player) 
{
	//开发板 人为更新 对A8有效
	gtk_widget_queue_draw(GTK_WIDGET(clist));
	
	//根据arg1与arg2来获取当前行的内容
	gchar *text=NULL;
	gchar buf[128] ="";
	gtk_clist_get_text(GTK_CLIST(clist),arg1,arg2,&text);
	sscanf(text, "%[^.]", buf);//将字符串取出
	sprintf(player->song_name, "../Music/%s.mp3",buf);//组包
	sprintf(player->buf, "../lrc/%s.lrc", buf);//组包
	player->ii = arg1;//将歌曲所在行数赋值给ii，表示点击歌曲的编号
	char song_name[128] ="";
	sprintf(song_name, "loadfile %s\n", player->song_name);//再次组包为播放新歌曲命令
	write(player->fd1,song_name,strlen(song_name));//写入管道
	return;
}

//-------------------------------------------------------下一曲---------------------------------------

void next_song(GtkWidget *widget, gpointer data)
{
	MPLAYER *player = (MPLAYER *)data;
	int i = 0;
	char song_name[128] = "";
	
	if (player->max == player->ii) {
		i = 0;
	} else {
		i = player->ii + 1;
	}
	player->ii = i;
	Name *song_list = read_audio_files("../Music");
	if (song_list != NULL) {
		Name *current = song_list;
		for (int j = 0; j < i; j++) {
			current = current->next;
		}
		sprintf(song_name, "loadfile ../Music/%s\n", current->data);
		free_list(song_list);
		write(player->fd1, song_name, strlen(song_name));
	}
}




//------------------------------------------------上一曲--------------------------------------------
void last_song(GtkWidget *widget, gpointer data)
{
	MPLAYER *player = (MPLAYER *)data;
	int i = 0;
	char song_name[128] = "";
	
	if (player->ii == 0) {
		i = player->max;
	} else {
		i = player->ii - 1;
	}
	
	player->ii = i;
	Name *song_list = read_audio_files("../Music");
	if (song_list != NULL) {
		Name *current = song_list;
		for (int j = 0; j < i; j++) {
			current = current->next;
		}
		sprintf(song_name, "loadfile ../Music/%s\n", current->data);
		free_list(song_list);
		if (player->signal_i == 0) {
			write(player->fd1, song_name, strlen(song_name));
		}
	}
}



// ------------------------------------------歌词处理-------------------------------------------
gpointer the_lrc(gpointer arg)
{
	MPLAYER *player = (MPLAYER *)arg;
	LRC_PTR lrc;
	LRC *head = NULL;
	
	while (1)  //暂时没找到合适的条件
	{
		if (player->signal_i == 0 && player->file_flag == 1)
		{
			usleep(100000);  // 每次休眠一定时间，避免频繁调用 dispose_lrc 函数
			head = dispose_lrc(player->buf, &lrc);
			player->file_flag = 0;
			
			if (head == NULL)  // 解析歌词失败
			{
				printf("No this lyrics\n");
			}
			else
			{
				int i;
				
				for (i = 0; i < lrc.lrc_arry_size; i++)
				{
					if (lrc.lrc_arry[i]->time > player->lrc_time)  // 找到下一个要显示的歌词
					{
						gdk_threads_enter();
						gtk_label_set_text(GTK_LABEL(player->lable_song_lrc), lrc.lrc_arry[i]->lrc);
						gdk_threads_leave();
						break;
					}
				}
				
				if (i >= lrc.lrc_arry_size)  // 已经到达末尾，没有更多歌词可以显示了
				{
					gdk_threads_enter();
					gtk_label_set_text(GTK_LABEL(player->lable_song_lrc), "");
					gdk_threads_leave();
				}
			}
			
			free_lrc_arry(&lrc);
		}
		usleep(10000);  // 每次休眠一定时间，减少 CPU 占用率
	}
	
	return NULL;
}

int sungtk_widget_set_font_color(GtkWidget *widget, const char *color_buf, gboolean is_button)
{
	if(widget == NULL && color_buf==NULL)
		return -1;
	
	GdkColor color;
	GtkWidget *labelChild = NULL;
	sungtk_color_get(color_buf, &color);
	if(is_button == TRUE){
		labelChild = gtk_bin_get_child(GTK_BIN(widget));//取出GtkButton里的label  
		gtk_widget_modify_fg(labelChild, GTK_STATE_NORMAL, &color);
		gtk_widget_modify_fg(labelChild, GTK_STATE_SELECTED, &color);
		gtk_widget_modify_fg(labelChild, GTK_STATE_PRELIGHT, &color);
	}
	else
	{
		gtk_widget_modify_fg(widget, GTK_STATE_NORMAL, &color);
	}
	return 0;
}


int sungtk_color_get(const char *color_buf, GdkColor *color)
{
	gdk_color_parse(color_buf, color);
	return 0;
}
//搜索
void search_song(GtkWidget *widget, gpointer data) {
	MPLAYER *player = (MPLAYER *)data;
	const gchar *search_text = gtk_entry_get_text(GTK_ENTRY(player->search_entry));
	
	// 遍历歌曲文件夹，匹配搜索文本
	Name *song_list = read_audio_files("../Music");
	Name *current = song_list;
	while (current != NULL) {
		if (strstr(current->data, search_text) != NULL) {
			// 找到匹配的歌曲
			gchar buf[128] = "";
			sscanf(current->data, "%[^.]", buf);
			sprintf(player->song_name, "../Music/%s.mp3", buf);
			sprintf(player->buf, "../lrc/%s.lrc", buf);
			
			// 更新播放器状态
			char song_name[128] = "";
			sprintf(song_name, "loadfile %s\n", player->song_name);
			write(player->fd1, song_name, strlen(song_name));
			
			// 释放歌曲列表内存
			free_list(song_list);
			return;
		}
		current = current->next;
	}
	
	// 如果没有找到匹配的歌曲，可以在这里添加一些提示或其他操作
	
	// 释放歌曲列表内存
	free_list(song_list);
}


