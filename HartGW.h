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
#include "sharedmem.h"

#define HART_CMDS_NUM_MAX  200
#define HART_CMDS_INTERVAL  30

const unsigned char hart_cmd_1[]={0xFF,0xFF,0xFF,0xFF,0xFF,0x82,0xA6,0x06,0x00,0x00,0x00,0x01,0x00,0x00};
const unsigned char hart_cmd_2[]={0xFF,0xFF,0xFF,0xFF,0xFF,0x82,0xA6,0x06,0x00,0x00,0x00,0x02,0x00,0x00};
const unsigned char hart_cmd_11[]={0xFF,0xFF,0xFF,0xFF,0xFF,0x82,0xA6,0x06,0x00,0x00,0x00,0x0B,0x06,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
const unsigned char hart_cmd_12[]={0xFF,0xFF,0xFF,0xFF,0xFF,0x82,0xA6,0x06,0x00,0x00,0x00,0x0C,0x00,0x00};
const unsigned char hart_cmd_13[]={0xFF,0xFF,0xFF,0xFF,0xFF,0x82,0xA6,0x06,0x00,0x00,0x00,0x0D,0x00,0x00};
const unsigned char hart_cmd_14[]={0xFF,0xFF,0xFF,0xFF,0xFF,0x82,0xA6,0x06,0x00,0x00,0x00,0x0E,0x00,0x00};
const unsigned char hart_cmd_15[]={0xFF,0xFF,0xFF,0xFF,0xFF,0x82,0xA6,0x06,0x00,0x00,0x00,0x0F,0x00,0x00};
const unsigned char hart_cmd_16[]={0xFF,0xFF,0xFF,0xFF,0xFF,0x82,0xA6,0x06,0x00,0x00,0x00,0x10,0x00,0x00};

//====================================================Hart设备属性定义=====================================================================
struct Hart_Device_Property_UC          //使用通用(Universal Commands)能够读取的属性
{
    unsigned char imfrs_ID;             //制造商ID
    unsigned char iMfrs_Dev_Type;       //制造商设备类型
    unsigned char iFF_Nums;             //请求的前导符数
    unsigned char iUC_Ver;              //通用命令文档版本号 
    unsigned char iDev_Ver;             //变送器规范版本号
    unsigned char iDev_SW_Ver;          //设备软件版本号
    unsigned char iDev_HW_Ver;          //设备硬件版本号
    unsigned char iDev_Flag;            //设备标志
    unsigned char iDev_ID;              //设备ID号
    unsigned char iPV_Unit_ID;          //主变量单位代码
    float fPV_Value;                    //主变量（PV）
    float fPV_Cur;                      //主变量电流
    float fPV_Per;                      //主变量量程百分比
    char sTag[6];                       //标签Tag
    char sDec[12];                      //描述符Description
    unsigned int iDate;                 //日期Date
    unsigned int fSensor_Upper;         //变量传感器上限
    float fSensor_Floor;                //变量传感器下限
    float fPV_Range_Mini;               //主变量最小量程
    char sMessage[24];                  //读消息（Message）
    unsigned int iPV_Alarm_Code;        //主变量报警选择代码
    unsigned int iPV_Transfer_Code;     //主变量传递Transfer功能代码
    unsigned int iPV_Limit_Unit;        //主变量上下量程值单位代码
    float fPV_Upper;                    //主变量上限值
    float fPV_Floor;                    //主变量下限值
    float fPV_Damp;                     //主变量阻尼值
    unsigned int iWrite_Protect;        //写保护代码
    char sRead_Tag[32];                 //读长标签
    unsigned int iResponse_FFs;         //响应最小前导符数（从-主）
    unsigned int iDev_Var_Nums;         //设备变量的最大个数
    unsigned int iDev_Status_1;         //附加设备状态（需要维护/参数报警）
    unsigned int iConfig_Modify_Nums;   //配置修改计数
    unsigned int iAssembly_Last;        //最终装配号
    unsigned int iPolling_Addr;         //polling地址
    unsigned char iVar2_Unit_ID;
    float fVar2_Value;
    unsigned char iVar3_Unit_ID;
    float fVar3_Value;
    unsigned char iVar4_Unit_ID;
    float fVar4_Value;
    unsigned char ReCode1;
    unsigned char ReCode2;
};

struct Hart_Device_Property_CC          //使用常用(Common Practice Commands)能够读取的属性
{
    unsigned int modbus_type;
    unsigned int modbus_address_base;
    unsigned int iSet_Var_Code;         //设置变量代码
    unsigned int iPV_Function_Code;     //主变量传递函数代码
    unsigned int iSensor_Code;          //设备变量传感器序列号
    unsigned int iDev_Status_2;         //设备特殊状态
    unsigned int iDev_Status_3;         //额外的设备状态
    unsigned int iDev_Work_Mode;        //设备操作模式
    unsigned int iChannel_1;            //模拟通道饱和
    unsigned int iChannel_2;            //模拟通道固定
    unsigned int iDev_Status_4;         //设备特殊状态
    unsigned int iBurst_Mode;           //Burst模式控制代码
    unsigned int iChannel_Code;         //模拟通道号代码
    unsigned int iLock_Code;            //锁定代码
    unsigned int iLock_Status;          //锁定状态
    unsigned int iMessage_Code;         //响应消息的命令号
    unsigned int iPort_Code;            //端口分配代码
    unsigned int iFunction_Code;        //功能代码
    unsigned int iHost_Data_Len;        //主机支持的最大数据长度
    unsigned int iHost_Head_Nums;       //主机开始字节计数
    unsigned int iCapture_Mode_Code;    //捕获模式代码
};

struct Hart_Device_Property_SC                      //使用特殊命令能够读取的属性
{

};

struct Hart_Device
{
    struct Hart_Device_Property_UC uc;				//通用属性
    struct Hart_Device_Property_CC cc;				//常用属性
    struct Hart_Device_Property_SC sc;				//特殊属性
};

struct hartcmd_struct
{
    unsigned int iEn;               //本条命令是否允许
    unsigned int iTimeout;          //本条命令通讯是否超时
    unsigned int iSuccCount;        //本条命令通讯成功的次数
    unsigned int iFailedCount;      //本条命令通讯失败的次数
    unsigned int iTimeoutCount;     //通讯超时的次数
    unsigned char bID[5];           //本条命令使用的Hart ID
    unsigned char bTag[6];          //本条命令使用的Tag
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
struct Hart_Device HD[HART_CMDS_NUM_MAX];	//Hart设备数组
unsigned char hart_recv_buf[2500];        //Hart接受返回的命令缓存
unsigned int hart_recv_len;               //Hart接受返回的命令缓存长度
unsigned int hart_run_status_reg;         //将Hart运行状态写入到这个Modnet的地址中
struct timeval tv_start,tv_end;         //记录开始轮询命令的开始时间和结束时间
unsigned int hart_debug_mode,hart_test_mode;

int HartGW_Init(char *ConfigFileName);									//读取xml配置文件初始化
unsigned char Hart_CmdCheckCode(unsigned char *cmd_buf,int cmd_len);	//生成Hart命令的校验码
unsigned int Hart_Send();												//根据需要生成Hart命令并发送
unsigned int Hart_ComDO();												//处理返回的modbusRTU命令
