#include <gtk/gtk.h>
#include <unistd.h>     //add read/open/.... declaration
#include <stdio.h>      //add print()
#include <stdlib.h>     //exit()
#include <fcntl.h>    //add struct of open/erad/wirted
#include <time.h>
#include <dirent.h> // opendir
#include <sys/types.h> //
#include <signal.h> // kill
#include <string.h>
#include <errno.h>
// gcc exp4.c -o exp4 `pkg-config --cflags --libs gtk+-3.0`

typedef struct SL{
    char *text;
    GtkWidget *Label;
}SL;

int updateHostName(GtkWidget *hostLabel);
int setStartTime(GtkWidget *startTimeLabel);
gboolean updateUptime(GtkWidget *uptimeLabel);
int setKernelInfo(GtkWidget *KInfoLabel);
int setCpuInfo(GtkWidget *CInfoLabel);
int setPidInfo(GtkListStore *PidStore);
int updatePidInfo(gpointer data);

int deletePid(gpointer data);

int openEntry_callback(GtkWidget *widget, gpointer entry);
int showPopWin(GtkWidget *widget, GdkEvent *event);
void showError(GtkWidget *window);
void sig_child();
int rebootsys();
int shutdownsys();
int clickSort(GtkTreeViewColumn *tree_column);

gboolean updateCpuRateInfo(char text[]);
gboolean updateMemRateInfo();
gboolean updateTimeInfo();

gboolean drawRateGraph_cpu(GtkWidget *widget, cairo_t *cr, gpointer data);
gboolean drawRateGraph_mem(GtkWidget *widget, cairo_t *cr, gpointer data);
gboolean drawRateGraph_swap(GtkWidget *widget, cairo_t *cr, gpointer data);

gboolean updateLabelInfo(SL *sl);

int setText2Label(GtkWidget *Label, char text[]);

enum{
    NAME_COL,
    MEMORY_COL,
    PID_COL,
    PPID_COL,
    STATUS_COL,
    PRO_COL,
    COL_NUM,
};

gchar *proInfoList[6] = {"进程名", "内存占用", "进程号", "父进程号", "进程状态", "动态优先级" };

GtkWidget *listStore;
GtkWidget *listTree;
GtkWidget *window;
GtkTreeModel *listSortModel;
GtkWidget *searchTLabel;

gdouble prefree = 0;
gdouble pretotal = 0;
gdouble point_cpu[120] = {0};
gdouble point_mem[120] = {0};
gdouble point_swap[120] = {0};

char CRInfoText[600] = {'/0'};
char MRInfoText[600] = {'/0'};
char SRInfoText[600] = {'/0'};
char TInfoText[600] = {'/0'};
char sortTitle[30] = {'/0'};

int sortsate[6] = {0};

int main (int argc, char *argv[])
{
    SL temsl;
    GtkWidget *menu;
    
    gtk_init(&argc, &argv);
    
    // 窗口初始化
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW (window), g_locale_to_utf8("任务管理器",-1,NULL,NULL,NULL));
    gtk_window_resize(GTK_WINDOW(window), 800, 600);
    // gtk_widget_set_default_size(GTK_WINDOW(window), 300, 400);
    // gtk_window_set_resizable (GTK_WINDOW (window), FALSE);// 窗口大小可改变（TRUE）
    gtk_container_set_border_width (GTK_CONTAINER (window), 5);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // 窗口分栏
    menu = gtk_vbox_new(FALSE, 0);
    gtk_container_add (GTK_CONTAINER (window), menu);

    // 对上窗口分页
    GtkWidget *notebook = gtk_notebook_new();
    gtk_notebook_set_tab_pos(GTK_NOTEBOOK (notebook), GTK_POS_TOP);
    // gtk_box_pack_start(GTK_BOX (menu), notebook, FALSE, FALSE,0);
    gtk_container_add (GTK_CONTAINER (menu), notebook);

    // 第一页：进程信息----------------
    GtkWidget *proLabel = gtk_label_new("进程信息");
    GtkWidget *proMenu = gtk_vbox_new(FALSE, 0);
    gtk_notebook_append_page(GTK_NOTEBOOK (notebook), proMenu, proLabel);

    GtkWidget *SOMenu = gtk_hbox_new(FALSE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (SOMenu), 5);
    gtk_box_pack_start(GTK_BOX (proMenu), SOMenu, FALSE, FALSE,0);

    GtkWidget *openMenu = gtk_hbox_new(FALSE, 0);
    // gtk_container_set_border_width (GTK_CONTAINER (searchMenu), 5);
    gtk_box_pack_start(GTK_BOX (SOMenu), openMenu, FALSE, FALSE,0);

    GtkWidget *openTLabel = gtk_label_new("运行:");
    gtk_box_pack_start(GTK_BOX (openMenu), openTLabel, FALSE, FALSE,0);

    GtkWidget *openEntry = gtk_entry_new();
    gtk_widget_set_size_request(openEntry , 20, 20);
    g_signal_connect(G_OBJECT (openEntry), "activate", G_CALLBACK(openEntry_callback), openEntry);
    gtk_box_pack_start(GTK_BOX (openMenu), openEntry, FALSE, FALSE,0);

    GtkWidget *nullMenu = gtk_hbox_new(FALSE, 0);
    // gtk_container_set_border_width (GTK_CONTAINER (searchMenu), 5);
    gtk_box_pack_start(GTK_BOX (SOMenu), nullMenu, TRUE, FALSE,0);

    GtkWidget *searchMenu = gtk_hbox_new(FALSE, 0);
    // gtk_container_set_border_width (GTK_CONTAINER (searchMenu), 5);
    gtk_box_pack_start(GTK_BOX (SOMenu), searchMenu, FALSE, FALSE,0);

    searchTLabel = gtk_label_new("进程名:");
    gtk_box_pack_start(GTK_BOX (searchMenu), searchTLabel, FALSE, FALSE,0);

    GtkWidget *searchEntry = gtk_search_entry_new();
    gtk_box_pack_start(GTK_BOX (searchMenu), searchEntry, FALSE, FALSE,0);
    
    // -----添加滑动-------------------
    GtkWidget *proScro = gtk_scrolled_window_new(NULL,NULL);
    gtk_widget_set_size_request(proScro, 100, 100);
    gtk_container_add (GTK_CONTAINER (proMenu), proScro);

    // ----------添加树形列表------------
    listStore = gtk_list_store_new(6, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT, G_TYPE_STRING, G_TYPE_INT);
    // listSortModel = gtk_tree_model_sort_new_with_model(GTK_TREE_MODEL (listStore));
    listTree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (listStore));
    gtk_container_add (GTK_CONTAINER (proScro), listTree);

    // ----------添加列表头--------------
    GtkCellRenderer *rend;
    GtkTreeViewColumn *column;
    for(int i = 0; i < 6; i++)
    {
        rend = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes(proInfoList[i],rend, "text", i, NULL);
        
        g_signal_connect(G_OBJECT (column), "clicked", G_CALLBACK(clickSort), NULL);
        gtk_tree_view_column_set_sort_indicator(column, TRUE);
        gtk_tree_view_column_set_sort_column_id(column, i);
        
        gtk_tree_view_append_column(GTK_TREE_VIEW(listTree),GTK_TREE_VIEW_COLUMN(column));
    }

    gtk_tree_view_set_enable_search(listTree, TRUE);
    // gtk_tree_view_set_headers_clickable(listTree, TRUE);

    gtk_tree_view_set_search_entry(listTree, searchEntry);

    // ----------从文件中读取pid信息并写入列表---
    setPidInfo(listStore);
    // gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (listSortModel), 0, GTK_SORT_ASCENDING);


    // -----第一页按键菜单----------------
    GtkWidget *buttonMenu = gtk_hbox_new(FALSE, 10);
    gtk_box_pack_start(GTK_BOX (proMenu), buttonMenu, FALSE, FALSE,0);
    // gtk_container_add (GTK_CONTAINER (proMenu), buttonMenu);
    gtk_container_set_border_width (GTK_CONTAINER (buttonMenu), 5);

    // GtkWidget *UPBfixed = gtk_fixed_new();
    // gtk_container_add (GTK_CONTAINER (buttonMenu), UPBfixed);
    
    GtkWidget *updatePidButton = gtk_button_new_with_label ("刷新信息");
    g_signal_connect(G_OBJECT (updatePidButton), "clicked", G_CALLBACK(updatePidInfo), NULL);
    // gtk_fixed_put(GTK_FIXED(UPBfixed), updatePidButton, 0, 0);
    // gtk_container_add (GTK_CONTAINER (buttonMenu), updatePidButton);
    gtk_widget_set_size_request(updatePidButton, 70, 30);
    gtk_box_pack_start(GTK_BOX (buttonMenu), updatePidButton, TRUE, FALSE,0);

    GtkWidget *deletePidButton = gtk_button_new_with_label ("结束进程");
    g_signal_connect(G_OBJECT (deletePidButton), "clicked", G_CALLBACK(deletePid), NULL);
    // gtk_fixed_put(GTK_FIXED(UPBfixed), deletePidButton, 200, 0);
    // gtk_container_add (GTK_CONTAINER (buttonMenu), deletePidButton);
    gtk_widget_set_size_request(deletePidButton, 70, 30);
    gtk_box_pack_start(GTK_BOX (buttonMenu), deletePidButton, TRUE, FALSE,0);


    GtkWidget *popMenu = gtk_menu_new();
    g_signal_connect_swapped(G_OBJECT(listTree), "button-press-event", G_CALLBACK(showPopWin), popMenu);

    GtkWidget *delEvent = gtk_menu_item_new_with_label("结束进程");
	gtk_widget_show(delEvent);
    g_signal_connect(G_OBJECT (delEvent), "activate", G_CALLBACK(deletePid), NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(popMenu), delEvent);

    // 第二页：资源信息---------------
    GtkWidget *resLabel = gtk_label_new("资源信息");
    GtkWidget *resMenu = gtk_vbox_new(FALSE, 0);
    gtk_notebook_append_page(GTK_NOTEBOOK (notebook), resMenu, resLabel);

    // -----对资源页分页-------------
    GtkWidget *resnotebook = gtk_notebook_new();
    gtk_notebook_set_tab_pos(GTK_NOTEBOOK (resnotebook), GTK_POS_TOP);
    gtk_container_add (GTK_CONTAINER (resMenu), resnotebook);
    gtk_notebook_set_tab_pos(GTK_NOTEBOOK (resnotebook), GTK_POS_LEFT);

    // -----第二页的cpu页的tag---------
    updateCpuRateInfo(CRInfoText);
    g_timeout_add(1000, updateCpuRateInfo, CRInfoText);

    GtkWidget *CRTagLabel = gtk_label_new("CPURate");
    setText2Label(CRTagLabel, CRInfoText);

    GtkWidget *resCMenu = gtk_vbox_new(FALSE, 0);
    gtk_notebook_append_page(GTK_NOTEBOOK (resnotebook), resCMenu, CRTagLabel);

    // 构建cpu的框架
    GtkWidget *resFrame_cpu = gtk_frame_new("cpu利用率");
    gtk_box_pack_start(GTK_BOX (resCMenu), resFrame_cpu, FALSE, FALSE,0);

    GtkWidget *drawArea_cpu = gtk_drawing_area_new();
    gtk_widget_set_size_request (drawArea_cpu, 500, 500);
    g_signal_connect (G_OBJECT (drawArea_cpu), "draw", G_CALLBACK (drawRateGraph_cpu), NULL);
    gtk_container_add (GTK_CONTAINER (resFrame_cpu), drawArea_cpu);

    // GtkWidget *CRInfoLabel = gtk_label_new("CPURate");
    // setText2Label(CRInfoLabel, CRInfoText);
    // gtk_container_add (GTK_CONTAINER (resCMenu), CRInfoLabel);


    // -----第二页的内存页的tag---------
    updateMemRateInfo();
    g_timeout_add(1000, updateMemRateInfo, NULL);
    
    GtkWidget *resMLabel = gtk_label_new("内存");
    setText2Label(resMLabel, MRInfoText);

    GtkWidget *resMMenu = gtk_vbox_new(FALSE, 0);
    gtk_notebook_append_page(GTK_NOTEBOOK (resnotebook), resMMenu, resMLabel);

    // 构建内存的框架
    GtkWidget *resFrame_mem = gtk_frame_new("内存使用率");
    gtk_box_pack_start(GTK_BOX (resMMenu), resFrame_mem, FALSE, FALSE,0);

    GtkWidget *drawArea_mem = gtk_drawing_area_new();
    gtk_widget_set_size_request (drawArea_mem, 500, 500);
    g_signal_connect (G_OBJECT (drawArea_mem), "draw", G_CALLBACK (drawRateGraph_mem), NULL);
    gtk_container_add (GTK_CONTAINER (resFrame_mem), drawArea_mem);

    
    // 第三页的内存页
    GtkWidget *resSLabel = gtk_label_new("swap");
    setText2Label(resSLabel, SRInfoText);
    GtkWidget *resSMenu = gtk_vbox_new(FALSE, 0);
    gtk_notebook_append_page(GTK_NOTEBOOK (resnotebook), resSMenu, resSLabel);

    // 构建swap的框架
    GtkWidget *resFrame_swap = gtk_frame_new("swap使用率");
    gtk_box_pack_start(GTK_BOX (resSMenu), resFrame_swap, FALSE, FALSE,0);

    GtkWidget *drawArea_swap = gtk_drawing_area_new();
    gtk_widget_set_size_request (drawArea_swap, 500, 500);
    g_signal_connect (G_OBJECT (drawArea_swap), "draw", G_CALLBACK (drawRateGraph_swap), NULL);
    gtk_container_add (GTK_CONTAINER (resFrame_swap), drawArea_swap);


    // 第三页：系统信息
    GtkWidget *sysLabel = gtk_label_new("系统信息");
    GtkWidget *sysMenu = gtk_vbox_new(FALSE, 0);
    gtk_notebook_append_page(GTK_NOTEBOOK (notebook), sysMenu, sysLabel);

    // 第三页第一部分：cpu
    GtkWidget *sysFrame_cpu = gtk_frame_new("cpu信息");
    // gtk_box_pack_start(GTK_BOX(sysMenu), sysFrame_cpu, TRUE,TRUE,0);
    gtk_container_add (GTK_CONTAINER (sysMenu), sysFrame_cpu);
    
    // 构建cpu的框架
    GtkWidget *sysFCInfo = gtk_vbox_new(TRUE, 0);
    gtk_container_add (GTK_CONTAINER (sysFrame_cpu), sysFCInfo);

    // 添加cpu内容
    GtkWidget *CInfoLabel = gtk_label_new("CPU");
    gtk_container_add (GTK_CONTAINER (sysFCInfo), CInfoLabel);
    setCpuInfo(CInfoLabel);

    // 第三页第二部分：os
    GtkWidget *sysFrame_os = gtk_frame_new("系统信息");
    gtk_container_add (GTK_CONTAINER (sysMenu), sysFrame_os);
    
    // 构建os的框架
    GtkWidget *sysFOInfo = gtk_vbox_new(TRUE, 0);
    gtk_container_add (GTK_CONTAINER (sysFrame_os), sysFOInfo);

    // 添加os内容
    GtkWidget *hostnameLabel = gtk_label_new(" ");
    gtk_container_add (GTK_CONTAINER (sysFOInfo), hostnameLabel);
    updateHostName(hostnameLabel);

    GtkWidget *startTimeLabel = gtk_label_new(" ");
    gtk_container_add (GTK_CONTAINER (sysFOInfo), startTimeLabel);
    setStartTime(startTimeLabel);

    GtkWidget *uptimeLabel = gtk_label_new(" ");
    gtk_container_add (GTK_CONTAINER (sysFOInfo), uptimeLabel);
    //updateUptime(uptimeLabel);
    g_timeout_add(1000,updateUptime, uptimeLabel);

    GtkWidget *KInfoLabel = gtk_label_new(" ");
    gtk_container_add (GTK_CONTAINER (sysFOInfo), KInfoLabel);
    setKernelInfo(KInfoLabel);

    // -----第一页按键菜单----------------
    GtkWidget *sysbuttonMenu = gtk_hbox_new(FALSE, 10);
    gtk_box_pack_start(GTK_BOX (sysMenu), sysbuttonMenu, FALSE, FALSE,0);
    gtk_container_set_border_width (GTK_CONTAINER (sysbuttonMenu), 5);

    GtkWidget *rebootButton = gtk_button_new_with_label ("重新启动");
    g_signal_connect(G_OBJECT (rebootButton), "clicked", G_CALLBACK(rebootsys), NULL);
    gtk_widget_set_size_request(rebootButton, 70, 30);
    gtk_box_pack_start(GTK_BOX (sysbuttonMenu), rebootButton, TRUE, FALSE,0);

    GtkWidget *shutdownButton = gtk_button_new_with_label ("关机");
    g_signal_connect(G_OBJECT (shutdownButton), "clicked", G_CALLBACK(shutdownsys), NULL);
    gtk_widget_set_size_request(shutdownButton, 70, 30);
    gtk_box_pack_start(GTK_BOX (sysbuttonMenu), shutdownButton, TRUE, FALSE,0);

    // 对下窗口建立按键目录
    GtkWidget *stateMenu = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX (menu), stateMenu, FALSE, FALSE,0);
    
    GtkWidget *stateCLabel = gtk_label_new("Cpu");
    setText2Label(stateCLabel, CRInfoText);
    gtk_box_pack_start(GTK_BOX (stateMenu), stateCLabel, TRUE, FALSE,0);

    GtkWidget *stateMLabel = gtk_label_new("Mem");
    setText2Label(stateMLabel, MRInfoText);
    gtk_box_pack_start(GTK_BOX (stateMenu), stateMLabel, TRUE, FALSE,0);

    updateTimeInfo();
    g_timeout_add(1000,updateTimeInfo, NULL);
    GtkWidget *stateTLabel = gtk_label_new("Time");
    setText2Label(stateTLabel, TInfoText);
    gtk_box_pack_start(GTK_BOX (stateMenu), stateTLabel, TRUE, FALSE,0);

    //button2 = gtk_button_new_with_label ("button2");
    //gtk_container_add (GTK_CONTAINER (menu), button2);

    gtk_widget_show_all(window);
    gtk_main();

    return 0;
}


int setCpuInfo(GtkWidget *CInfoLabel){
    char text[600], temtext[500];
    FILE *fp = fopen("/proc/cpuinfo", "r");
    int temop = 0;
    do {
        temop = fgets(temtext,11, fp);
        // printf("%s || ", temtext);
    }while(strcmp(temtext, "model name") != 0 && temop != 0);
    
    fscanf(fp, "%s", temtext);
    fgets(temtext,500, fp);
    sprintf(text,"CPU:  %s", temtext);

    // fscanf(fp, "%s", temtext);  
    // sprintf(text,"%s Hz:%s", text, temtext);
    
    printf("%s\n", text);
    gtk_label_set_text(GTK_LABEL (CInfoLabel), text);
    
    fclose(fp);
    return 0;
}

int setKernelInfo(GtkWidget *KInfoLabel){
    char text[600], temtext[500];
    FILE *fp = fopen("/proc/sys/kernel/ostype", "r");
    fscanf(fp, "%s", temtext);
    fclose(fp);
    sprintf(text,"Kernel:  %s", temtext);

    fp = fopen("/proc/sys/kernel/osrelease", "r");
    fscanf(fp, "%s", temtext);
    fclose(fp);
    sprintf(text,"%s %s", text, temtext);
    
    printf("%s\n", text);
    gtk_label_set_text(GTK_LABEL (KInfoLabel), text);
    
    return 0;
}

int setStartTime(GtkWidget *startTimeLabel){
    char text[600];
    FILE *fp = fopen("/proc/uptime", "r");
    struct tm *info;
    time_t nowtime, temtime;
    time(&nowtime);
    fscanf(fp, "%ld", &temtime);
    fscanf(fp, "%ld", &temtime);
    nowtime -= temtime;
    info = localtime(&nowtime);
    sprintf(text,"启动时间:  %d年%d月%d日 %2d:%2d:%2d", 1900+info->tm_year, 1+info->tm_mon, info->tm_mday, 1+info->tm_hour, info->tm_min, info->tm_sec);
    printf("%s\n", text);
    gtk_label_set_text(GTK_LABEL (startTimeLabel), text);
    fclose(fp);
    return 0;
}

int updateHostName(GtkWidget *hostLabel)
{
    char name[600], temname[500];
    FILE *fp = fopen("/proc/sys/kernel/hostname", "r");
    fscanf(fp, "%s", temname);
    printf("host name :%s \n",temname);
    sprintf(name, "hostname:  %s", temname);
    gtk_label_set_text(GTK_LABEL (hostLabel), name);
    fclose(fp);
    return 0;
}

int setPidInfo(GtkListStore *PidStore) {
    DIR *dp;
    if((dp = opendir("/proc")) == NULL)
    {
        printf("Can't open /proc\n");
        return 0;
    }

    struct dirent *dirp;
    char subdir[255], temInfo[26][255], temtext[100];
    int pid, ppid, pro;
    FILE *fp;
    GtkTreeIter iter;
    char *nowstate;
    char state[7][20] = {"running", "idle", "sleeping", "waiting", "stoped", "zoombie", "dead"};
    long int mem;
    while ((dirp = readdir(dp)) != NULL) {
        if (dirp->d_name[0] > '0' && dirp->d_name[0] <= '9') {
            sprintf(subdir, "/proc/%s/stat", dirp->d_name);
            fp = fopen(subdir, "r");
            if (fp == NULL){
                printf("error");
            }
            fscanf(fp, "%d (%s", &pid, temInfo[1]);
            temInfo[1][strlen(temInfo[1]) - 1] = '\0';
            for (int i = 2; i < 26; i++) {
                if (i == 22) {
                    fscanf(fp, "%ld", &mem);
                    continue;
                }
                if (i == 3) {
                    fscanf(fp, "%d", &ppid);
                    continue;
                }
                if (i == 17) {
                    fscanf(fp, "%d", &pro);
                    continue;
                }
                fscanf(fp, "%s", temInfo[i]);
            }
            fclose(fp);
            
            sprintf(temtext, "%ld MB", mem/ 1024 /1024);
            switch(temInfo[2][0]){
                case 'R': nowstate = state[0]; break;
                case 'I': nowstate = state[1]; break;
                case 'S': nowstate = state[2]; break;
                case 'D': nowstate = state[3]; break;
                case 'T': nowstate = state[4]; break;
                case 'Z': nowstate = state[5]; break;
                case 'X': nowstate = state[6];
            }

            gtk_list_store_append(PidStore, &iter);
            gtk_list_store_set(PidStore, &iter, 
                                NAME_COL,temInfo[1],
                                MEMORY_COL, temtext,
                                PID_COL, pid,
                                PPID_COL, ppid,
                                STATUS_COL, nowstate,
                                PRO_COL, pro, -1);
            // printf("\n$$%s %s$$\n",temInfo[2], temInfo[4]);
        }
        
    }
    closedir(dp);
    return 0;
}

int updatePidInfo(gpointer data){
    gtk_list_store_clear(listStore);
    setPidInfo(listStore);
}

int deletePid(gpointer data){
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter;
    pid_t pidNum;
    int res = -1;

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(listTree));
    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        gtk_tree_model_get(model, &iter, PID_COL, &pidNum, -1);
        res = kill(pidNum, SIGTERM);
        // printf("%d\n",pidNum);
        if (res != 0){
            printf(" kill error\n");
        }
    }

    return res;

}

int openEntry_callback(GtkWidget *widget, gpointer entry){
    const gchar *entry_text;
    gchar pathtext[100] = {'\0'}, argtext[100] = {'\0'};

    //获取文本内容
    entry_text = gtk_entry_get_text(GTK_ENTRY(entry));
    int i = 0;
    for (i = 0; i < strlen(entry_text); i++){
        if (entry_text[i] == ' '){
            break;
        }
        pathtext[i] = entry_text[i];
    }
    pathtext[i] = '\0';
    strcpy(argtext, entry_text+i);
    printf("path:%s, argv:%s |\n", pathtext, argtext);

    signal(SIGCHLD,sig_child);//prevent defunct
    pid_t pid=fork();
 
    // 当pid==0时，是子进程代码运行区域。其他则是父进程运行区域。
    if(pid == 0){
        // system(entry_text);
        char exepath[100];
        sprintf(exepath, "gnome-terminal -- %s", pathtext);
        system(exepath);
        // execv(pathtext, NULL);
        printf("Error: %s\n", strerror(errno));
        // showError(window);
        exit(0);
    }
    else if (pid < 0){
        printf("create pid fail!\n");
    }

    return 0;
}

int showPopWin(GtkWidget *widget, GdkEvent *event){
	const gint RIGHT_CLICK = 3;
    GtkTreePath *path;
    GtkTreePath *cousor_path;
    GtkTreeViewColumn *col;
    gint x,y;
    gint cell_x, cell_y;

    GdkEventButton *bevent = (GdkEventButton *) event;
    x = (gint)bevent->x;
    y = (gint)bevent->y;

    gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(listTree), x, y, &path, &col, &cell_x, &cell_y);
    gtk_widget_grab_focus(listTree);
    gtk_tree_view_set_cursor(GTK_TREE_VIEW(listTree), path, NULL, FALSE);
    gtk_tree_path_free(path);
    
	if (event->type == GDK_BUTTON_PRESS)
	{
		if (bevent->button == RIGHT_CLICK)
		{
			gtk_menu_popup(GTK_MENU(widget), NULL, NULL, NULL, NULL, bevent->button, bevent->time);
		}
		return TRUE;
	}
	return FALSE;
}

gboolean updateUptime(GtkWidget *uptimeLabel)
{   
    char text[600];
    time_t temtime;
    struct tm info;
    FILE *fp = fopen("/proc/uptime", "r");
    fscanf(fp, "%ld", &temtime);
    fscanf(fp, "%ld", &temtime);
    int day = temtime / (60*60*24) + 0.5, hour = temtime / (60*60) % 24, min = temtime / 60 % 60, sec = temtime % 60;
    sprintf(text,"已运行:  %d天%2d时%02d分%02d秒", day, hour, min, sec);
    // printf("%s\n", text);
    gtk_label_set_text(GTK_LABEL (uptimeLabel), text);
    fclose(fp);
    return TRUE;
}

gboolean updateCpuRateInfo(char text[]) {
    char temtext[100];
    gdouble tem[50];
    gdouble nowfree, nowtotal, durfree, durtotal, rate_cpu; 
    FILE *fp = fopen("/proc/stat", "r");
    
    fscanf(fp, "%s", temtext);

    for (int i = 1; i < 10; i++) {
        fscanf(fp, "%lf", &tem[i]);
        continue;
    }
    fclose(fp);

    nowfree = tem[4];
    nowtotal = tem[1] + tem[2] + tem[3] + tem[4];
    durfree = nowfree - prefree;
    durtotal = nowtotal - pretotal;
    rate_cpu = 1 - durfree / durtotal;
    // printf("%.2f %f, %f \n", rate_cpu, durfree, durtotal);

    prefree = nowfree;
    pretotal = nowtotal;
    for (int i = 0; i < 120; i++) {
        if (i == 119) {
            point_cpu[i] = rate_cpu;
        }
        else {
            point_cpu[i] = point_cpu[i+1];
        }
    }

    sprintf(text, "CPU 利用率： %-2.2f%%", rate_cpu*100);

    // fscanf(fp, "%s", temtext);  
    // sprintf(text,"%s Hz:%s", text, temtext);
    
    // printf("%s\n", text);
    
    return TRUE;
}

gboolean updateLabelInfo(SL *sl) {
    gtk_label_set_text(GTK_LABEL (sl->Label), sl->text);
    return TRUE;
}

int setText2Label(GtkWidget *Label, char text[]) {
    SL *sl = (SL*)malloc(sizeof(sl));
    sl->Label = Label;
    sl->text = text;
    g_timeout_add(1000, updateLabelInfo, sl);
    return 0;
}

gboolean updateMemRateInfo() {
    char temtext[100];
    int temop;
    gdouble memfree = -1, memtotal = -1, swapfree = -1, swaptotal = -1, rate_mem = 0, rate_swap = 0; 
    FILE *fp = fopen("/proc/meminfo", "r");
    
    do {
        temop = fgets(temtext,15, fp);
        // printf("%s || ", temtext);
        if (strstr(temtext, "MemTotal:") != NULL){
            fscanf(fp, "%lf", &memtotal);
        }
        else if (strstr(temtext, "MemFree:")){
            fscanf(fp, "%lf", &memfree);
        }
        else if (strstr(temtext, "SwapTotal:")){
            fscanf(fp, "%lf", &swaptotal);
        }
        else if (strstr(temtext, "SwapFree:")){
            fscanf(fp, "%lf", &swapfree);
        }
        if (memfree >=0 && memtotal >=0 && swapfree >=0 && swaptotal >=0) {
            break;
        }
        
    }while(temop != 0);

    fclose(fp);
    
    rate_mem = 1 - memfree / memtotal;
    rate_swap = 1 - swapfree / swaptotal;
    for (int i = 0; i < 120; i++) {
        if (i == 119) {
            point_mem[i] = rate_mem;
            point_swap[i] = rate_swap;
        }
        else {
            point_mem[i] = point_mem[i+1];
            point_swap[i] = point_swap[i+1];
        }
    }
    sprintf(MRInfoText, "内存使用： %-2.2fGB/%-2.2fGB", (memtotal-memfree)/1024/1024, memtotal/1024/1024);
    sprintf(SRInfoText, "Swap使用： %-2.2fGB/%-2.2fGB", (swaptotal-swapfree)/1024/1024, swaptotal/1024/1024);
    
    // printf("%s\n", text);
    
    return TRUE;
}

gboolean updateTimeInfo() {
    time_t nowtime = time(NULL);
    struct tm *nowtm = gmtime(&nowtime);
    char *tem = asctime(nowtm);
    tem[strlen(tem) - 1] = 0;
    sprintf(TInfoText, "%s",tem);
    return TRUE;
}

gboolean drawRateGraph_cpu(GtkWidget *widget, cairo_t *cr, gpointer data)
{
    guint width, height;
    gdouble perwidth;
    GdkRGBA color;

    width = gtk_widget_get_allocated_width (widget);
    height = gtk_widget_get_allocated_height (widget);
    perwidth = width / 119.0;
    cairo_rectangle (cr, 0, 0,width, height);
    cairo_set_line_width(cr, 1);
    gdk_rgba_parse(&color, "#ADD8E6");
    //gtk_style_context_get_color (gtk_widget_get_style_context (widget),0,&color);
    gdk_cairo_set_source_rgba (cr, &color);
    cairo_stroke (cr);

    
    cairo_move_to(cr, 0, height);
    for (int i = 0; i < 120; i++) {
        cairo_line_to(cr, perwidth*i, height*(1-point_cpu[i]));
    }
    cairo_line_to(cr, perwidth*119, height);
    // cairo_line_to(cr, 0, height);
    cairo_close_path (cr);
    cairo_fill_preserve(cr);

    cairo_pattern_t *newmask = cairo_pattern_create_rgba(0.5,0.5,0.5,0.05);
    cairo_mask(cr, newmask);

    cairo_set_line_width(cr, 1);
    gdk_rgba_parse(&color, "#00BFFF");
    gdk_cairo_set_source_rgba (cr, &color);
    cairo_stroke (cr);

    return FALSE;
}

gboolean drawRateGraph_mem(GtkWidget *widget, cairo_t *cr, gpointer data)
{
    guint width, height;
    gdouble perwidth;
    GdkRGBA color;

    width = gtk_widget_get_allocated_width (widget);
    height = gtk_widget_get_allocated_height (widget);
    perwidth = width / 119.0;
    cairo_rectangle (cr, 0, 0,width, height);
    cairo_set_line_width(cr, 1);
    gdk_rgba_parse(&color, "#ADD8E6");

    //gtk_style_context_get_color (gtk_widget_get_style_context (widget),0,&color);
    gdk_cairo_set_source_rgba (cr, &color);
    cairo_stroke (cr);

    cairo_set_line_width(cr, 2);
    cairo_move_to(cr, 0, height);
    for (int i = 0; i < 120; i++) {
        cairo_line_to(cr, perwidth*i, height*(1-point_mem[i]));
    }
    cairo_line_to(cr, perwidth*119, height);
    // cairo_line_to(cr, 0, height);
    cairo_close_path (cr);
    cairo_fill_preserve(cr);

    cairo_pattern_t *newmask = cairo_pattern_create_rgba(0.5,0.5,0.5,0.05);
    cairo_mask(cr, newmask);

    cairo_set_line_width(cr, 1);
    gdk_rgba_parse(&color, "#00BFFF");
    gdk_cairo_set_source_rgba (cr, &color);
    cairo_stroke (cr);

    return FALSE;
}

gboolean drawRateGraph_swap(GtkWidget *widget, cairo_t *cr, gpointer data)
{
    guint width, height;
    gdouble perwidth;
    GdkRGBA color;

    width = gtk_widget_get_allocated_width (widget);
    height = gtk_widget_get_allocated_height (widget);
    perwidth = width / 119.0;
    cairo_rectangle (cr, 0, 0,width, height);
    cairo_set_line_width(cr, 1);
    gdk_rgba_parse(&color, "#ADD8E6");

    //gtk_style_context_get_color (gtk_widget_get_style_context (widget),0,&color);
    gdk_cairo_set_source_rgba (cr, &color);
    cairo_stroke (cr);

    cairo_set_line_width(cr, 2);
    cairo_move_to(cr, 0, height);
    for (int i = 0; i < 120; i++) {
        cairo_line_to(cr, perwidth*i, height*(1-point_swap[i])-1);
    }
    cairo_line_to(cr, perwidth*119, height);
    // cairo_line_to(cr, 0, height);
    cairo_close_path (cr);
    cairo_fill_preserve(cr);

    cairo_pattern_t *newmask = cairo_pattern_create_rgba(0.5,0.5,0.5,0.05);
    cairo_mask(cr, newmask);

    cairo_set_line_width(cr, 1);
    gdk_rgba_parse(&color, "#00BFFF");
    gdk_cairo_set_source_rgba (cr, &color);
    cairo_stroke (cr);

    return FALSE;
}

void showError(GtkWidget *window) {
  GtkWidget *dialog;
  dialog = gtk_message_dialog_new(window,
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_OK,
            strerror(errno));
  gtk_window_set_title(GTK_WINDOW(dialog), "Run Error");
  gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);
}

int clickSort(GtkTreeViewColumn *tree_column){
    char *title = gtk_tree_view_column_get_title(tree_column);
    // printf("%s, %s", title, proInfoList[0]);
    int i = 0;
    for (i = 0; i < 6; i++) {
        if (strcmp (proInfoList[i], title) == 0) {
            // strcpy(sortTitle, proInfoList[i]);
            char text[600];
            sprintf(text, "%s:", proInfoList[i]);
            gtk_label_set_text(GTK_LABEL (searchTLabel), text);
            break;
        }
    }
    gtk_tree_view_set_search_column(listTree, i);
    // gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (listSortModel), i, sortsate[i]);
    // sortsate[i]  = !sortsate[i];
    return 0;
}

void sig_child(){
    pid_t   pid; 
    int       status; 
    while((pid   =   waitpid(-1,   &status,   WNOHANG))   >   0)   {} 
    //Key !!!!!!!!  wait or waitpid
    return; 
}
    
int rebootsys() {
    system("shutdown -r 0");
    return 0;
}

int shutdownsys() {
    system("shutdown -h 0");
    return 0;
}
