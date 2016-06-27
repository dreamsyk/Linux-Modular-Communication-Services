#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define MAXDATASIZE 100

int main(int argc,char **argv){

	int sockfd,num;
	char buf[MAXDATASIZE];
	char ser_addr[MAXDATASIZE];
	int ser_port;
	struct sockaddr_in server;

	if(argc == 3){
		strcpy(ser_addr,argv[1]);
		ser_port = atoi(argv[2]);
	} else {
		printf("IP:");
		scanf("%s",ser_addr);
		printf("PORT:");
		scanf("%d",&ser_port);
	}

	printf("Server address:%s , port %d \n ",ser_addr,ser_port);

	if((sockfd = socket(AF_INET,SOCK_STREAM,0))==-1){
		printf("socket() error \n");
		exit(1);
	}

	bzero(&server,sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(ser_port);	
	server.sin_addr.s_addr = inet_addr(ser_addr);

	if(connect(sockfd , (struct sockaddr *)&server,sizeof(server))==-1){
		printf("connect() error\n");
		exit(1);
	}

	//接收欢迎信息
	if((num=recv(sockfd,buf,MAXDATASIZE,0))==-1){
			printf("recv() error\n");
			return -1;
		}

		if(num == 0){
			printf("Server Disconnected ...\n");
			return -1;
		}

	buf[num] = '\0';
	printf("%s\n",buf);
	
	//输入用户昵称，并发送
	{
		char clientNameBuf[MAXDATASIZE];
		int clientNameLength;
		printf("Client Name:");
		scanf("%s",clientNameBuf);
		clientNameLength = strlen(clientNameBuf);
		send(sockfd,clientNameBuf,clientNameLength,MSG_DONTWAIT);
	}

	//客户端模式选择
	{
		putchar('\n');
		
		char modeSTRBuf[MAXDATASIZE];
		int modeSTRLength;
		int modeInput;
		//接收服务列表并打印
		if((modeSTRLength = recv(sockfd,modeSTRBuf,MAXDATASIZE,0)) == -1){
			printf("Mode select Error");
			exit(-1);
		}
		modeSTRBuf[modeSTRLength] = '\0';
		printf("%s",modeSTRBuf);

		//输入模式索引，并发送		
		scanf("%d",&modeInput);
		sprintf(modeSTRBuf,"%d",modeInput);
		send(sockfd,modeSTRBuf,strlen(modeSTRBuf),0);

		putchar('\n');
	}
	
	//创建子进程，子进程负责发送，父进程负责接收
	pid_t pid;
	if((pid = fork())==0){//子进程：负责输入发送

		while(1){
			
			char inputBuf[MAXDATASIZE];
			int inputLength;
			int i = 0;
			char ch;
			while((ch=getchar())!='\n'){
				inputBuf[i++] = ch;
			}
			inputBuf[i] = '\0';

			inputLength = strlen(inputBuf);

			if(inputLength > 0){
				send(sockfd,inputBuf,inputLength,0);
				if(!strcmp(inputBuf,"bye")){
					exit(1);
					break;
				}
			}
		}
	} else if(pid > 0){//父进程:负责接收信息并输出

		while(1){

			if((num=recv(sockfd,buf,MAXDATASIZE,0))==-1){
				printf("recv() error\n");
				exit(1);
			}

			if(num == 0){
				printf("Server Disconnected ...\n");
				break;
			}

			buf[num] = '\0';
			printf("%s\n",buf);

		}

	} else {
		perror("fork error.\n");
		exit(0);
	}
	
	//关闭套接字
	close(sockfd);
}
