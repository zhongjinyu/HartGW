#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>
#include <netinet/in.h>
#include <syslog.h>

#define SHM_MODBUS_HOLDREG_KEY	1001
#define SHM_MODBUS_INPUTREG_KEY	1002
#define SHM_MODBUS_COILST_KEY	1003
#define SHM_MODBUS_INPUTST_KEY	1004
#define SEM_MODBUS_HOLDREG_KEY	2001
#define SEM_MODBUS_INPUTREG_KEY	2002
#define SEM_MODBUS_COILST_KEY	2003
#define SEM_MODBUS_INPUTST_KEY	2004

union semun
{
	int val;
	struct semid_ds *buf;
	unsigned short *arry;
};

static int sem_mb_holdreg_id=0;
static int sem_mb_inputreg_id=0;
static int sem_mb_coilst_id=0;
static int sem_mb_inputst_id=0;  

void *shm_mb_holdreg_buf;
void *shm_mb_inputreg_buf;
void *shm_mb_coilst_buf;
void *shm_mb_inputst_buf;

static int shm_mb_holdreg_id=0;
static int shm_mb_inputreg_id=0;
static int shm_mb_coilst_id=0;
static int shm_mb_inputst_id=0;

static int set_semvalue(int sem_id);
static int del_semvalue(int sem_id);
static int semaphore_p(int sem_id);
static int semaphore_v(int sem_id);
static int sem_Init();				//信号量初始化
static int shm_Init();				//共享内存初始化

//用于初始化信号量，在使用信号量前必须这样做
static int set_semvalue(int sem_id)
{
    union semun sem_union;

	sem_union.val = 1;
	if(semctl(sem_id, 0, SETVAL, sem_union) == -1)
	{
    	syslog(LOG_DEBUG,"[ERR][ID=%d]set semaphore value.",sem_id);
		return 1;
	}
	return 0;
}

//删除信号量 
static int del_semvalue(int sem_id)
{        
    union semun sem_union;
  
    if(semctl(sem_id, 0, IPC_RMID, sem_union) == -1)
    { 
        syslog(LOG_DEBUG,"[ERR][ID=%d]delete semaphore.",sem_id);
        return 1;
    }
    return 0;
}  

//对信号量做减1操作，即等待P（sv）
static int semaphore_p(int sem_id)  
{  
    struct sembuf sem_b;

	sem_b.sem_num = 0;
	sem_b.sem_op = -1;//P()
	sem_b.sem_flg = SEM_UNDO;
	if(semop(sem_id, &sem_b, 1) == -1)
	{   
		syslog(LOG_DEBUG,"[ERR][ID=%d]semaphore_p failed.",sem_id);
		return 1;  
	}  
	return 0;  
}  
  
//这是一个释放操作，它使信号量变为可用，即发送信号V（sv）
static int semaphore_v(int sem_id)
{
    struct sembuf sem_b;

	sem_b.sem_num = 0;
	sem_b.sem_op = 1;//V()
	sem_b.sem_flg = SEM_UNDO;
	if(semop(sem_id, &sem_b, 1) == -1)
	{   
		syslog(LOG_DEBUG,"[ERR][ID=%d]semaphore_v failed.",sem_id);
		return 1;
	}
	return 0;  
}

//信号量初始化
static int sem_Init()
{
	sem_mb_holdreg_id = semget((key_t)SEM_MODBUS_HOLDREG_KEY, 1, 0666 | IPC_CREAT);
	sem_mb_inputreg_id = semget((key_t)SEM_MODBUS_INPUTREG_KEY, 1, 0666 | IPC_CREAT);
	sem_mb_coilst_id = semget((key_t)SEM_MODBUS_COILST_KEY, 1, 0666 | IPC_CREAT);
	sem_mb_inputst_id = semget((key_t)SEM_MODBUS_INPUTST_KEY, 1, 0666 | IPC_CREAT);

	return 0;
}

//共享内存初始化
static int shm_Init()
{
	//创建共享内存 for hold reg,如果共享内存空间已经存在就返回标示
	shm_mb_holdreg_id=shmget((key_t)SHM_MODBUS_HOLDREG_KEY,sizeof(uint16_t)*0xffff,0666|IPC_CREAT);
	if(shm_mb_holdreg_id==-1)
	{
		syslog(LOG_DEBUG,"[ERR][ShareMEM][HOLDREG]shmget failed");
		exit(EXIT_FAILURE);
	}
	//将共享内存连接到当前进程的地址空间
	shm_mb_holdreg_buf=shmat(shm_mb_holdreg_id,0,0);
	if(shm_mb_holdreg_buf==(void*)-1)
	{
		syslog(LOG_DEBUG,"[ERR][ShareMEM][HOLDREG]shmat failed");
		exit(EXIT_FAILURE);
	}
	syslog(LOG_DEBUG,"[INFO][ShareMEM][HOLDREG]Memory attached at 0x%X",(uint)shm_mb_holdreg_buf);

	//创建共享内存 for input reg,如果共享内存空间已经存在就返回标示
	shm_mb_inputreg_id=shmget((key_t)SHM_MODBUS_INPUTREG_KEY,sizeof(uint16_t)*0xffff,0666|IPC_CREAT);
	if(shm_mb_inputreg_id==-1)
	{
		syslog(LOG_DEBUG,"[ERR][ShareMEM][INPUTREG]shmget failed");
		exit(EXIT_FAILURE);
	}
	//将共享内存连接到当前进程的地址空间
	shm_mb_inputreg_buf=shmat(shm_mb_inputreg_id,0,0);
	if(shm_mb_inputreg_buf==(void*)-1)
	{
		syslog(LOG_DEBUG,"[ERR][ShareMEM][INPUTREG]shmat failed");
		exit(EXIT_FAILURE);
	}
	syslog(LOG_DEBUG,"[INFO][ShareMEM][INPUTREG]Memory attached at 0x%X",(uint)shm_mb_inputreg_buf);

	//创建共享内存 for coilst reg,如果共享内存空间已经存在就返回标示
	shm_mb_coilst_id=shmget((key_t)SHM_MODBUS_COILST_KEY,sizeof(unsigned char)*0xffff,0666|IPC_CREAT);
	if(shm_mb_coilst_id==-1)
	{
		syslog(LOG_DEBUG,"[ERR][ShareMEM][COILST]shmget failed");
		exit(EXIT_FAILURE);
	}
	//将共享内存连接到当前进程的地址空间
	shm_mb_coilst_buf=shmat(shm_mb_coilst_id,0,0);
	if(shm_mb_coilst_buf==(void*)-1)
	{
		syslog(LOG_DEBUG,"[ERR][ShareMEM][COILST]shmat failed");
		exit(EXIT_FAILURE);
	}
	syslog(LOG_DEBUG,"[INFO][ShareMEM][COILST]Memory attached at 0x%X",(uint)shm_mb_coilst_buf);

	//创建共享内存 for inputst reg,如果共享内存空间已经存在就返回标示
	shm_mb_inputst_id=shmget((key_t)SHM_MODBUS_INPUTST_KEY,sizeof(unsigned char)*0xffff,0666|IPC_CREAT);
	if(shm_mb_inputst_id==-1)
	{
		syslog(LOG_DEBUG,"[ERR][ShareMEM][INPUTST]shmget failed");
		exit(EXIT_FAILURE);
	}
	//将共享内存连接到当前进程的地址空间
	shm_mb_inputst_buf=shmat(shm_mb_inputst_id,0,0);
	if(shm_mb_inputst_buf==(void*)-1)
	{
		syslog(LOG_DEBUG,"[ERR][ShareMEM][INPUTST]shmat failed");
		exit(EXIT_FAILURE);
	}
	syslog(LOG_DEBUG,"[INFO][ShareMEM][INPUTST]Memory attached at 0x%X",(uint)shm_mb_inputst_buf);
}

