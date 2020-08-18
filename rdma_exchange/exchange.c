
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
#define CMD_WRITE	0x120 // FOR EXCHANGE DATA CMD.
#define CMD_REMOVE  0x121 
#define CMD_READ	0X110

typedef struct Serverinfo{
	int socketfd;
	struct sockaddr_in addr;
	pthread_t tid;
    void *shmaddr;
}Serverinfo;


struct mr_phy_info {
	uint64_t	phyaddr;
	uint64_t    vaddr;
	uint32_t 	rkey;
	uint32_t    len ;// sometimes may not from the beginning of page.
    uint64_t    rsved;  //for 32bytes aligned.
};

struct sg_phy_info {
	uint64_t	phyaddr;
	uint64_t	size;
};

struct qp_vaddr{
    uint32_t    cmd;
    uint32_t 	rkey; // get  server's rkey.
	uint64_t 	vaddr;
    uint64_t    phyaddr;
    uint32_t    len;
};


//sem_t sem_id; // for there is 1024 qp to access.
static pthread_rwlock_t rw_lock;
static mr_len = 0;
int tmp_num = 0;
int thread_num = 1024;
pthread_mutex_t *hw_lock =NULL;
struct mr_phy_info *mr_pool = NULL;


static void bxroce_remove_mr(struct mr_phy_info *mr_pool, int i)
{
    int movelen = mr_len - i -1;
    //use memmove to delete mr.
    if(movelen == 0)
    memset(&mr_pool[i],0,sizeof(struct mr_phy_info));
    else
    memmove(&mr_pool[i],&mr_pool[i+1],movelen);
}

static void *server_fun(void *arg){
	Serverinfo *info = (Serverinfo *)arg;
	struct sg_phy_info *sginfo = NULL;
	struct sg_phy_info *tmpsginfo =NULL;
	struct qp_vaddr *vaddr = NULL;
    struct qp_vaddr *tmpvaddr = NULL;
	int len = 0;
	int i =0;
    int offset =0;

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

		printf("[IP:%s, port:%d] recv data:%lx, cmd:0x%x, rkey:0x%x\n", \
				inet_ntoa(info->addr.sin_addr), \
				ntohs(info->addr.sin_port), vaddr->vaddr,vaddr->cmd,vaddr->rkey);

        
        switch(vaddr->cmd)
        {
            case CMD_READ:
                        pthread_rwlock_rdlock(&rw_lock);
                        printf("get in read\n");
                        sginfo->phyaddr = vaddr->vaddr;
                        for(i=0;i<mr_len;i++)
                        {
                            if(vaddr->rkey == mr_pool[i].rkey)
                            {
                                if((vaddr->vaddr >= mr_pool[i].vaddr) && (vaddr->vaddr <= (mr_pool[i].vaddr + mr_pool[i].len)))
                                {
                                    printf("find server's dma addr\n");
                                    offset = vaddr->vaddr - mr_pool[i].vaddr;
                                    sginfo->phyaddr = mr_pool[i].phyaddr + offset;
                                    sginfo->size    = mr_pool[i].len;
                                    break;
                                }
                            }
                        }
                        printf("get out read\n");
                        pthread_rwlock_unlock(&rw_lock);
                        break;
            case CMD_WRITE:
                        pthread_rwlock_wrlock(&rw_lock);
                        mr_pool[mr_len].rkey = vaddr->rkey;
                        mr_pool[mr_len].vaddr = vaddr->vaddr;
                        mr_pool[mr_len].phyaddr = vaddr->phyaddr;
                        mr_pool[mr_len].len    = vaddr->len;

                        sginfo->phyaddr = mr_pool[mr_len].phyaddr;
                        mr_len++;
                        pthread_rwlock_unlock(&rw_lock);
                        break;
            case CMD_REMOVE:
                        pthread_rwlock_wrlock(&rw_lock);

                        for(i=0;i<mr_len;i++)
                        {
                            if(mr_pool[i].rkey == vaddr->rkey)
                            {
                                if(mr_pool[i].vaddr == vaddr->vaddr)
                                {
                                    printf("dereg mr find \n");
                                    bxroce_remove_mr(mr_pool,i);
                                    printf("remove success\n");
                                    break;
                                }
                            }
                        }
                        sginfo->phyaddr = vaddr->phyaddr;
                        mr_len--;
                        pthread_rwlock_unlock(&rw_lock);
                        break;
            default:
                    printf("cmd error \n");
                    sginfo->phyaddr = vaddr->vaddr;
                    break;
                        
        }


        len = sizeof(struct sg_phy_info);
		printf("send\n");
		write(info->socketfd,sginfo,len);

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
    int robust;
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

	if(setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int)) < 0)
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
    //sem_init(&sem_id,0,1);//0-1 for every qp.
    if(pthread_rwlock_init(&rw_lock,NULL) != 0)
    {
        printf("rwlock init failed\n");
        return -1;
    }

    //alloc shm
    buflen =sizeof(pthread_mutex_t); //this is for all process to mutex hw access.
    shm = shmget(IPC_KEY,buflen,IPC_CREAT|0666);
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
    memset(shmstart,0,buflen);

	/* init mutex*/
	hw_lock = (pthread_mutex_t *)(shmstart);
	if(pthread_mutexattr_init(&mat) !=0)
	{
		printf("err mutexattr init\n");
		return -1;
	}

    if(pthread_mutexattr_getrobust(&mat,&robust) != 0)
    {
        printf("get robust attr err\n");
        return -1;
    }
	if(pthread_mutexattr_setpshared(&mat,PTHREAD_PROCESS_SHARED) !=0)
	{
		printf("err mutexarr setpshared\n");
		return -1;
	}

    if(robust != PTHREAD_MUTEX_ROBUST)
    {
        if(pthread_mutexattr_setrobust(&mat,PTHREAD_MUTEX_ROBUST) != 0)
            printf("err mutexarr setrobust");
            return -1;
    }

	
	pthread_mutex_init(hw_lock,&mat);
	/*end */
    //alloc 4M SIZE space to mr pool.
	mr_pool =(struct mr_phy_info *)malloc(MR_REGION_SIZE);
    memset(mr_pool,0,MR_REGION_SIZE);

	printf("listen...waiting for client..\n");
	socklen_t len = sizeof(struct sockaddr_in);
	while(1)
	{
		tmp_num++;
		if(tmp_num > thread_num)
		{printf("too much client,close socket\n");break;}
		
		Serverinfo *info = malloc(sizeof(*info));
        
		info->socketfd = accept(socket_fd, (struct sockaddr*)&info->addr, &len);
		pthread_create(&info->tid, NULL, server_fun, info);
		printf("pthread detach start\n");
		pthread_detach(info->tid);
		printf("pthread detach end \n");

	}

	printf("socket close, child process exit\n");
	close(socket_fd);
    
    pthread_mutex_destroy(hw_lock);

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

    pthread_rwlock_destroy(&rw_lock);

	exit(EXIT_SUCCESS);
}