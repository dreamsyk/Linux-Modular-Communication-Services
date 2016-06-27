#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include "cache.h"

char *rev(char *s)  {

	/* h指向s的头部 */
	char* h = s;
	char* t = s;
	char ch;

	/* t指向s的尾部 */
	while(*t++){};
	t--;    /* 与t++抵消 */
	t--;    /* 回跳过结束符'\0' */

	/* 当h和t未重合时，交换它们所指向的字符 */
	while(h < t)
	{
	    ch = *h;
	    *h++ = *t;    /* h向尾部移动 */
	    *t-- = ch;    /* t向头部移动 */
	}

	return (s);
}

//逆回文服务的具体实现
void revService(NetChatClient *client){

	char recvbuf[MAX_SIZE];
	char sendbuf[MAX_SIZE];
	int recvlen;

	//接收用户发送过来的信息
	while((recvlen = recv(client->connfd,recvbuf,MAX_SIZE,0)) > 0){

		recvbuf[recvlen] = '\0';
		printf("Receive from client <%s> message :%s, Rev Now ... \n",client->name,recvbuf);

		//逆置字符串
		strcpy(sendbuf,recvbuf);
		rev(sendbuf);

		//发送逆置字符串
		send(client->connfd,sendbuf,strlen(sendbuf),0);

	}

}

//群聊服务具体实现
void mutiChatService(NetChatClient *client){

	char recvbuf[MAX_SIZE];
	char sendbuf[MAX_SIZE];
	int recvlen;

	//接收用户发送过来的信息
	while((recvlen = recv(client->connfd,recvbuf,MAX_SIZE,0)) > 0){

		recvbuf[recvlen] = '\0';
		printf("Receive from client <%s> message :%s, muti send now ...\n",client->name,recvbuf);

		//获取群聊用户列表
		NetChatClient *head  = loadClientDataWithMode(MODE_MUTI);
		NetChatClient *ptr = head;

		//获取当前系统时间
		time_t t1;
		t1 = time(NULL);//机器时间 
		char *t2;
		t2 = ctime(&t1); //转换为字符串时间

		//格式化输出信息
		sprintf(sendbuf,"<%s> %s\t %s\n",client->name,t2,recvbuf);
		
		printf("Matched Node List:\n");
		displayForEach(head);

		printf("Recv node list:\n");
		//遍历用户列表发送信息
		while(ptr != NULL){
			if(ptr->connfd == client->connfd) continue;
			displayClientNode(ptr);
			send(ptr->connfd,sendbuf,strlen(sendbuf),0);
			ptr = ptr->next;
		}
	}
}
