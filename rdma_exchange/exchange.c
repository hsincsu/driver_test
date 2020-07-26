
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/shm.h>
#include <unistd.h>
#include <semaphore.h>
#include <signal.h>
#include <fcntl.h>
#include <strings.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <pthread.h>

#define IPC_KEY 	0x88888888
#define QPNUM 		1024
#define MR_REGION_SIZE 4*1024*1024  //4M size.

typedef struct Serverinfo{
	int socketfd;
	struct sockaddr_in addr;
	pthread_t tid;
    void *shmaddr;
}Serverinfo;

struct mr_head
{
	struct sg_phy_info *next;
	struct sg_phy_info *rear;
}

struct sg_phy_info {
	uint64_t	phyaddr;
	uint64_t    vaddr;
	uint32_t 	rkey;
	uint32_t    len ;// sometimes may not from the beginning of page.
};


struct qp_vaddr{
	uint64_t 	vaddr;
	uint32_t 	rkey; // get  server's rkey.
};


sem_t sem_id; // for there is 1024 qp to access.
int tmp_num = 0;
int thread_num = 1024;
pthread_mutex_t *hw_lock =NULL;

static void *server_fun(void *arg){
	Serverinfo *info = (Serverinfo *)arg;
	struct sg_phy_info *sginfo = NULL;
	struct sg_phy_info *tmpsginfo =NULL;
	struct qp_vaddr *vaddr = NULL;
    struct qp_vaddr *tmpvaddr = NULL;
	int len = 0;
	struct bxroce_mr_sginfo *mr_sginfo;
	int i =0;

	vaddr = malloc(sizeof(*vaddr));
	sginfo = malloc(sizeof(struct sg_phy_info));
	tmpsginfo = sginfo;

	memset(vaddr,0,sizeof(*vaddr));
	memset(sginfo,0,sizeof(struct sg_phy_info));
	printf("accept client IP:%s, port:%d\n", \
			inet_ntoa(info->addr.sin_addr), \
			ntohs(info->addr.sin_port));
 

	while(1){
		len = sizeof(*vaddr);
		int readret = read(info->socketfd,vaddr,len);
		printf("readret:0x%x \n",readret);
		if(readret == -1)
		{
			printf("err:read failed caused by %s \n",strerror(errno));
			free(info);
			pthread_exit(NULL);
		}else if(readret == 0){
			printf("client has closed\n");
			close(info->socketfd);
			break;
		}

		printf("[IP:%s, port:%d] recv data:%lx, qp->id:0x%x\n", \
				inet_ntoa(info->addr.sin_addr), \
				ntohs(info->addr.sin_port), vaddr->vaddr,vaddr->qpid);

        sem_wait(&sem_id);
        tmpvaddr = (struct qp_vaddr *)info->shmaddr;
        tmpvaddr->vaddr = vaddr->vaddr;
        tmpvaddr->qpid  = vaddr->qpid;
        while(tmpvaddr->qpid != 0)
        {
            usleep(10);
        }
        printf("data is updated\n");
        sginfo->phyaddr = tmpvaddr->vaddr;
        memset(tmpvaddr,0,sizeof(*tmpvaddr));
        sem_post(&sem_id);

        len = sizeof(struct sg_phy_info);
		printf("send\n");
		write(info->socketfd,sginfo,len);
		break;

	}
	
    free(info);
	printf("pthread exit\n");
	pthread_exit(NULL);

}


int main(int argc, char* argv[])
{
    int socket_fd = socket(AF_INET,SOCK_STREAM,0);
	int port_num = 11988; // default port is 11988
    int i = 0;
    int shm;
    int buflen = 0;
    void *shmstart = NULL;
	uint8_t *shmoffset = NULL;
	pthread_mutexattr_t mat;

	if(socket_fd < 0)
	{
		printf("child process create failed,casued by %s \n",strerror(errno));
		return;
	}

	//bind
	struct sockaddr_in server_sock;
	memset(&server_sock,0,sizeof(server_sock));

	server_sock.sin_family = AF_INET;
	server_sock.sin_addr.s_addr = htonl(INADDR_ANY);
	server_sock.sin_port   = htons(port_num);

	int on = 1;

	if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int)) < 0)
	{
		printf("set error\n");
		exit(0);
	}

	int bind_sta = bind(socket_fd,(struct sockaddr*)&server_sock, sizeof(server_sock));
	if(bind_sta < 0)
	{
		printf("bind error,casued by %s \n",strerror(errno));
		return ;
	}

	int listenret = listen(socket_fd,128); // listen 128 queue. if over it ,then client will failed.
	if(listenret < 0)
	{
		printf("listen error,caused by %s \n",strerror(errno));
	}

    // init 1024 sem
    sem_init(&sem_id,0,1);//0-1 for every qp.
    
    //alloc shm
    buflen =sizeof(pthread_mutex_t); //this is for all process to mutex hw access.
    shm = shmget(IPC_KEY,buflen,IPC_CREAT|0664);
    if(shm < 0)
    {
        printf("shmget error\n");
        return -1;
    }

    shmstart = shmat(shm,NULL,0);
    if(shmstart == (void *)-1)
    {
        printf("err to map shm addr\n");
        return -1;
    }

	/* init mutex*/
	hw_lock = (pthread_mutex_t *)(shmstart);
	if(pthread_mutexattr_init(&mat) !=0)
	{
		printf("err mutexattr init\n");
		return -1;
	}

	if(pthread_mutexattr_setpshared(&mat,PTHREAD_PROCESS_SHARED) !=0)
	{
		printf("err mutexarr setpshared\n");
		return -1;
	}
	
	pthread_mutex_init(hw_lock,&mat);
	/*end */

	

	printf("listen...waiting for client..\n");
	socklen_t len = sizeof(struct sockaddr_in);
	while(1)
	{
		tmp_num++;
		if(tmp_num > thread_num)
		{printf("too much client,close socket\n");break;}
		
		Serverinfo *info = malloc(sizeof(*info));
        
		info->socketfd = accept(socket_fd, (struct sockaddr*)&info->addr, &len);
        info->shmaddr = shmstart;
		pthread_create(&info->tid, NULL, server_fun, info);
		printf("pthread detach start\n");
		pthread_detach(info->tid);
		printf("pthread detach end \n");

	}

	printf("socket close, child process exit\n");
	close(socket_fd);
    
    
    if(shmdt(shmstart) == -1)
    {
        printf("failed to shmdt\n");
        exit(EXIT_FAILURE);
    }

    //dealloc shm
    if(shmctl(shm,IPC_RMID,NULL) == -1)
    {
        printf("failed to IPC_RMID shmctl\n");
        exit(EXIT_FAILURE);
    }


	exit(EXIT_SUCCESS);
}