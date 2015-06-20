#include "HartGW.h"
 
 //读取xml配置文件初始化---------------------------------------------------------------------------------------------------------------
 int HartGW_Init(char *ConfigFileName)
 {
    FILE *fp;
    mxml_node_t *xml_tree;
    mxml_node_t *hart,*node;
    int i;
    char filename[50]="//zqkj//config//";

    memset(hart_recv_buf,0,sizeof(hart_recv_buf));  //清空接受缓存
    memset(&HT,0,sizeof(HT));                       //初始化HT结构

    HT.iCmdIndex=0;
    HT.iSendedIndex=0;
    HT.iInterval=HART_CMDS_INTERVAL;
    HT.iIntervalCount=0;
    hart_run_status_reg=1;
    strcat(filename,ConfigFileName);
    fp=fopen(filename,"r");                                 //打开配置文件
    if(fp==NULL)                                            //打开配置文件失败
    {
        syslog(LOG_DEBUG,"[ERR][HartGW]Open Config file failed![%s]",strerror(errno));
        return 0;
    }
    xml_tree=mxmlLoadFile(NULL,fp,MXML_NO_CALLBACK);        //读取xml文件树
    fclose(fp);  

    //串口通讯参数
    memset(&coms,0,sizeof(coms));
    node=mxmlFindElement(xml_tree,xml_tree,"Port",NULL,NULL,MXML_DESCEND);  //获取Port
    if(node!=NULL)
    {
        strcpy(coms.sPortName,node->child->value.text.string);
    }
    node=mxmlFindElement(xml_tree,xml_tree,"Baud",NULL,NULL,MXML_DESCEND);  //获取Baud
    if(node!=NULL)
    {
        coms.iSpeed=strtoul(node->child->value.text.string,NULL,10);
    }
    node=mxmlFindElement(xml_tree,xml_tree,"Com_Time_Out",NULL,NULL,MXML_DESCEND);  //获取Baud
    if(node!=NULL)
    {
        coms.itimeout=strtoul(node->child->value.text.string,NULL,10);
    }
    node=mxmlFindElement(xml_tree,xml_tree,"Com_Test_Mode",NULL,NULL,MXML_DESCEND);  //Com_Test_Mode
    if(node!=NULL)
    {
        com_test_mode=strtoul(node->child->value.text.string,NULL,10);
    }
    node=mxmlFindElement(xml_tree,xml_tree,"Com_Debug_Mode",NULL,NULL,MXML_DESCEND);  //Com_Debug_Mode
    if(node!=NULL)
    {
        com_debug_mode=strtoul(node->child->value.text.string,NULL,10);
    }

    //Hart参数
    node=mxmlFindElement(xml_tree,xml_tree,"Interval",NULL,NULL,MXML_DESCEND);  //获取Interval node
    if(node!=NULL)
    {
        HT.iInterval=strtoul(node->child->value.text.string,NULL,10);
    }

    node=mxmlFindElement(xml_tree,xml_tree,"RunStatusReg",NULL,NULL,MXML_DESCEND);  //获取RunStatusReg node
    if(node!=NULL)
    {
        hart_run_status_reg=strtoul(node->child->value.text.string,NULL,10);
    }

    node=mxmlFindElement(xml_tree,xml_tree,"Debug_Mode",NULL,NULL,MXML_DESCEND);    //获取Debug_Mode node
    if(node!=NULL)
    {
        hart_debug_mode=strtoul(node->child->value.text.string,NULL,10);
    }

    //hart node
    i=0;
    hart=mxmlFindElement(xml_tree,xml_tree,"hart",NULL,NULL,MXML_DESCEND);  //hart node
    if(hart!=NULL)
    {
        do
        {
            if(mxmlGetElement(hart)!=NULL)    //检查获取的node是否是空node
            {
                node=mxmlFindElement(hart,hart,"cmd_enable",NULL,NULL,MXML_DESCEND);
                if(node!=NULL)
                {
                    HT.HTCmds[i].iEn=strtoul(node->child->value.text.string,NULL,10);
                }   
                node=mxmlFindElement(hart,hart,"hart_id",NULL,NULL,MXML_DESCEND);
                if(node!=NULL)
                {
                    //读取Hart ID
                    sscanf(node->child->value.text.string,"%i,%i,%i",
                            &HT.HTCmds[i].bID[0],
                            &HT.HTCmds[i].bID[1],
                            &HT.HTCmds[i].bID[2]);
                } 
                node=mxmlFindElement(hart,hart,"hart_cmd_no",NULL,NULL,MXML_DESCEND);
                if(node!=NULL)
                {
                    HT.HTCmds[i].bCMD=strtoul(node->child->value.text.string,NULL,10);
                }
                node=mxmlFindElement(hart,hart,"modnet_map_address",NULL,NULL,MXML_DESCEND);
                if(node!=NULL)
                {
                    HT.HTCmds[i].iMapAddress=strtoul(node->child->value.text.string,NULL,10);
                }
                node=mxmlFindElement(hart,hart,"hart_device_type",NULL,NULL,MXML_DESCEND);
                if(node!=NULL)
                {
                    HT.HTCmds[i].iDeviceType=strtoul(node->child->value.text.string,NULL,10);
                }
                node=mxmlFindElement(hart,hart,"hart_hub_port",NULL,NULL,MXML_DESCEND);
                if(node!=NULL)
                {
                    HT.HTCmds[i].iPort=strtoul(node->child->value.text.string,NULL,10);
                }
                if(hart_debug_mode)   //如果在调试模式下,就在syslog中显示每条命令的内容
                {
                    syslog(LOG_DEBUG,"[DEBUG][HartGW]HT_CMD(%d):EN=%d,ID=%X,%X,%X,CMD=%d,MapAddr=%d,Type=%d,HubPort=%d",
                        i+1,
                        HT.HTCmds[i].iEn,
                        HT.HTCmds[i].bID[0],
                        HT.HTCmds[i].bID[1],
                        HT.HTCmds[i].bID[2],
                        HT.HTCmds[i].bCMD,
                        HT.HTCmds[i].iMapAddress,
                        HT.HTCmds[i].iDeviceType,
                        HT.HTCmds[i].iPort);
                }    
                i=i+1;                                      
            }
            hart=mxmlWalkNext(hart,xml_tree,MXML_NO_DESCEND);     //下一个hart node
        }while((hart!=NULL)&&(i<HART_CMDS_NUM_MAX));
        mxmlDelete(hart);
    }
    mxmlDelete(node);
    mxmlDelete(xml_tree);
 } 

/*******************************************************************
* 名称:Hart_CmdCheckCode
* 功能:生成Hart命令的校验码
* 出口参数：正确返回校验码
*******************************************************************/
unsigned char Hart_CmdCheckCode(unsigned char *cmd_buf,int cmd_len)
{
    unsigned char checkcode;
    int i;

    checkcode=0;
    for(i=0;i<cmd_len-1;i++)
    {
        checkcode=checkcode ^ cmd_buf[i];
    }
    return checkcode;
}

//根据需要生成Hart命令并发送===========================================================================================================
unsigned int Hart_Send()
{
    unsigned int i,index;
    
    if(coms.bfree)          //串口处于空闲状态,可以发送命令
    {
        if(HT.iIntervalCount==0) //命令之间的间隔时间到,可以发送下一条命令
        {
            for(i=HT.iCmdIndex;i<HART_CMDS_NUM_MAX;i++)    //从上次发送的命令的下一条命令开始,遍历到最后,找到需要发送的命令
            {
               if(HT.HTCmds[i].iEn)                             //找到需要发送的命令
                {
                    switch (HT.HTCmds[i].bCMD)                  //根据命令号不同的处理
                    {
                        case 1:
                            memcpy(coms.send_buf,hart_cmd_1,strlen(hart_cmd_1));                        //设置命令内容
                            coms.send_buf[8]=HT.HTCmds[i].cID[0];                                       //设置该设备的Hart ID
                            coms.send_buf[9]=HT.HTCmds[i].cID[1];
                            coms.send_buf[10]=HT.HTCmds[i].cID[2];
                            coms.send_buf[13]=Hart_CmdCheckCode(coms.send_buf+5,strlen(hart_cmd_1)-6);  //设置校验码
                            coms.isend_len=strlen(hart_cmd_1);
                            if(hart_debug_mode)
                            {
                                syslog(LOG_DEBUG,"[DEBUG][HartGW][CMD1]Cmds=%d,ID=%d,%d,%d,CheckCode=%d",
                                    i,coms.send_buf[8],coms.send_buf[9],coms.send_buf[10],coms[i].send_buf[13]);
                            }
                            break;
                        case 2:
                            memcpy(coms.send_buf,hart_cmd_2,strlen(hart_cmd_2));                        //设置命令内容
                            coms.send_buf[8]=HT.HTCmds[i].cID[0];                                       //设置该设备的Hart ID
                            coms.send_buf[9]=HT.HTCmds[i].cID[1];
                            coms.send_buf[10]=HT.HTCmds[i].cID[2];
                            coms.send_buf[13]=Hart_CmdCheckCode(coms.send_buf+5,strlen(hart_cmd_2)-6);  //设置校验码
                            coms.isend_len=strlen(hart_cmd_2);
                            if(hart_debug_mode)
                            {
                                syslog(LOG_DEBUG,"[DEBUG][HartGW][CMD2]Cmds=%d,ID=%d,%d,%d,CheckCode=%d",
                                    i,coms.send_buf[8],coms.send_buf[9],coms.send_buf[10],coms[i].send_buf[13]);
                            } 
                            break;                      
                        case 12:
                            memcpy(coms.send_buf,hart_cmd_12,strlen(hart_cmd_12));                      //设置命令内容
                            coms.send_buf[8]=HT.HTCmds[i].cID[0];                                       //设置该设备的Hart ID
                            coms.send_buf[9]=HT.HTCmds[i].cID[1];
                            coms.send_buf[10]=HT.HTCmds[i].cID[2];
                            coms.send_buf[13]=Hart_CmdCheckCode(coms.send_buf+5,strlen(hart_cmd_12)-6); //设置校验码
                            coms.isend_len=strlen(hart_cmd_12);
                            if(hart_debug_mode)
                            {
                                syslog(LOG_DEBUG,"[DEBUG][HartGW][CMD12]Cmds=%d,ID=%d,%d,%d,CheckCode=%d",
                                    i,coms.send_buf[8],coms.send_buf[9],coms.send_buf[10],coms[i].send_buf[13]);
                            }
                            break;
                        case 13:
                            memcpy(coms.send_buf,hart_cmd_13,strlen(hart_cmd_13));                      //设置命令内容
                            coms.send_buf[8]=HT.HTCmds[i].cID[0];                                       //设置该设备的Hart ID
                            coms.send_buf[9]=HT.HTCmds[i].cID[1];
                            coms.send_buf[10]=HT.HTCmds[i].cID[2];
                            coms.send_buf[13]=Hart_CmdCheckCode(coms.send_buf+5,strlen(hart_cmd_13)-6); //设置校验码
                            coms.isend_len=strlen(hart_cmd_13);
                            if(hart_debug_mode)
                            {
                                syslog(LOG_DEBUG,"[DEBUG][HartGW][CMD13]Cmds=%d,ID=%d,%d,%d,CheckCode=%d",
                                    i,coms.send_buf[8],coms.send_buf[9],coms.send_buf[10],coms[i].send_buf[13]);
                            }
                            break;
                        case 14:
                            memcpy(coms.send_buf,hart_cmd_14,strlen(hart_cmd_14));                      //设置命令内容
                            coms.send_buf[8]=HT.HTCmds[i].cID[0];                                       //设置该设备的Hart ID
                            coms.send_buf[9]=HT.HTCmds[i].cID[1];
                            coms.send_buf[10]=HT.HTCmds[i].cID[2];
                            coms.send_buf[13]=Hart_CmdCheckCode(coms.send_buf+5,strlen(hart_cmd_14)-6); //设置校验码
                            coms.isend_len=strlen(hart_cmd_14);
                            if(hart_debug_mode)
                            {
                                syslog(LOG_DEBUG,"[DEBUG][HartGW][CMD14]Cmds=%d,ID=%d,%d,%d,CheckCode=%d",
                                    i,coms.send_buf[8],coms.send_buf[9],coms.send_buf[10],coms[i].send_buf[13]);
                            }
                            break;
                        case 15:
                            memcpy(coms.send_buf,hart_cmd_15,strlen(hart_cmd_15));                      //设置命令内容
                            coms.send_buf[8]=HT.HTCmds[i].cID[0];                                       //设置该设备的Hart ID
                            coms.send_buf[9]=HT.HTCmds[i].cID[1];
                            coms.send_buf[10]=HT.HTCmds[i].cID[2];
                            coms.send_buf[13]=Hart_CmdCheckCode(coms.send_buf+5,strlen(hart_cmd_15)-6); //设置校验码
                            coms.isend_len=strlen(hart_cmd_15);
                            if(hart_debug_mode)
                            {
                                syslog(LOG_DEBUG,"[DEBUG][HartGW][CMD15]Cmds=%d,ID=%d,%d,%d,CheckCode=%d",
                                    i,coms.send_buf[8],coms.send_buf[9],coms.send_buf[10],coms[i].send_buf[13]);
                            }
                            break;
                        case 16: 
                            memcpy(coms.send_buf,hart_cmd_16,strlen(hart_cmd_16));                      //设置命令内容
                            coms.send_buf[8]=HT.HTCmds[i].cID[0];                                       //设置该设备的Hart ID
                            coms.send_buf[9]=HT.HTCmds[i].cID[1];
                            coms.send_buf[10]=HT.HTCmds[i].cID[2];
                            coms.send_buf[13]=Hart_CmdCheckCode(coms.send_buf+5,strlen(hart_cmd_16)-6); //设置校验码
                            coms.isend_len=strlen(hart_cmd_16);
                            if(hart_debug_mode)
                            {
                                syslog(LOG_DEBUG,"[DEBUG][HartGW][CMD16]Cmds=%d,ID=%d,%d,%d,CheckCode=%d",
                                    i,coms.send_buf[8],coms.send_buf[9],coms.send_buf[10],coms[i].send_buf[13]);
                            } 
                            break;              
                    }
                    HT.iSendedIndex=i;                                      //记录本次命令的序号
                    if(i==0)                                                //记录发送第一条命令的开始时间
                    {
                        gettimeofday(&tv_start,NULL);
                    }
                    HT.iCmdIndex=i+1;                                       //记录下次命令的序号
                    if(HT.iCmdIndex>HART_CMDS_NUM_MAX)                      //如果是最后一条命令则下次从第一条命令开始
                    {
                        HT.iCmdIndex=0;
                    }
                    break;                                                  //跳出for循环
                } 
            }   // end "for"
            if(i>=HART_CMDS_NUM_MAX)   //遍历了所有命令都没有找到需要发送的命令,下次就从头开始
            {
                HT.iCmdIndex=0;
                gettimeofday(&tv_end,NULL);                                                     //轮询结束的时间 
                semaphore_p(sem_mb_inputreg_id);                                                //进入临界区
                modnet_inputreg_buf[hart_run_status_reg]=tv_end.tv_sec-tv_start.tv_sec;         //轮询所有命令需要的时间
                semaphore_v(sem_mb_inputreg_id);                                                //退出临界区  
            }            
        }   // end "if"
        else
        {
            HT.iIntervalCount--;     
        }  
    }
}

//处理返回的Hart命令===================================================================================================================
unsigned int Hart_ComDO()
{
    unsigned char in_id[3];                     //返回hart id,3 Byte
    unsigned char in_cmd;                       //返回hart 命令号,1 Byte
    unsigned char in_cc;                        //返回hart 命令的crc16校验值
    unsigned char id[3];                        //已经发出的命令中的id号
    unsigned char cmd;                          //已经发出的命令中的命令号
    unsigned short cc;
    unsigned int mapaddress;                    //已经发出的命令中的映射到modnet的地址
    int i,m,n; 
 
    if(hart_recv_len)     //有接受的数据需要处理
    {
        n=0;
        for (i=0;i<hart_recv_len-3;i++)                           //处理返回命令的0xFF头
        {
            if((hart_recv_buf[i]==0xff)&&(hart_recv_buf[i+1]==0xff)&&(hart_recv_buf[i+2]==0xff))
            {
                for(m=i;m<hart_recv_len;m++)
                {
                    if(hart_recv_buf[m]!=0xff)                  //去除0xff命令头后,找到返回命令的第一个字节
                    {
                        n=m;
                        break;                                  //结束for循环
                    }
                }
                for(m=n;m<hart_recv_buf;m++)
                {
                    hart_recv_buf[m-n]=hart_recv_buf[m];        //去掉命令头,保留命令体
                }
                hart_recv_len=hart_recv_len-n;                  //重新计算命令长度
                break;                                          //结束for循环
            }
        }
        if(n==0)                                                //没有找到命令头
        {
            syslog(LOG_DEBUG,"[ERR][HartGW][%s]No Head!",coms.sPortName);
            hart_recv_len=0;
            memset(hart_recv_buf,0,sizeof(hart_recv_buf));
            HT.iIntervalCount=HT.iInterval;             //设置下一条命令发送的延迟时间
            HT.HTCmds[HT.iSendedIndex].iFailedCount++;  //该条命令通讯失败次数+1
            return 1;            
        }
        id[0]=HT.HTCmds[HT.iSendedIndex].bID[0];                //取得已经发出的命令的ID号
        id[1]=HT.HTCmds[HT.iSendedIndex].bID[1];
        id[2]=HT.HTCmds[HT.iSendedIndex].bID[2];
        cmd=HT.HTCmds[HT.iSendedIndex].bCMD;                    //取得已经发出的命令的命令号
//        m=HT.HTCmds[HT.iSendedIndex].iLen;
        mapaddress=HT.HTCmds[HT.iSendedIndex].iMapAddress-1;    //取得已经发出的命令映射到modnet的地址

        sscanf()
        in_id[0]=hart_recv_buf[3];
        in_id[1]=hart_recv_buf[4];
        in_id[2]=hart_recv_buf[5];
        in_cmd=hart_recv_buf[6];

        if(hart_recv_len<8)                               // 返回的命令太短
        {
            syslog(LOG_DEBUG,"[ERR][HartGW][%s]back too short!",coms.sPortName);
            hart_recv_len=0;
            memset(hart_recv_buf,0,sizeof(hart_recv_buf));
            HT.iIntervalCount=HT.iInterval;             //设置下一条命令发送的延迟时间
            HT.HTCmds[HT.iSendedIndex].iFailedCount++;  //该条命令通讯失败次数+1
            return 1;
        }
        in_cc=hart_recv_buf[13];                            // 收到命令的checkcode值
        cc=Hart_CmdCheckCode(hart_recv_buf,hart_recv_len-2);// 计算收到命令的crc16值
        if(in_cc!=cc)                                       //校验是否正确
        {
            syslog(LOG_DEBUG,"[ERR][HartGW][%s]CheckCode Err!(No.=%d,len=%d,0x%02X,0x%02X)",
                    coms.sPortName,HT.iSendedIndex+1,hart_recv_len,in_cc,cc);
            hart_recv_len=0;
            memset(hart_recv_buf,0,sizeof(hart_recv_buf));
            HT.iIntervalCount=HT.iInterval;             //设置下一条命令发送的延迟时间
            HT.HTCmds[HT.iSendedIndex].iFailedCount++;  //该条命令通讯失败次数+1
            return 1;
        }
        if((id[0]!=in_id[0])||(id[1]!=in_id[1])||(id[2]!=in_id[2]))     // Hart ID不符
        {
            syslog(LOG_DEBUG,"[ERR][HartGW][%s]Hart ID Err!(No.=%d)",coms.sPortName,HT.iSendedIndex);
            hart_recv_len=0;
            memset(hart_recv_buf,0,sizeof(hart_recv_buf));
            HT.iIntervalCount=HT.iInterval;                             //设置下一条命令发送的延迟时间
            HT.HTCmds[HT.iSendedIndex].iFailedCount++;                  //该条命令通讯失败次数+1
            return 1;
        }
        if(cmd!=in_cmd)                                 // Hart命令号不符
        {
            syslog(LOG_DEBUG,"[ERR][HartGW][%s]Hart command Err!(No.=%d,%d,%d)",coms.sPortName,HT.iSendedIndex,cmd,in_cmd);
            hart_recv_len=0;
            memset(hart_recv_buf,0,sizeof(hart_recv_buf));
            HT.iIntervalCount=HT.iInterval;             //设置下一条命令发送的延迟时间
            HT.HTCmds[HT.iSendedIndex].iFailedCount++;  //该条命令通讯失败次数+1
            return 1;
        }

        switch(cmd)
        {
            case 1:
                semaphore_p(sem_mb_coilst_id);                              //进入临界区

                semaphore_v(sem_mb_coilst_id);                              //退出临界区
                break;
            case 2:
                semaphore_p(sem_mb_inputst_id);                             //进入临界区

                semaphore_v(sem_mb_inputst_id);                             //退出临界区
                break;
            case 3:
                semaphore_p(sem_mb_holdreg_id);                             //进入临界区
                for(i=0;i<m;i++)
                {
                    modnet_holdreg_buf[mapaddress+i]=hart_recv_buf[i*2+3]*0x100+hart_recv_buf[i*2+3+1];
                }                                                           //将返回的结果放入modnet存储区
                semaphore_v(sem_mb_holdreg_id);                             //退出临界区
                if(hart_debug_mode)
                {
                    syslog(LOG_DEBUG,"[DEBUG][HartGW][%s]Write %d Byte to modnet holding reg",coms.sPortName,m*2);
                }
                break;
            case 4:
                semaphore_p(sem_mb_inputreg_id);                             //进入临界区
                for(i=0;i<m;i++)
                {
                    modnet_inputreg_buf[mapaddress+i]=hart_recv_buf[i*2+3]*0x100+hart_recv_buf[i*2+3+1];
                }                                                            //将返回的结果放入modnet存储区
                semaphore_v(sem_mb_inputreg_id);                             //退出临界区
                if(hart_debug_mode)
                {
                    syslog(LOG_DEBUG,"[DEBUG][HartGW][%s]Write %d Byte to modnet input reg",coms.sPortName,m*2);
                }
                break;
        }
        HT.iIntervalCount=HT.iInterval;                 //收到返回信息,设置下一条命令发送的延迟时间
        memset(hart_recv_buf,0,sizeof(hart_recv_buf));  //清空接受区,准备接受下一次数据
        hart_recv_len=0;
        HT.HTCmds[HT.iSendedIndex].iSuccCount++;        //该条命令通讯成功次数+1
    }
    return 0;
}

//将每条命令的通讯情况写入到指定的Modnet Input Reg中============================================================================
void Hart_ComStatus()
{
    int i;

    semaphore_p(sem_mb_inputreg_id);        //进入临界区
    for(i=0;i<Hart_CMDS_NUM_MAX;i++)   //遍历每一条命令
    {
        if(HT.HTCmds[i].iEn)
        {
            modnet_inputreg_buf[mb_run_status_reg+10+i*3]=HT.HTCmds[i].iSuccCount;       //通讯成功的次数
            modnet_inputreg_buf[mb_run_status_reg+10+i*3+1]=HT.HTCmds[i].iFailedCount;   //通讯失败的次数
            modnet_inputreg_buf[mb_run_status_reg+10+i*3+2]=HT.HTCmds[i].iTimeoutCount;  //通讯超时的次数
        }
    }
    semaphore_v(sem_mb_inputreg_id);        //退出临界区
}

//-----------------------------------------------Main Program---------------------------------------------------------------------------
int main(int argc, char **argv)
{   
    int i;

    if(argc<2)
    {
        printf("[USER]:HartGW ConfigFileName\n");
        exit(0);
    }  
    //采用守护进程运行模式
    daemon(0,0);

    //打开syslog以便后续可以写入日志内容，系统日志保存在/var/log/message中
    openlog("[HartGW]",LOG_CONS|LOG_PID,LOG_USER);
    syslog(LOG_DEBUG,"[INFO][HartGW]================================Hart Gateway server start.===============================");
    //初始化信号量
    sem_Init();
    //初始化共享内存
    shm_Init();
    modnet_holdreg_buf=(uint16_t *)shm_mb_holdreg_buf;
    modnet_inputreg_buf=(uint16_t *)shm_mb_inputreg_buf;
    modnet_coilst_buf=(unsigned char *)shm_mb_coilst_buf;
    modnet_inputst_buf=(unsigned char *)shm_mb_inputst_buf;

    //读取配置文件初始化系统
    HartGW_Init(argv[1]);
    //初始化串口通讯
    UART_Init();

    while (1)   //循环读取数据
    {  
        //Hart命令发送处理
        Hart_Send();
        //串口服务处理
        if(UART_ComDO()==1)     //串口服务处理,包括处理需要发送的数据,接收到需要处理的数据,如果超时返回1
        {
            HT.HTCmds[HT.iSendedIndex].iTimeoutCount++;   //超时错误计数器+1
        } 
        //Hart接收命令处理
        Hart_ComDO();
        //如果有需要就将通讯状态写入到Modnet input reg中,如果hart_run_status_reg=0就不写入
        if(hart_run_status_reg)
            Hart_ComStatus();
        //延迟1ms,然后进行下一次处理,经过测试证明usleep(1)暂停的时间是5us
        usleep(200);
    }
    // 关闭串口 
    UART_Close();           
}