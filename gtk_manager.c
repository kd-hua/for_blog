#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h> // for open
#include <unistd.h> // for close
#include <time.h>
#include <dirent.h> // opendir
#include <string.h>

void printmode(int st_mode)
{
    if (st_mode & S_IRUSR)
    {
        printf("r");
    }
    else
    {
        printf("-");
    }

    if (st_mode & S_IWUSR)
    {
        printf("w");
    }
    else
    {
        printf("-");
    }         

    if (st_mode & S_IXUSR)
    {
        printf("x");
    }
    else
    {
        printf("-");
    }

    if (st_mode & S_IRGRP)
    {
        printf("r");
    }
    else
    {
        printf("-");
    }

    if (st_mode & S_IWGRP)
    {
        printf("w");
    }
    else
    {
        printf("-");
    }         

    if (st_mode & S_IXGRP)
    {
        printf("x");
    }
    else
    {
        printf("-");
    }
    
    if (st_mode & S_IROTH)
    {
        printf("r");
    }
    else
    {
        printf("-");
    }

    if (st_mode & S_IWOTH)
    {
        printf("w");
    }
    else
    {
        printf("-");
    }         

    if (st_mode & S_IXOTH)
    {
        printf("x");
    }
    else
    {
        printf("-");
    }
    printf(" "); 
}

void printdir(char *dir, int depth){
    DIR *dp;
    struct dirent *dirp;
    char subfile[1024] = { 0 };
    char subdir[100][1024] = { 0 };
    int dirNum = 0;
    if((dp = opendir(dir)) == NULL)
    {
        printf("Can't open %s\n", dir);
        return ;
    }
    printf("\n%s:\n",dir);
    struct stat buf;  // 存文件信息
    struct tm *temtm; // 时间内容的结构
    int fd;
    while((dirp = readdir(dp)) != NULL)
    {
        if (strcmp(dirp->d_name, "..") == 0 || strcmp(dirp->d_name, ".") == 0)
            continue;
        if (dir[strlen(dir) - 1] == '/')
            dir[strlen(dir) - 1] = 0;
        sprintf(subfile, "%s/%s", dir, dirp->d_name);
        // 获取文件修饰符
        // printf("*%s*", subfile);
        fd = open(subfile,O_RDONLY);
        // 获取文件信息
        fstat(fd, &buf);
        // 打印文件权限与类型    
        if (buf.st_mode & S_IFDIR)
        {
            printf("d");
            sprintf(subdir[dirNum], "%s/%s", dir, dirp->d_name);
            dirNum ++;
        }
        else // if (buf.st_mode & S_IFREG)
        {
            printf("-");
        }
        printmode(buf.st_mode);
        
        printf("%2ld ", buf.st_nlink);
        printf("%5ld ", buf.st_size);
        temtm = localtime(&(buf.st_atime));
        printf("%2d月 %2d %2d:%02d ", temtm->tm_mon + 1, temtm->tm_mday, temtm->tm_hour, temtm->tm_min);
        
        printf("%s\n", dirp->d_name);
        close(fd);
        // printf("%s/%s\n", dir, dirp->d_name);
    }
    closedir(dp);
    for (int i = 0; i < dirNum; i++)
    {

        printdir(subdir[i], depth + 1);
    }
    
}

int main(int argc, char* argv[]){
    if (argc != 2)
        printf("参数错误\n");
    char path[] = "./";
    printdir(argv[1], 0);
    return 0;
}
