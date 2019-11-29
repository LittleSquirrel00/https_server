#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include<sys/stat.h>
#define IPSTR "127.0.0.1" //httpsever ip;
char *path = "/home/lixuefei/Documents/vscode/http/server";
#define PORT 8000
#define BUFSIZE 1024
int creat_listenfd(void){
    //create tcp connect
    int fd=socket(AF_INET,SOCK_STREAM,0);
    int n=1;
    setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&n,4);
    //bind addr
    struct sockaddr_in sin;
    bzero(&sin,sizeof(sin));//clear
    sin.sin_family=AF_INET;
    sin.sin_port=htons(PORT);
    sin.sin_addr.s_addr=INADDR_ANY;

    int res=bind(fd,(struct sockaddr *)&sin,sizeof(sin));
    if(res==-1){
        perror("bind error");
    }

    listen(fd,100);//Netlisten
    return fd;
}
void not_found_request(int fd)//404
{
    char filename[BUFSIZE]={0};
    strcpy(filename,"not_found_request.html");
    char *mime=NULL;
    if (strstr(filename, ".html"))
		mime="text/html";
    else if (strstr(filename, ".gif"))
        mime="image/gif";
    else if (strstr(filename, ".jpg"))
        mime="image/jpeg";
    else if (strstr(filename, ".png"))
        mime="image/png";
    else
    mime="text/plain";
    //Open the file, read the content, build the response, and send it back to the client
    char response[BUFSIZE*BUFSIZE]={0};
    sprintf(response,"HTTP/1.1 404 NOT_FOUND\r\nContent-Type: %s\r\n\r\n",mime);
    int headlen = strlen(response);

    int filefd=open(filename,O_RDONLY);
    int filelen=read(filefd,response+headlen,sizeof(response)-headlen);
    //Send the response header and content
    write(fd,response,headlen+filelen);
    close(filefd);
}
void bad_respond(int fd)//400
{
    char filename[BUFSIZE]={0};
    strcpy(filename,"bad_respond.html");
    char *mime=NULL;
    if (strstr(filename, ".html"))
		mime="text/html";
    else if (strstr(filename, ".gif"))
        mime="image/gif";
    else if (strstr(filename, ".jpg"))
        mime="image/jpeg";
    else if (strstr(filename, ".png"))
        mime="image/png";
    else
    mime="text/plain";
    //Open the file, read the content, build the response, and send it back to the client
    char response[BUFSIZE*BUFSIZE]={0};
    sprintf(response,"HTTP/1.1 400 BAD_REQUESTION\r\nContent-Type: %s\r\n\r\n",mime);
    int headlen = strlen(response);

    int filefd=open(filename,O_RDONLY);
    int filelen=read(filefd,response+headlen,sizeof(response)-headlen);
    //Send the response header and content
    write(fd,response,headlen+filelen);
    close(filefd);
}
void forbiden_respond(int fd)//403
{
    char filename[BUFSIZE]={0};
    strcpy(filename,"forbiden_respond.html");
    char *mime=NULL;
    if (strstr(filename, ".html"))
		mime="text/html";
    else if (strstr(filename, ".gif"))
        mime="image/gif";
    else if (strstr(filename, ".jpg"))
        mime="image/jpeg";
    else if (strstr(filename, ".png"))
        mime="image/png";
    else
    mime="text/plain";
    //Open the file, read the content, build the response, and send it back to the client
    char response[BUFSIZE*BUFSIZE]={0};
    sprintf(response,"HTTP/1.1 403 FORBIDDEN\r\nContent-Type: %s\r\n\r\n",mime);
    int headlen = strlen(response);

    int filefd=open(filename,O_RDONLY);
    int filelen=read(filefd,response+headlen,sizeof(response)-headlen);
    //Send the response header and content
    write(fd,response,headlen+filelen);
    close(filefd);
}
void dynamic(char *filename,int fd,char *argv)
{
    int len = strlen(argv);
    int k = 0;
    int number[2];
    int sum=0;
    char response[BUFSIZE*BUFSIZE]={0};
    char body[BUFSIZE*BUFSIZE]={0};
    //Get parameter form
    sscanf(argv,"a=%d&b=%d",&number[0],&number[1]);
    //Dynamic processing functions
    if(strcmp(filename,"/add")==0)
    {
        sum = number[0] + number[1];
        sprintf(body,"<html><body>\r\n<p>%d + %d = %d </p><hr>\r\n</body></html>\r\n",number[0],number[1],sum);
        sprintf(response,"HTTP/1.1 200 ok\r\nConnection: close\r\n\r\n");
        int ret=send(fd,response,strlen(response),0);
        int r = send(fd,body,strlen(body),0); 
        printf("%s",body);
    }
    else if(strcmp(filename,"/multiplication")==0)
    {
        printf("%s\n",filename);
        sum = number[0]*number[1];
        sprintf(body,"<html><body>\r\n<p>%d * %d = %d </p><hr>\r\n</body></html>\r\n",number[0],number[1],sum);
        sprintf(response,"HTTP/1.1 200 ok\r\nConnection: close\r\n\r\n");
        int ret=send(fd,response,strlen(response),0);
        int r = send(fd,body,strlen(body),0); 
        
    }
}

void successful_request(char *filename,int fd)//200
{
    char *mime=NULL;
    if (strstr(filename, ".html"))
        mime="text/html";
    else if (strstr(filename, ".gif"))
        mime="image/gif";
    else if (strstr(filename, ".jpg"))
        mime="image/jpeg";
    else if (strstr(filename, ".png"))
        mime="image/png";
    else
    mime="text/plain";
    //Open the file, read the content, build the response, and send it back to the client
    char response[BUFSIZE*BUFSIZE]={0};
    sprintf(response,"HTTP/1.1 200 ok\r\nContent-Type: %s\r\n\r\n",mime);
    int headlen = strlen(response);

    int filefd=open(filename,O_RDONLY);
    int filelen=read(filefd,response+headlen,sizeof(response)-headlen);

    //Send the response header and content
    write(fd,response,headlen+filelen);
    printf("hello\n");
    close(filefd);
}
void response_get(char *filename,int fd){
    

    char *argv;
    char * ch;
    struct stat m_file_stat;
    char function[100];
    char file[100];
	strcpy(file, path);
	strcat(file, filename);

    if(ch=strchr(file,'?'))
    {
        argv = ch+1;
        *ch = '\0';
        strncpy(function, filename, strlen(filename)-strlen(argv)-1);
        dynamic(function,fd,argv); 
    }
    //404
    else if(stat(file, &m_file_stat) < 0)
    {
        not_found_request(fd);   
    }
    //403
    else if( !(m_file_stat.st_mode & S_IROTH))
    {
        forbiden_respond(fd);
    }
    //400
    else if(S_ISDIR(m_file_stat.st_mode))
    {
        bad_respond(fd);
    }else{ //success
        successful_request(file,fd);
    }

}
void response_post(char *filename,int fd,char *argv){

    struct stat m_file_stat;
    char function[100];
    char file[100];
	strcpy(file, path);
	strcat(file, filename);
    if(strcmp(argv,"\0")!=0){
        dynamic(filename,fd,argv); 
    }
    //404
    else if(stat(file, &m_file_stat) < 0)
    {
        not_found_request(fd);   
    }
    //403
    else if( !(m_file_stat.st_mode & S_IROTH))
    {
        forbiden_respond(fd);
    }
    //400
    else if(S_ISDIR(m_file_stat.st_mode))
    {
        bad_respond(fd);
    }else{ //
        successful_request(file,fd);
    }

}
void handle_request(int fd){
    char buffer[BUFSIZE*BUFSIZE]={0};
    int nread=read(fd,buffer,sizeof(buffer));
    printf("%s",buffer);
    //Parses the filename from the request
    char method[5];
    char filename[50];
    int i, j;
    i = j = 0;
    while(buffer[j] != ' ' && buffer[j] != '\0')//获取请求方法
    {
        method[i++] = buffer[j++];
    }
    ++j;
    method[i] = '\0';
    i = 0;
    while(buffer[j] != ' ' && buffer[j] != '\0')//获取请求文件
    {
        filename[i++] = buffer[j++];
    }
    filename[i] = '\0';
    //Determine the status of the request
    struct stat m_file_stat;
    char function[100];
    char file[100];
	strcpy(file, path);
	strcat(file, filename);
    //The connection methods
    char connection[100];
    memset(connection, 0, sizeof(connection));
    int k = 0;
    char *ch = NULL;
    ++j;
    while((ch = strstr(connection, "Connection")) == NULL) //查找请求头部中的Content-Length行
    {
        k = 0;
        memset(connection, 0, sizeof(connection));
        while(buffer[j] != '\r' && buffer[j] != '\0')
        {
            connection[k++] = buffer[j++];
        }
        ++j;
    }
    char type[100]={0};
    char temp[100]={0};
    sscanf(connection,"%s %s",type,temp);
    printf("%s\n",temp);
    if(strcasecmp(method, "GET") == 0)  //get method
    {
        response_get(filename,fd);
    }
    else if(strcasecmp(method, "POST") == 0)  //post method
    {
        //printf("Begin\n");
        char argvs[100];
        memset(argvs, 0, sizeof(argvs));
        int k = 0;
        char *ch = NULL;
        ++j;
        while((ch = strstr(argvs, "Content-Length")) == NULL) //查找请求头部中的Content-Length行
        {
            k = 0;
            memset(argvs, 0, sizeof(argvs));
            while(buffer[j] != '\r' && buffer[j] != '\0')
            {
                argvs[k++] = buffer[j++];
            }
            ++j;
            //printf("%s\n", argvs);
        }
        // printf("%s\n", argvs);
        int length;
        char *str = strchr(argvs, ':');  //获取POST请求数据的长度
        ++str;
        sscanf(str, "%d", &length);
        j = strlen(buffer) - length;    //从请求报文的尾部获取请求数据
        k = 0;
        memset(argvs, 0, sizeof(argvs));
        while(buffer[j] != '\r' && buffer[j] != '\0')
            argvs[k++] = buffer[j++];

        argvs[k] = '\0';
        // printf("%s\n", argvs);
        response_post(filename,fd,argvs);  //POST方法
    }
    
    //Gets the file type based on the filename
   
}

int main(void){
    //Creates a listening socket and returns a socket descriptors
    int sockfd=creat_listenfd();
    while(1){
        //Accept client requests for connection
        int fd=accept(sockfd,NULL,NULL);
        printf("There is a client connection.\n");
        //Handle client requests
        handle_request(fd);
        close(fd);
    }
    close(sockfd);
}
