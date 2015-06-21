#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <netinet/in.h>
#include <syslog.h>
#include <time.h>
#include <termios.h>
#include <errno.h>

#define SerialPort_Nums 4
#define SerialPort_Timeout  500    //超时默认设置=2秒

extern unsigned char hart_recv_buf[2500];        //Hart接受返回的命令缓存
extern unsigned int hart_recv_len;               //Hart接受返回的命令缓存长度

struct SerialPort_struct
{
    char sPortName[50];             //串口设备名称
    int iPort;                      //串口编号
    int iSpeed;                     //串口波特率
    int iFlow_ctrl;                 //串口流量控制
    int iDatabits;                  //串口数据位数
    int iStopbits;                  //串口停止位
    int iParity;                    //串口校验方式
    int ifd;                        //串口打开后的文件描述符
    unsigned char recv_buf[2500];   //接受数据缓存
    unsigned char send_buf[500];    //发送数据缓存
    int irecv_len;                  //接受缓存数据长度
    int isend_len;                  //发送缓存数据长度
    int irecv_begin;                //串口开始接收一帧数据的标志
    int irecv_no;                   //串口接受数据时间间隔计数,如果超过规定时间还没有继续收到数据说明一帧数据已经结束
    int itimer;                     //串口超时计数器,每次循环+1
    int itimeout;                   //串口超时上限
    int btimeout;                   //串口超时标志
    int bfree;                      //串口空闲标志
};

struct SerialPort_struct coms;
unsigned int com_debug_mode,com_test_mode;

int     UART_Open(char* port);
void    UART_Close();
int     UART_Set(int fd,int speed,int flow_ctrl,int databits,int stopbits,int parity);
int     UART_Init();
int     UART_Recv(int fd,char *rcv_buf,int data_len);
int     UART_Send(int fd,char *send_buf,int data_len);
int     UART_ComDO();

/*******************************************************************
* 名称:UART_Open
* 功能:打开串口并返回串口设备文件描述
* 入口参数: fd:     文件描述符
            port:   串口号(ttySAC0,ttySAC1,ttySAC2,ttySAC3)
* 出口参数：正确返回文件描述符，错误返回为-1
*******************************************************************/
int UART_Open(char* port)
{
    int fd;

    fd = open(port,O_RDWR|O_NOCTTY|O_NDELAY);
    if (fd == -1)
    {
        syslog(LOG_DEBUG,"[ERR][Serial][%s]Can''t Open Serial Port!",coms.sPortName);
        return -1;
    }
    //判断串口的状态是否为阻塞状态                                
    if(fcntl(fd, F_SETFL, 0) < 0)
    {
        syslog(LOG_DEBUG,"[ERR][Serial][%s]fcntl failed!",coms.sPortName);
        return -1;
    }               
    return fd;
}

/*******************************************************************
* 名称:UART_Close
* 功能:关闭所有串口
* 入口参数: null
* 出口参数：void
*******************************************************************/
void UART_Close()
{
    int i;
    
    if(coms.ifd)
    {
        syslog(LOG_DEBUG,"[INFO][Serail][%s]Port closed.",coms.sPortName);
        close(coms.ifd);        
    }
}

/*******************************************************************
* 名称:UART_Set
* 功能:设置串口数据位，停止位和效验位
* 入口参数：fd          串口文件描述符
*           speed       串口速度
*           flow_ctrl   数据流控制
*           databits    数据位   取值为 7 或者8
*           stopbits    停止位   取值为 1 或者2
*           parity      效验类型 取值为N,E,O,,S
*出口参数:正确返回为0，错误返回为-1
*******************************************************************/
int UART_Set(int fd,int speed,int flow_ctrl,int databits,int stopbits,int parity)
{
    int   i,err;
    int   status;
    int   speed_arr[] = {B115200,B57600,B38400,B19200,B9600,B4800,B2400,B1200,B300};
    int   name_arr[] = {115200,57600,38400,19200,9600,4800,2400,1200};
    struct termios options;
   
    //options初始化全部清零
    bzero(&options,sizeof(options));

    //tcgetattr(fd,&options)得到与fd指向对象的相关参数，并将它们保存于options,该函数还可以测试配置是否正确，该串口是否可用等。
    //若调用成功，函数返回值为0，若调用失败，函数返回值为其他.
    if (tcgetattr(fd,&options)!=0)
    {
        syslog(LOG_DEBUG,"[ERR][Serial][%s]Serial Port setup failed!",coms.sPortName);    
        return -1; 
    }
    cfmakeraw(&options);    //配置为原始模式     
    //设置串口输入波特率和输出波特率
    for (i=0;i<sizeof(speed_arr)/sizeof(int);i++)
    {
        if  (speed == name_arr[i])
        {             
            cfsetispeed(&options, speed_arr[i]); 
            cfsetospeed(&options, speed_arr[i]);  
            syslog(LOG_DEBUG,"[INFO][Serial][%s]baud rate:%d",coms.sPortName,name_arr[i]);
        }
    }     
   
    //设置数据位
    //屏蔽其他标志位
    options.c_cflag &= ~CSIZE;
    switch (databits)
    {  
        case 7:    
            options.c_cflag |= CS7;
            break;
        case 8:    
            options.c_cflag |= CS8;
            break;  
        default:   
            syslog(LOG_DEBUG,"[ERR][Serail][%s]Unsupported data size",coms.sPortName);
            return (-1); 
    }
    //设置校验位
    switch (parity)
    {  
        case 'n':
        case 'N': //无奇偶校验位。
            options.c_cflag &= ~PARENB; 
            options.c_iflag &= ~INPCK;  
            break; 
        case 'o':  
        case 'O'://设置为奇校验    
            options.c_cflag |= (PARODD | PARENB); 
            options.c_iflag |= INPCK;             
            break; 
        case 'e': 
        case 'E'://设置为偶校验  
            options.c_cflag |= PARENB;       
            options.c_cflag &= ~PARODD;       
            options.c_iflag |= INPCK;      
            break;
        default:  
            syslog(LOG_DEBUG,"[ERR][Serail][%s]Unsupported parity",coms.sPortName);    
            return (-1); 
    } 
    // 设置停止位 
    switch (stopbits)
    {  
        case 1:   
            options.c_cflag &= ~CSTOPB; 
            break; 
        case 2:   
            options.c_cflag |= CSTOPB; 
            break;
        default:   
            syslog(LOG_DEBUG,"[ERR][Serail][%s]Unsupported stop bits",coms.sPortName); 
            return (-1);
    }
   
    //设置等待时间和最小接收字符
    options.c_cc[VTIME] = 0; /* 读取一个字符等待1*(1/10)s */  
    options.c_cc[VMIN] = 1; /* 读取字符的最少个数为1 */
   
    //如果发生数据溢出，接收数据，但是不再读取
    tcflush(fd,TCIFLUSH);
   
    //激活配置 (将修改后的termios数据设置到串口中）
    err=tcsetattr(fd,TCSANOW,&options);
    if (err != 0)  
    {
        syslog(LOG_DEBUG,"[ERR][Serail][%s]Serial Port setup error! code:%d",coms.sPortName,err);  
        return -1; 
    }
    return 0; 
}

/*******************************************************************
* 名称:UART_Init()
* 功能:串口初始化
* 入口参数:
*                      
* 出口参数:正确返回为0，错误返回为-1
*******************************************************************/
int UART_Init()
{
    int i,err;

    //初始化缓存区
    //memset(&coms,0,sizeof(coms));
    //coms.sPortName=portname;
    
    if(strlen(coms.sPortName))
    {
        coms.ifd=UART_Open(coms.sPortName);             //打开串口，返回文件描述符
        err=UART_Set(coms.ifd,coms.iSpeed,0,8,1,'N');   //设置串口参数
        coms.iPort=i+1;                                 //记录串口端口号
        coms.bfree=1;                                   //设置串口处于空闲状态
        if (err==-1)
        {
            syslog(LOG_DEBUG,"[ERR][Serail][%s]Setup failed!",coms.sPortName);
            return -1;
        }
        else
        {
            syslog(LOG_DEBUG,"[INFO][Serail][%s]Timeout=%d",coms.sPortName,coms.itimeout);
            syslog(LOG_DEBUG,"[INFO][Serail][%s]Setup successful!",coms.sPortName);
        }              
    }

    if(com_debug_mode)
        syslog(LOG_DEBUG,"[DEBUG][Serail][%s]Debug mode on",coms.sPortName);
    if(com_test_mode)
        syslog(LOG_DEBUG,"[DEBUG][Serail][%s]Test mode on",coms.sPortName);
    
    return 0;
}
 
/*******************************************************************
* 名称:UART_Recv
* 功能:接收串口数据
* 入口参数: fd:         文件描述符    
*           rcv_buf:    接收串口中数据存入rcv_buf缓冲区中
*           data_len:   一帧数据的长度
* 出口参数:正确返回为收到的数据长度，错误返回为-1
*******************************************************************/
int UART_Recv(int fd,char *rcv_buf,int data_len)
{
    int len,fs_sel;
    fd_set fs_read;
    struct timeval time;
   
    FD_ZERO(&fs_read);
    FD_SET(fd,&fs_read);
   
    time.tv_sec = 0;
    time.tv_usec = 0;
   
    //使用select实现串口的多路通信
    fs_sel = select(fd+1,&fs_read,NULL,NULL,&time);
    if(fs_sel<0)
    {
        syslog(LOG_DEBUG,"[ERR][Serail][%s]Select Error,no=%d",coms.sPortName,fs_sel);
        return -1;
    }
    if(fs_sel)
    {
        len = read(fd,rcv_buf,data_len);
        return len;
    }
    else
    {
        return -1;
    }     
}

/********************************************************************
* 名称:UART_Send
* 功能:发送数据
* 入口参数: fd:         文件描述符    
*           send_buf:   存放串口发送数据
*           data_len:   一帧数据的个数
* 出口参数:正确返回为发送字节数，错误返回为-1
*******************************************************************/
int UART_Send(int fd,char *send_buf,int data_len)
{
    int len = 0;
   
    len = write(fd,send_buf,data_len);
    if (len == data_len )
    {
        return len;
    }     
    else   
    {            
        tcflush(fd,TCOFLUSH);
        return -1;
    } 
}

/*******************************************************************
* 名称:UART_ComDO()
* 功能:串口轮询,并处理收到的信息
* 入口参数:            
* 出口参数:正确返回为0,错误返回为-1,超时返回1
*******************************************************************/
int UART_ComDO()
{
    int i,recv_len,send_len;
    unsigned char recv_buf[500];
    char buf[500];

    if(coms.itimer)                          //超时计数器>0,说明已经发出数据处于等待接受返回的情况下,每次循环都+1
    {
        if(coms.itimer++>coms.itimeout)      //已经超时
        {
            coms.btimeout=1;                 //串口超时标志
            coms.itimer=0;                   //串口超时计数器清零
            //初始化接受缓存,准备接受下一帧数据
            memset(recv_buf,0,sizeof(recv_buf));
            recv_len=0;
            coms.irecv_len=0;
            coms.irecv_no=0;
            coms.irecv_begin=0;
            coms.bfree=1;                   //设置串口处于空闲状态
            return 1;                       //超时返回1
        }           
    }
    recv_len=UART_Recv(coms.ifd,recv_buf,sizeof(recv_buf));
    if(recv_len>0)               //如果收到的数据长度>0,说明有数据收到,开始记录收到的数据
    {
        coms.itimer=0;           //超时计数器清零,不在做超时计时
        coms.btimeout=0;         //超时标志
        coms.irecv_no=0;         //记录没有数据的连续次数,用来判断是否一帧数据接受完成
        coms.irecv_begin=1;      //标记已经开始接受数据
        memcpy(coms.recv_buf+coms.irecv_len,recv_buf,recv_len);     //将接受的数据写入到接受缓存,位置从上次写入的结束位置开始
        coms.irecv_len=coms.irecv_len+recv_len;                     //记录已经接受的数据长度
    }
    else
    {
        if (coms.irecv_begin)     //如果已经开始接受数据
        {
            //如果连续20次没有收到数据说明这一帧数据接受完成
            coms.irecv_no++;
            if (coms.irecv_no>=20)
            {
                //处理已经收到的数据，并准备接受下一帧数据
                if(com_debug_mode)
                    syslog(LOG_DEBUG,"[DEBUG][Serail][%s][RECV]%d Byte",coms.sPortName,coms.irecv_len);
                memcpy(hart_recv_buf,coms.recv_buf,coms.irecv_len);    //复制接的数据帧到Hart接受命令缓存
                hart_recv_len=coms.irecv_len;
                //初始化接受缓存,准备接受下一帧数据
                memset(recv_buf,0,sizeof(recv_buf));
                recv_len=0;
                coms.irecv_len=0;
                coms.irecv_no=0;
                coms.irecv_begin=0;
                coms.bfree=1;           //设置串口处于空闲状态
            }
        }
    }
    //如果有数据需要发送就开始发送
    if(coms.isend_len)
    {
        send_len=UART_Send(coms.ifd,coms.send_buf,coms.isend_len);
        if(send_len==coms.isend_len)     //发送成功的长度等于需要发送的长度,说明发送成功
        {
            if(com_debug_mode)
                syslog(LOG_DEBUG,"[DEBUG][Serail][%s][SEND]%d Byte",coms.sPortName,send_len);                   
        }
        else
        {
            syslog(LOG_DEBUG,"[ERR][Serail][%s]Send data failed! len=%d,need send len=%d",coms.sPortName,send_len,coms.isend_len);
        }
        //发送完成,准备下一次的发送
        memset(coms.send_buf,0,sizeof(coms.send_buf));
        coms.isend_len=0;
        coms.itimer=1;       //超时计数器开始计时
        coms.btimeout=0;     //超时标志清除
        coms.bfree=0;        //串口处于等待返回数据的状态
    }      
    return 0;
}