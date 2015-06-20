#include <stdio.h>      /*标准输入输出定义*/
#include <stdlib.h>     /*标准函数库定义*/
#include <unistd.h>     /*Unix 标准函数定义*/
#include <sys/types.h> 
#include <sys/stat.h>   
#include <fcntl.h>      /*文件控制定义*/
#include <termios.h>    /*PPSIX 终端控制定义*/
#include <errno.h>      /*错误号定义*/
#include <string.h>
#include <syslog.h>     //系统日志定义
#include <mxml.h>

#include "serial.h"		//自己定义的串口通讯头文件
//#include "hartcmd.h"	//hart协议解析
#include "sharedmem.h"

#define HART_CMDS_NUM_MAX  200
#define HART_CMDS_INTERVAL  30

const unsigned char hart_cmd_1[]={0xFF,0xFF,0xFF,0xFF,0xFF,0x82,0xA6,0x06,0x00,0x00,0x00,0x01,0x00,0x00};
const unsigned char hart_cmd_2[]={0xFF,0xFF,0xFF,0xFF,0xFF,0x82,0xA6,0x06,0x00,0x00,0x00,0x02,0x00,0x00};
const unsigned char hart_cmd_12[]={0xFF,0xFF,0xFF,0xFF,0xFF,0x82,0xA6,0x06,0x00,0x00,0x00,0x0C,0x00,0x00};
const unsigned char hart_cmd_13[]={0xFF,0xFF,0xFF,0xFF,0xFF,0x82,0xA6,0x06,0x00,0x00,0x00,0x0D,0x00,0x00};
const unsigned char hart_cmd_14[]={0xFF,0xFF,0xFF,0xFF,0xFF,0x82,0xA6,0x06,0x00,0x00,0x00,0x0E,0x00,0x00};
const unsigned char hart_cmd_15[]={0xFF,0xFF,0xFF,0xFF,0xFF,0x82,0xA6,0x06,0x00,0x00,0x00,0x0F,0x00,0x00};
const unsigned char hart_cmd_16[]={0xFF,0xFF,0xFF,0xFF,0xFF,0x82,0xA6,0x06,0x00,0x00,0x00,0x10,0x00,0x00};

struct hartcmd_struct
{
    unsigned int iEn;               //本条命令是否允许
    unsigned int iTimeout;          //本条命令通讯是否超时
    unsigned int iSuccCount;        //本条命令通讯成功的次数
    unsigned int iFailedCount;      //本条命令通讯失败的次数
    unsigned int iTimeoutCount;     //通讯超时的次数
    unsigned char bID[3];           //本条命令使用的Hart ID
    unsigned char bCMD;             //本条命令使用的Hart命令号
    unsigned int iMapAddress;       //本条命令使用的映射到MODNET的寄存器地址
    unsigned int iDeviceType;       //本条命令对应的Hart设备的类型,在解析返回值时根据不同的设备类型可能解析的结果不同,0代表通用设备
    unsigned int iPort;             //本条命令对应的Hart设备对应Hart Hub的端口
};

struct hart_struct
{
    unsigned int iCmdIndex;         //准备发送的命令索引
    unsigned int iSendedIndex;      //已经发送的命令索引
    unsigned int iInterval;         //两条Hart命令之间的时间间隔
    unsigned int iIntervalCount;    //两条Hart命令之间的时间间隔计数器
    struct hartcmd_struct HTCmds[HART_CMDS_NUM_MAX]; 
};
uint16_t *modnet_holdreg_buf;           //modnet holding reg 指针
uint16_t *modnet_inputreg_buf;          //modnet input reg 指针
unsigned char *modnet_coilst_buf;       //modnet coil 寄存器指针
unsigned char *modnet_inputst_buf;      //modnet input 寄存器指针

struct hart_struct HT;
unsigned char hart_recv_buf[2500];        //Hart接受返回的命令缓存
unsigned int hart_recv_len;               //Hart接受返回的命令缓存长度
unsigned int hart_run_status_reg;         //将Hart运行状态写入到这个Modnet的地址中
struct timeval tv_start,tv_end;         //记录开始轮询命令的开始时间和结束时间
unsigned int hart_debug_mode,hart_test_mode;

int HartGW_Init();														//读取xml配置文件初始化
unsigned char Hart_CmdCheckCode(unsigned char *cmd_buf,int cmd_len);	//生成Hart命令的校验码
unsigned int Hart_Send();												//根据需要生成Hart命令并发送
unsigned int Hart_ComDO()											//处理返回的modbusRTU命令
