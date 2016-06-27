#define _GNU_SOURCE
#include "service.h"
#include <sched.h>

#define PORT 1234
#define BACKLOG 2
#define MAXDATASIZE MAX_SIZE

#define STACK_SIZE 1024*8 //8K

int process_client();

int connfd_temp;
struct sockaddr_in client_temp;

int main(int argc,char **argv){

	int listenfd,connectfd;
	struct sockaddr_in server,client;
	int opt = SO_REUSEADDR;
	socklen_t addr_len;
	pid_t pid;

	if((listenfd = socket(AF_INET,SOCK_STREAM,0)) == -1){
		perror("Create socket failed.");
		exit(-1);
	}

	setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));

	bzero(&server,sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(PORT);
	server.sin_addr.s_addr = htonl(INADDR_ANY);

	if(bind(listenfd,(struct sockaddr*)&server,sizeof(server)) == -1){
		perror("Bind error()");
		exit(-1);
	}

	if(listen(listenfd,BACKLOG) == -1){
		perror("listen error.");
		exit(-1);
	}

	addr_len = sizeof(client);

	while(1){

		if((connectfd = accept(listenfd,(struct sockaddr*)&client,&addr_len)) == -1){
			perror("Accept error.");
			exit(-1);
		}

	    bzero(&client_temp,sizeof(client_temp));
	    connfd_temp = connectfd;
	    client_temp.sin_family = client.sin_family;
	    client_temp.sin_port = client.sin_port;
	    client_temp.sin_addr.s_addr = client.sin_addr.s_addr;

	    void ** stack;
	    stack = (void **)malloc(STACK_SIZE);//为子进程申请系统堆栈
	    if(!stack) {
	            printf("Create stack failed\n");
	            exit(0);
	    }
	    
		//创建子进程
    	clone(&process_client, (char *)stack + STACK_SIZE, CLONE_VM|CLONE_FILES|CLONE_SIGHAND, 0);

	}

	return 0;
}

int process_client(){

	int connectfd;
	struct sockaddr_in client;

	char client_name[MAXDATASIZE];
	int recvlen;
	int currentMode = MODE_REV;

	//参数初始化
	connectfd = connfd_temp;
	bzero(&client,sizeof(client));
    client.sin_family = client_temp.sin_family;
    client.sin_family = client_temp.sin_port;
    client.sin_family = client_temp.sin_addr.s_addr;

    //打印客户端连接信息
	printf("Get a connection from %s:%d\n",inet_ntoa(client.sin_addr),ntohs(client.sin_port));

	//发送欢迎信息
	send(connectfd,"Welcome to SYK server.\n",23,0);

	//接收客户端昵称
	recvlen = recv(connectfd,client_name,MAXDATASIZE,0);
	if(recvlen == 0){
		close(connectfd);
		printf("Client disconnected.\n");
		return;
	} else if(recvlen < 0){
		close(connectfd);
		printf("Client connect broked.\n");
		return;
	}
	
	client_name[recvlen] = '\0';
	printf("Client name is %s\n",client_name);

	//用户模式选择
	{
		char modeTemp[MAXDATASIZE] ="\0";
		int tempLen = 0;
		int mode = MODE_ERROR;

		//发送模式选择提示字符串
		char modeSTR[MAXDATASIZE] ="Mode Select \n1.Rev Echo Service\n2.Muti Chat Service\n MODE INDEX =";
		send(connectfd,modeSTR,strlen(modeSTR),0);

		//接收用户模式索引
		tempLen = recv(connectfd,modeTemp,MAXDATASIZE,0);
		modeTemp[tempLen] = '\0';

		printf("Client <%s> MODE select : %s , length:%d\n",client_name,modeTemp,tempLen);

		if(tempLen == 0){
			close(connectfd);
			printf("Client disconnected.\n");
			return;
		} else if(tempLen < 0){
			close(connectfd);
			printf("Client connect broked.\n");
			return;
		}

		switch(atoi(modeTemp)){
			case 1:{//Rev Echo Service
				mode = MODE_REV;
			}break;
			case 2:{//Muti Chat Service
				mode = MODE_MUTI;
			}break;
		}

		currentMode = mode;
		printf("Client <%s> CurrentMode:%d\n",client_name,currentMode);
	} 

	//当前用户创建用户节点
	NetChatClient * client_node = createClientNodeParam(connectfd,client_name,client,currentMode);

	//更新节点缓存
	saveNodeData(client_node);

	//根据索引自动载入相关服务模块
	switch(currentMode){
		case MODE_REV:{//字符串逆置服务
			printf("MODE_REV ....\n");
			revService(client_node);
		}break;
		case MODE_MUTI:{//群聊服务
			printf("MODE_MUTI ....\n");
			mutiChatService(client_node);
		}break;
		case MODE_ERROR:{//模式选择错误
			printf("MODE_ERROR ....\n");
		}break;
	}

	//销毁当前用户节点缓存
	printf("Destory Client <%s> node cache...\n",client_name);
	destoryClientCache(client_node);

	//关闭客户端连接
	printf("Close Client <%s> connection ... \n",client_name);
	close(connectfd);

	return 0;
}

