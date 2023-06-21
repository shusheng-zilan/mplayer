#ifndef __MPLAYER_GTK_H_
#define __MPLAYER_GTK_H_

extern int my_course( MPLAYER *player);

extern int get_menu(MPLAYER *player);
extern void window_my_gtk(MPLAYER *player);
extern void control(MPLAYER *player);
extern void my_player(MPLAYER *player);
extern gpointer my_read(gpointer arg);
extern void* send_player(void *arg);//向mplayer无限的去发
extern gboolean deal_mouse_press(GtkWidget *widget, GdkEventButton *event,gpointer user_data)  ;
extern void on_idle_update_progress_bar(gpointer data) ;
extern void on_main_window_destroy(GtkWidget *widget, gpointer data);
#endif
