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

#define MODE_REV    0x4201 //逆回文服务
#define MODE_MUTI   0x4203 //群聊
#define MODE_ERROR  0x4204 //模式选择错误

#define MAX_SIZE 100    //最大缓冲区大小
const char *DATA_FILE_NAME = "client.cache";//用户数据缓存文件名

typedef struct NetChatClient{
	int connfd;				     //套接字描述符
	char name[MAX_SIZE]; 	     //用户昵称
	int mode;				     //当前用户节点所处模式
	struct sockaddr_in addr;     //当前用户网际套接字地址结构
	struct NetChatClient * next; //下一个用户节点地址指针
}NetChatClient;

//创建一个用户节点
NetChatClient * createClientNode(){
	NetChatClient *node = (struct NetChatClient *)malloc(sizeof(NetChatClient));
	bzero(node,sizeof(NetChatClient));
	node->next = NULL;
	return node;
}

//依据参数创建一个用户节点
NetChatClient * createClientNodeParam(int connectfd, char *nickname, struct sockaddr_in client, int mode) {
	NetChatClient *node = createClientNode();
	node->connfd = connectfd;
	strcpy(node->name , nickname);
	node->addr.sin_family = client.sin_family;
	node->addr.sin_addr = client.sin_addr;
	node->addr.sin_port = client.sin_port;
	node->mode = mode;
	return node;
}

//释放指定用户节点
void destoryClientNode(NetChatClient *head, NetChatClient *node){
	if(head==NULL || node==NULL) return;
	NetChatClient * current = head;
	while(current!=NULL){
		if(current->next == node) break;
		current = current->next;
	}
	current ->next = current->next->next;
	free(node);
}

//释放链表空间
void freeList(NetChatClient *head){

   NetChatClient *ptr;
   while ( head != NULL ){
      ptr = head;
      head = head->next;
      free(ptr);
   }

}

//打印指定用户节点
void displayClientNode(NetChatClient *node){
	if(node!=NULL){
		printf("client <[%d]%s:%d>, connfd <%d>, name <%s>, mode <%s>\n",
					node->addr.sin_family,
					inet_ntoa(node->addr.sin_addr),
					ntohs(node->addr.sin_port),
					node->connfd,
					node->name,
					(node->mode == MODE_REV)?"REV":
							((node->mode == MODE_MUTI)?"MUTI":"ERROR"));
	}
}

//遍历打印所有用户节点
void displayForEach(NetChatClient *head){
	NetChatClient * currentNode = head;
    while(currentNode != NULL){
    	displayClientNode(currentNode);
    	currentNode = currentNode ->next;
    }
}

//获取链表最后一个节点
NetChatClient *getLastNode(NetChatClient *head){
	if(head==NULL) return NULL;
	NetChatClient * currentNode = head;
    while(currentNode->next != NULL){
    	currentNode = currentNode ->next;
    }
    return currentNode;
}

//将指定到用户节点添加到链表尾部，并返回链表首节点指针
NetChatClient *addtoListEnd(NetChatClient *head,NetChatClient *node){
	if(head==NULL || node==NULL) return NULL;
	getLastNode(head)->next = node;
	return head;
}

//将用户节点追加至缓存文件
void saveNodeData(NetChatClient *node){
	if(node==NULL) return;
	if(node->mode == MODE_ERROR) return;

	//使用“追加”的方式打开文件
	FILE *clients = fopen(DATA_FILE_NAME,"ab");
	//直接在文件末尾写入内容
	fwrite(node,sizeof(NetChatClient),1,clients);
	fclose(clients);
}

//将链表所有节点存至缓存文件暂存
void saveListData(NetChatClient *head){

	NetChatClient * currentNode = head;
	FILE *clients = fopen(DATA_FILE_NAME,"wb");

	while(currentNode != NULL){
		fwrite(currentNode,sizeof(NetChatClient),1,clients);
		currentNode = currentNode ->next;
	}
	fclose(clients);

}

//从缓存文件中载入用户数据
NetChatClient *loadClientData(){

	NetChatClient * head;
	int isFirstNode = 1 ;

	FILE *clients = fopen(DATA_FILE_NAME,"rb");
	while(!feof(clients)){

		NetChatClient * temp = createClientNode();
		fread(temp,sizeof(NetChatClient),1,clients);
		temp->next = NULL;

		if(isFirstNode){
			isFirstNode = 0;
			head = temp;
		} else {
			addtoListEnd(head,temp);
		}

	}

	fclose(clients);

	//释放最后一个节点，因为文件结尾有一个结束符，有一个多余的“脏数据”
	destoryClientNode(head,getLastNode(head));

	return head;
}

//从缓存文件中载入指定模式用户列表
NetChatClient *loadClientDataWithMode(int mode){

	NetChatClient * head;
	int isFirstNode = 1 ;

	FILE *clients = fopen(DATA_FILE_NAME,"rb");
	while(!feof(clients)){

		NetChatClient * temp = createClientNode();
		fread(temp,sizeof(NetChatClient),1,clients);
		temp->next = NULL;

		if(temp->mode != mode) {
			free(temp);
			continue;
		}

		if(isFirstNode){
			isFirstNode = 0;
			head = temp;
		} else {
			addtoListEnd(head,temp);
		}

	}

	fclose(clients);

	return head;
}

//从缓存文件中去除指定套接字描述符到用户节点
void destoryClientCache(NetChatClient *node){
	NetChatClient * head = NULL;
	int isFirstNode = 1 ;
	FILE *clients = fopen(DATA_FILE_NAME,"rb");
	while(!feof(clients)){
		NetChatClient * temp = createClientNode();
		fread(temp,sizeof(NetChatClient),1,clients);
		temp->next = NULL;
		//节点比较
		//如果是指定节点，则释放相应节点内存，并不执行当次循环
		if(ClientNodeCompare(node,temp)){
			free(temp);
			continue;
		}
		if(isFirstNode){
			isFirstNode = 0;
			head = temp;
		} else {
			addtoListEnd(head,temp);
		}
	}
	fclose(clients);
	//释放最后一个节点，因为文件结尾有一个结束符，有一个多余的“脏数据”
	destoryClientNode(head,getLastNode(head));
	//save data
	saveListData(head);
	//free list
	freeList(head);
}

//更新缓存文件中指定的用户节点
void updateClientCache(NetChatClient *node){

		if(node == NULL) return;

		NetChatClient * head = NULL;
		int isFirstNode = 1 ;

		FILE *clients = fopen(DATA_FILE_NAME,"rb");
		while(!feof(clients)){

			NetChatClient * temp = createClientNode();
			fread(temp,sizeof(NetChatClient),1,clients);
			temp->next = NULL;

			if(temp->connfd == node->connfd) {
				strcpy(temp->name , node->name);
				temp->addr.sin_family = node->addr.sin_family;
				temp->addr.sin_addr = node->addr.sin_addr;
				temp->addr.sin_port = node->addr.sin_port;
				temp->mode = node->mode;
			}

			if(isFirstNode){
				isFirstNode = 0;
				head = temp;
			} else {
				addtoListEnd(head,temp);
			}

		}

		fclose(clients);

		//释放最后一个节点，因为文件结尾有一个结束符，有一个多余的“脏数据”
		destoryClientNode(head,getLastNode(head));

		//save data
		saveListData(head);

		//free list
		freeList(head);
}

//用户节点比较
int ClientNodeCompare(NetChatClient *node1 , NetChatClient *node2){

	if(node1->connfd != node2->connfd) return 0;
	if(strcmp(node1->name,node2->name)) return 0;
	if(node1->mode != node2->mode) return 0;
	if(node1->addr.sin_family!=node2->addr.sin_family) return 0;
	if(node1->addr.sin_addr.s_addr!=node2->addr.sin_addr.s_addr) return 0;
	if(node1->addr.sin_port!=node2->addr.sin_port) return 0;
	return 1;
}
