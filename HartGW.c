#include "HartGW.h"
 
 //读取xml配置文件初始化==============================================================================================================
 int HartGW_Init(char *ConfigFileName)
 {
    FILE *fp;
    mxml_node_t *xml_tree;
    mxml_node_t *hart,*node;
    int i,t1,t2,t3,t4,t5,t6;
    char filename[50]="//zqkj//config//";

    memset(hart_recv_buf,0,sizeof(hart_recv_buf));  //清空接受缓存
    memset(&HT,0,sizeof(HT));                       //初始化HT结构
    memset(&HD,0,sizeof(HD));                       //初始化HD结构

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
    node=mxmlFindElement(xml_tree,xml_tree,"Com_Time_Out",NULL,NULL,MXML_DESCEND);  //Com_Time_Out
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
                    sscanf(node->child->value.text.string,"%X,%X,%X,%X,%X",&t1,&t2,&t3,&t4,&t5);
                    HT.HTCmds[i].bID[0]=t1;
                    HT.HTCmds[i].bID[1]=t2;
                    HT.HTCmds[i].bID[2]=t3;
                    HT.HTCmds[i].bID[3]=t4;
                    HT.HTCmds[i].bID[4]=t5;

                } 
                node=mxmlFindElement(hart,hart,"hart_tag",NULL,NULL,MXML_DESCEND);
                if(node!=NULL)
                {
                    //读取Hart ID
                    sscanf(node->child->value.text.string,"%X,%X,%X,%X,%X,%X",&t1,&t2,&t3,&t4,&t5,&t6);
                    HT.HTCmds[i].bTag[0]=t1;
                    HT.HTCmds[i].bTag[1]=t2;
                    HT.HTCmds[i].bTag[2]=t3;
                    HT.HTCmds[i].bTag[3]=t4;
                    HT.HTCmds[i].bTag[4]=t5;
                    HT.HTCmds[i].bTag[5]=t6;
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
                    syslog(LOG_DEBUG,"[DEBUG][HartGW][CONFIG]Index=%d#,EN=%d,ID=0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,CMD=%d,MapAddr=%d,Type=%d,HubPort=%d",
                        i+1,
                        HT.HTCmds[i].iEn,
                        HT.HTCmds[i].bID[0],
                        HT.HTCmds[i].bID[1],
                        HT.HTCmds[i].bID[2],
                        HT.HTCmds[i].bID[3],
                        HT.HTCmds[i].bID[4],
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

    return 0;
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
    for(i=0;i<cmd_len;i++)
    {
        checkcode=checkcode ^ cmd_buf[i];
    }
    return checkcode;
}

//根据需要生成Hart命令并发送===========================================================================================================
unsigned int Hart_Send()
{
    unsigned int i,index;
    
    if(coms.bfree)               //串口处于空闲状态,可以发送命令
    {
        if((HT.iIntervalCount==0)||(coms.btimeout))        //命令之间的间隔时间到或者串口超时,可以发送下一条命令
        {
            HT.iIntervalCount=HT.iInterval;
            for(i=HT.iCmdIndex;i<HART_CMDS_NUM_MAX;i++)    //从上次发送的命令的下一条命令开始,遍历到最后,找到需要发送的命令
            {
               if(HT.HTCmds[i].iEn)                             //找到需要发送的命令
                {
                    switch (HT.HTCmds[i].bCMD)                  //根据命令号不同的处理
                    {
                        case 1:
                            memcpy(coms.send_buf,hart_cmd_1,sizeof(hart_cmd_1));                        //设置命令内容
                            coms.send_buf[6]=HT.HTCmds[i].bID[0];                                       //设置该设备的Hart ID
                            coms.send_buf[7]=HT.HTCmds[i].bID[1];
                            coms.send_buf[8]=HT.HTCmds[i].bID[2];
                            coms.send_buf[9]=HT.HTCmds[i].bID[3];
                            coms.send_buf[10]=HT.HTCmds[i].bID[4];
                            coms.send_buf[13]=Hart_CmdCheckCode(coms.send_buf+5,sizeof(hart_cmd_1)-6);  //设置校验码
                            coms.isend_len=sizeof(hart_cmd_1);
                            if(hart_debug_mode)
                            {
                                syslog(LOG_DEBUG,"[DEBUG][HartGW][SEND_CMD=1][%s]Index=%d#,ID=0x%02X,0x%02X,0x%02X,CheckCode=0x%02X,len=%d",
                                    coms.sPortName,i+1,coms.send_buf[8],coms.send_buf[9],coms.send_buf[10],coms.send_buf[13],coms.isend_len);
                            }
                            break;
                        case 2:
                            memcpy(coms.send_buf,hart_cmd_2,sizeof(hart_cmd_2));                        //设置命令内容
                            coms.send_buf[6]=HT.HTCmds[i].bID[0];                                       //设置该设备的Hart ID
                            coms.send_buf[7]=HT.HTCmds[i].bID[1];
                            coms.send_buf[8]=HT.HTCmds[i].bID[2];
                            coms.send_buf[9]=HT.HTCmds[i].bID[3];
                            coms.send_buf[10]=HT.HTCmds[i].bID[4];
                            coms.send_buf[13]=Hart_CmdCheckCode(coms.send_buf+5,sizeof(hart_cmd_2)-6);  //设置校验码
                            coms.isend_len=sizeof(hart_cmd_2);
                            if(hart_debug_mode)
                            {
                                syslog(LOG_DEBUG,"[DEBUG][HartGW][SEND_CMD=2][%s]Index=%d#,ID=0x%02X,0x%02X,0x%02X,CheckCode=0x%02X,len=%d",
                                    coms.sPortName,i+1,coms.send_buf[8],coms.send_buf[9],coms.send_buf[10],coms.send_buf[13],coms.isend_len);
                            } 
                            break;
                        case 11:
                            memcpy(coms.send_buf,hart_cmd_11,sizeof(hart_cmd_11));                      //设置命令内容
                            coms.send_buf[6]=HT.HTCmds[i].bID[0];                                       //设置该设备的Hart ID
                            coms.send_buf[7]=HT.HTCmds[i].bID[1];
                            coms.send_buf[8]=HT.HTCmds[i].bID[2];
                            coms.send_buf[9]=HT.HTCmds[i].bID[3];
                            coms.send_buf[10]=HT.HTCmds[i].bID[4];
                            coms.send_buf[13]=HT.HTCmds[i].bTag[0];										//设置改设备的Tag
                            coms.send_buf[14]=HT.HTCmds[i].bTag[1];
                            coms.send_buf[15]=HT.HTCmds[i].bTag[2];
                            coms.send_buf[16]=HT.HTCmds[i].bTag[3];
                            coms.send_buf[17]=HT.HTCmds[i].bTag[4];
                            coms.send_buf[18]=HT.HTCmds[i].bTag[5];
                            coms.send_buf[19]=Hart_CmdCheckCode(coms.send_buf+5,sizeof(hart_cmd_11)-6); //设置校验码
                            coms.isend_len=sizeof(hart_cmd_11);
                            if(hart_debug_mode)
                            {
                                syslog(LOG_DEBUG,"[DEBUG][HartGW][SEND_CMD=11][%s]Index=%d#,ID=0x%02X,0x%02X,0x%02X,CheckCode=0x%02X,len=%d",
                                    coms.sPortName,i+1,coms.send_buf[8],coms.send_buf[9],coms.send_buf[10],coms.send_buf[13],coms.isend_len);
                                syslog(LOG_DEBUG,"[DEBUG][HartGW][SEND_CMD=11][%s]Index=%d#,Tag=0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X",
                                	coms.sPortName,i+1,coms.send_buf[13],coms.send_buf[14],coms.send_buf[15],coms.send_buf[16],coms.send_buf[17],coms.send_buf[18]);
                            }
                            break;                    
                        case 12:
                            memcpy(coms.send_buf,hart_cmd_12,sizeof(hart_cmd_12));                      //设置命令内容
                            coms.send_buf[6]=HT.HTCmds[i].bID[0];                                       //设置该设备的Hart ID
                            coms.send_buf[7]=HT.HTCmds[i].bID[1];
                            coms.send_buf[8]=HT.HTCmds[i].bID[2];
                            coms.send_buf[9]=HT.HTCmds[i].bID[3];
                            coms.send_buf[10]=HT.HTCmds[i].bID[4];
                            coms.send_buf[13]=Hart_CmdCheckCode(coms.send_buf+5,sizeof(hart_cmd_12)-6); //设置校验码
                            coms.isend_len=sizeof(hart_cmd_12);
                            if(hart_debug_mode)
                            {
                                syslog(LOG_DEBUG,"[DEBUG][HartGW][SEND_CMD=12][%s]Index=%d#,ID=0x%02X,0x%02X,0x%02X,CheckCode=0x%02X,len=%d",
                                    coms.sPortName,i+1,coms.send_buf[8],coms.send_buf[9],coms.send_buf[10],coms.send_buf[13],coms.isend_len);
                            }
                            break;
                        case 13:
                            memcpy(coms.send_buf,hart_cmd_13,sizeof(hart_cmd_13));                      //设置命令内容
                            coms.send_buf[6]=HT.HTCmds[i].bID[0];                                       //设置该设备的Hart ID
                            coms.send_buf[7]=HT.HTCmds[i].bID[1];
                            coms.send_buf[8]=HT.HTCmds[i].bID[2];
                            coms.send_buf[9]=HT.HTCmds[i].bID[3];
                            coms.send_buf[10]=HT.HTCmds[i].bID[4];
                            coms.send_buf[13]=Hart_CmdCheckCode(coms.send_buf+5,sizeof(hart_cmd_13)-6); //设置校验码
                            coms.isend_len=sizeof(hart_cmd_13);
                            if(hart_debug_mode)
                            {
                                syslog(LOG_DEBUG,"[DEBUG][HartGW][SEND_CMD=13][%s]Index=%d#,ID=0x%02X,0x%02X,0x%02X,CheckCode=0x%02X,len=%d",
                                    coms.sPortName,i+1,coms.send_buf[8],coms.send_buf[9],coms.send_buf[10],coms.send_buf[13],coms.isend_len);
                            }
                            break;
                        case 14:
                            memcpy(coms.send_buf,hart_cmd_14,sizeof(hart_cmd_14));                      //设置命令内容
                            coms.send_buf[6]=HT.HTCmds[i].bID[0];                                       //设置该设备的Hart ID
                            coms.send_buf[7]=HT.HTCmds[i].bID[1];
                            coms.send_buf[8]=HT.HTCmds[i].bID[2];
                            coms.send_buf[9]=HT.HTCmds[i].bID[3];
                            coms.send_buf[10]=HT.HTCmds[i].bID[4];
                            coms.send_buf[13]=Hart_CmdCheckCode(coms.send_buf+5,sizeof(hart_cmd_14)-6); //设置校验码
                            coms.isend_len=sizeof(hart_cmd_14);
                            if(hart_debug_mode)
                            {
                                syslog(LOG_DEBUG,"[DEBUG][HartGW][SEND_CMD=14][%s]Index=%d#,ID=0x%02X,0x%02X,0x%02X,CheckCode=0x%02X,len=%d",
                                    coms.sPortName,i+1,coms.send_buf[8],coms.send_buf[9],coms.send_buf[10],coms.send_buf[13],coms.isend_len);
                            }
                            break;
                        case 15:
                            memcpy(coms.send_buf,hart_cmd_15,sizeof(hart_cmd_15));                      //设置命令内容
                            coms.send_buf[6]=HT.HTCmds[i].bID[0];                                       //设置该设备的Hart ID
                            coms.send_buf[7]=HT.HTCmds[i].bID[1];
                            coms.send_buf[8]=HT.HTCmds[i].bID[2];
                            coms.send_buf[9]=HT.HTCmds[i].bID[3];
                            coms.send_buf[10]=HT.HTCmds[i].bID[4];
                            coms.send_buf[13]=Hart_CmdCheckCode(coms.send_buf+5,sizeof(hart_cmd_15)-6); //设置校验码
                            coms.isend_len=sizeof(hart_cmd_15);
                            if(hart_debug_mode)
                            {
                                syslog(LOG_DEBUG,"[DEBUG][HartGW][SEND_CMD=15][%s]Index=%d#,ID=0x%02X,0x%02X,0x%02X,CheckCode=0x%02X,len=%d",
                                    coms.sPortName,i+1,coms.send_buf[8],coms.send_buf[9],coms.send_buf[10],coms.send_buf[13],coms.isend_len);
                            }
                            break;
                        case 16: 
                            memcpy(coms.send_buf,hart_cmd_16,sizeof(hart_cmd_16));                      //设置命令内容
                            coms.send_buf[6]=HT.HTCmds[i].bID[0];                                       //设置该设备的Hart ID
                            coms.send_buf[7]=HT.HTCmds[i].bID[1];
                            coms.send_buf[8]=HT.HTCmds[i].bID[2];
                            coms.send_buf[9]=HT.HTCmds[i].bID[3];
                            coms.send_buf[10]=HT.HTCmds[i].bID[4];
                            coms.send_buf[13]=Hart_CmdCheckCode(coms.send_buf+5,sizeof(hart_cmd_16)-6); //设置校验码
                            coms.isend_len=sizeof(hart_cmd_16);
                            if(hart_debug_mode)
                            {
                                syslog(LOG_DEBUG,"[DEBUG][HartGW][SEND_CMD=16][%s]Index=%d#,ID=0x%02X,0x%02X,0x%02X,CheckCode=0x%02X,len=%d",
                                    coms.sPortName,i+1,coms.send_buf[8],coms.send_buf[9],coms.send_buf[10],coms.send_buf[13],coms.isend_len);
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
    unsigned char in_id[5];                     //返回hart id,5 Byte
    unsigned char in_cmd;                       //返回hart 命令号,1 Byte
    unsigned char in_len;                       //返回hart 数据长度,1 Byte
    unsigned char in_cc;                        //返回hart 命令的校验值
    unsigned char id[5];                        //已经发出的命令中的id号
    unsigned char cmd;                          //已经发出的命令中的命令号
    unsigned char cc;
    unsigned int mapaddress;                    //已经发出的命令中的映射到modnet的地址
    int i,m,n; 
    char s1[]="0x00,",s2[200];
 
    if(hart_recv_len)     //有接受的数据需要处理
    {
        //处理返回命令的前导(0xff)头
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
                for(m=n;m<hart_recv_len;m++)
                {
                    hart_recv_buf[m-n]=hart_recv_buf[m];        //去掉命令头,保留命令体
                }
                hart_recv_len=hart_recv_len-n;                  //重新计算命令长度
                break;                                          //结束for循环
            }
        }
        if(n==0)                                                //没有找到命令头
        {
            syslog(LOG_DEBUG,"[ERR][HartGW][RECV][%s]Index=%d#,No Head!",coms.sPortName,HT.iSendedIndex+1);
            hart_recv_len=0;
            memset(hart_recv_buf,0,sizeof(hart_recv_buf));
            HT.iIntervalCount=HT.iInterval;                     //设置下一条命令发送的延迟时间
            HT.HTCmds[HT.iSendedIndex].iFailedCount++;          //该条命令通讯失败次数+1
            return 1;            
        }
        //处理前导(0xff)头完成
        if(hart_recv_len<8)                                     // 返回的命令太短
        {
            syslog(LOG_DEBUG,"[ERR][HartGW][RECV][%s]Index=%d#,back too short!",coms.sPortName,HT.iSendedIndex+1);
            hart_recv_len=0;
            memset(hart_recv_buf,0,sizeof(hart_recv_buf));
            HT.iIntervalCount=HT.iInterval;                     //设置下一条命令发送的延迟时间
            HT.HTCmds[HT.iSendedIndex].iFailedCount++;          //该条命令通讯失败次数+1
            return 1;
        }        
        hart_recv_len=hart_recv_buf[7]+9;                       //根据收到命令的数据长度来修正收到命令的长度,如果有多余的字节被抛弃
        id[0]=HT.HTCmds[HT.iSendedIndex].bID[0];                //取得已经发出的命令的ID号
        id[1]=HT.HTCmds[HT.iSendedIndex].bID[1];
        id[2]=HT.HTCmds[HT.iSendedIndex].bID[2];
        id[3]=HT.HTCmds[HT.iSendedIndex].bID[3];
        id[4]=HT.HTCmds[HT.iSendedIndex].bID[4];
        cmd=HT.HTCmds[HT.iSendedIndex].bCMD;                    //取得已经发出的命令的命令号
        mapaddress=HT.HTCmds[HT.iSendedIndex].iMapAddress-1;    //取得已经发出的命令映射到modnet的地址
        in_id[0]=hart_recv_buf[1];                              //取得返回的命令的Hart ID
        in_id[1]=hart_recv_buf[2];
        in_id[2]=hart_recv_buf[3];
        in_id[3]=hart_recv_buf[4];
        in_id[4]=hart_recv_buf[5];                              
        in_cmd=hart_recv_buf[6];                                //取得返回命令的命令号
        in_len=hart_recv_buf[7];                                //取得返回命令的数据长度
        in_cc=hart_recv_buf[hart_recv_len-1];                   //收到命令的checkcode值
        cc=Hart_CmdCheckCode(hart_recv_buf,hart_recv_len-1);    //计算收到命令的checkcode值
        if(in_cc!=cc)                                           //校验是否正确
        {
            syslog(LOG_DEBUG,"[ERR][HartGW][RECV][%s]Index=%d#,CheckCode Err!(len=%d,head=0x%02X,0x%02X,0x%02X,CheckCode=0x%02X,0x%02X)",
                    coms.sPortName,HT.iSendedIndex+1,hart_recv_len,hart_recv_buf[0],hart_recv_buf[1],hart_recv_buf[2],in_cc,cc);
            memset(s2,0,sizeof(s2));
            for(i=0;i<hart_recv_len;i++)
            {
                sprintf(s2+i*5,"0x%02X,",hart_recv_buf[i]);
            }
            syslog(LOG_DEBUG,"[DEBUG][HartGW][RECV]%s",s2);
            hart_recv_len=0;
            memset(hart_recv_buf,0,sizeof(hart_recv_buf));
            HT.iIntervalCount=HT.iInterval;                     //设置下一条命令发送的延迟时间
            HT.HTCmds[HT.iSendedIndex].iFailedCount++;          //该条命令通讯失败次数+1
            return 1;
        }

        if(!(hart_recv_buf[0]&0x80))                            //返回的命令不是针对第1个主机
        {
            if(hart_debug_mode)
            {
                syslog(LOG_DEBUG,"[DEBUG][HartGW][RECV][%s]Index=%d#,Respon not for me!",coms.sPortName,HT.iSendedIndex+1);
            }
            hart_recv_len=0;
            memset(hart_recv_buf,0,sizeof(hart_recv_buf));
            HT.iIntervalCount=HT.iInterval;                     //设置下一条命令发送的延迟时间
            HT.HTCmds[HT.iSendedIndex].iFailedCount++;          //该条命令通讯失败次数+1
            return 1;
        }

        if(hart_recv_buf[0]&0x40)                               //处于Burset模式的返回
        {
            if(hart_debug_mode)
            {
                syslog(LOG_DEBUG,"[DEBUG][HartGW][RECV][%s]Index=%d#,Respon for Burset mode!",coms.sPortName,HT.iSendedIndex+1);
            }
            hart_recv_len=0;
            memset(hart_recv_buf,0,sizeof(hart_recv_buf));
            HT.iIntervalCount=HT.iInterval;                     //设置下一条命令发送的延迟时间
            HT.HTCmds[HT.iSendedIndex].iFailedCount++;          //该条命令通讯失败次数+1
            return 1;
        }

        if((id[0]!=in_id[0])||(id[1]!=in_id[1])||(id[2]!=in_id[2])||(id[3]!=in_id[3])||(id[4]!=in_id[4]))     // Hart ID不符
        {
            syslog(LOG_DEBUG,"[ERR][HartGW][RECV][%s]Index=%d#,Hart ID Err!(0x%02X,0x%02X,0x%02X)(0x%02X,0x%02X,0x%02X)",
                    coms.sPortName,HT.iSendedIndex+1,id[2],id[3],id[4],in_id[2],in_id[3],in_id[4]);
            hart_recv_len=0;
            memset(hart_recv_buf,0,sizeof(hart_recv_buf));
            HT.iIntervalCount=HT.iInterval;                     //设置下一条命令发送的延迟时间
            HT.HTCmds[HT.iSendedIndex].iFailedCount++;          //该条命令通讯失败次数+1
            return 1;
        }

        if(cmd!=in_cmd)                                         // Hart命令号不符
        {
            syslog(LOG_DEBUG,"[ERR][HartGW][RECV][%s]Index=%d#,Hart command Err!(%d,%d)",coms.sPortName,HT.iSendedIndex+1,cmd,in_cmd);
            hart_recv_len=0;
            memset(hart_recv_buf,0,sizeof(hart_recv_buf));
            HT.iIntervalCount=HT.iInterval;                     //设置下一条命令发送的延迟时间
            HT.HTCmds[HT.iSendedIndex].iFailedCount++;          //该条命令通讯失败次数+1
            return 1;
        }

        switch(cmd)
        {
            case 1:                                                           //读取PV单位(1Byte)和值(浮点,4Byte),需要5个reg
                semaphore_p(sem_mb_inputreg_id);                              //进入临界区
                    modnet_inputreg_buf[mapaddress]=hart_recv_buf[8]*0x100+hart_recv_buf[9];        //保存响应码
                    modnet_inputreg_buf[mapaddress+1]=in_len;                //保存返回命令的数据长度
                    modnet_inputreg_buf[mapaddress+2]=hart_recv_buf[10];     //PV的单位
                    modnet_inputreg_buf[mapaddress+3]=hart_recv_buf[11]*0x100+hart_recv_buf[12];    //PV值,浮点4Byte
                    modnet_inputreg_buf[mapaddress+4]=hart_recv_buf[13]*0x100+hart_recv_buf[14];
                semaphore_v(sem_mb_inputreg_id);                              //退出临界区
                if(hart_debug_mode)
                {
                    syslog(LOG_DEBUG,"[DEBUG][HartGW][RECV_CMD=1][%s]Write to InputReg=%d",coms.sPortName,mapaddress+1);
                }               
                break;
            case 2:                                                          //读取PV电流和百分比,需要6个Reg
                semaphore_p(sem_mb_inputreg_id);                             //进入临界区
                    modnet_inputreg_buf[mapaddress]=hart_recv_buf[8]*0x100+hart_recv_buf[9];        //保存响应码
                    modnet_inputreg_buf[mapaddress+1]=in_len;                //保存返回命令的数据长度
                    modnet_inputreg_buf[mapaddress+2]=hart_recv_buf[10]*0x100+hart_recv_buf[11];    //PV的电流,浮点4Byte
                    modnet_inputreg_buf[mapaddress+3]=hart_recv_buf[12]*0x100+hart_recv_buf[13];    
                    modnet_inputreg_buf[mapaddress+4]=hart_recv_buf[14]*0x100+hart_recv_buf[15];    //PV值,浮点4Byte
                    modnet_inputreg_buf[mapaddress+5]=hart_recv_buf[16]*0x100+hart_recv_buf[17];
                semaphore_v(sem_mb_inputreg_id);                             //退出临界区 
                if(hart_debug_mode)
                {
                    syslog(LOG_DEBUG,"[DEBUG][HartGW][RECV_CMD=2][%s]Write to InputReg=%d",coms.sPortName,mapaddress+1);
                }
                break;
            case 11:                                                        //通过设备Tag读取制造商ID(1Byte),设备类型(1Byte),请求的前导符个数(1Byte)
            																//Hart版本(1Byte),设备版本(1Byte),设备软件版本(1Byte),设备硬件版本(1Byte)
            																//设备标志(1Byte),设备ID号(3Byte),响应的最小前导符个数(1Byte),
                                                                            //设备变量的最大个数(1Byte),配置修改次数(2Byte),附加设备状态(1Byte),
                                                                            //需要16reg
                semaphore_p(sem_mb_inputreg_id);                            //进入临界区
                    modnet_inputreg_buf[mapaddress]=hart_recv_buf[8]*0x100+hart_recv_buf[9];        //保存响应码
                    modnet_inputreg_buf[mapaddress+1]=in_len;               //保存返回命令的数据长度
                    modnet_inputreg_buf[mapaddress+2]=hart_recv_buf[11];    //制造商ID,1Byte
                    modnet_inputreg_buf[mapaddress+3]=hart_recv_buf[12];	//设备类型(1Byte) 
                    modnet_inputreg_buf[mapaddress+4]=hart_recv_buf[13];    //请求的前导符个数(1Byte)
                    modnet_inputreg_buf[mapaddress+5]=hart_recv_buf[14];	//Hart版本(1Byte)
                    modnet_inputreg_buf[mapaddress+6]=hart_recv_buf[15];	//设备版本(1Byte)
                    modnet_inputreg_buf[mapaddress+7]=hart_recv_buf[16];	//设备软件版本(1Byte)
                    modnet_inputreg_buf[mapaddress+8]=hart_recv_buf[17];	//设备硬件版本(1Byte)
                    modnet_inputreg_buf[mapaddress+9]=hart_recv_buf[18];	//设备标志(1Byte)
                    modnet_inputreg_buf[mapaddress+10]=hart_recv_buf[19];	//设备ID号(3Byte)
                    modnet_inputreg_buf[mapaddress+11]=hart_recv_buf[20]*0x100+hart_recv_buf[21]; 
                    modnet_inputreg_buf[mapaddress+12]=hart_recv_buf[22];   //响应的最小前导符个数
                    modnet_inputreg_buf[mapaddress+13]=hart_recv_buf[23];   //设备变量的最大个数
                    modnet_inputreg_buf[mapaddress+14]=hart_recv_buf[24]*0x100+hart_recv_buf[25];   //配置修改次数
                    modnet_inputreg_buf[mapaddress+15]=hart_recv_buf[26];   //附加设备状态              
                semaphore_v(sem_mb_inputreg_id);                            //退出临界区 
                if(hart_debug_mode)
                {
                    syslog(LOG_DEBUG,"[DEBUG][HartGW][RECV_CMD=11][%s]Write to InputReg=%d",coms.sPortName,mapaddress+1);
                }
                break;
            case 12:                                                         //读message,24Byte,需要14reg
                semaphore_p(sem_mb_inputreg_id);                             //进入临界区
                    modnet_inputreg_buf[mapaddress]=hart_recv_buf[8]*0x100+hart_recv_buf[9];        //保存响应码
                    modnet_inputreg_buf[mapaddress+1]=in_len;                //保存返回命令的数据长度                   
                    for(i=0;i<12;i++)                                        //需要12个reg,message 12个
                    {
                        modnet_inputreg_buf[mapaddress+2+i]=hart_recv_buf[i*2+10]*0x100+hart_recv_buf[i*2+11];
                    }                                                        //将返回的结果放入modnet存储区
                semaphore_v(sem_mb_inputreg_id);                             //退出临界区
                if(hart_debug_mode)
                {
                    syslog(LOG_DEBUG,"[DEBUG][HartGW][RECV_CMD=12][%s]Write to InputReg=%d",coms.sPortName,mapaddress+1);
                }
                break;
            case 13:                                                         //读取标签(6Byte),描述符(12Byte),日期(3Byte),需要13reg
                semaphore_p(sem_mb_inputreg_id);                             //进入临界区
                    modnet_inputreg_buf[mapaddress]=hart_recv_buf[8]*0x100+hart_recv_buf[9];        //保存响应码
                    modnet_inputreg_buf[mapaddress+1]=in_len;                //保存返回命令的数据长度
                    for(i=0;i<9;i++)                                         //需要9个reg,保存标签3个(6Byte),描述符6个(12Byte)
                    {
                        modnet_inputreg_buf[mapaddress+2+i]=hart_recv_buf[i*2+10]*0x100+hart_recv_buf[i*2+11];
                    }                                                            
                    modnet_inputreg_buf[mapaddress+11]=hart_recv_buf[28]*0x100+hart_recv_buf[29];   //保存日期-日,月
                    modnet_inputreg_buf[mapaddress+12]=hart_recv_buf[30];                           //保存日期-年
                semaphore_v(sem_mb_inputreg_id);                             //退出临界区
                if(hart_debug_mode)
                {
                    syslog(LOG_DEBUG,"[DEBUG][HartGW][RECV_CMD=13][%s]Write to InputReg=%d",coms.sPortName,mapaddress+1);
                }
                break;
            case 14:                                                            //读PV传感器序列号,上下限,单位精度代码,需要11个Reg
                semaphore_p(sem_mb_inputreg_id);                                //进入临界区
                    modnet_inputreg_buf[mapaddress]=hart_recv_buf[8]*0x100+hart_recv_buf[9];        //保存响应码
                    modnet_inputreg_buf[mapaddress+1]=in_len;                   //保存返回命令的数据长度
                    modnet_inputreg_buf[mapaddress+2]=hart_recv_buf[10];        //序列号,3Byte,需要2个Reg
                    modnet_inputreg_buf[mapaddress+3]=hart_recv_buf[11]*0x100+hart_recv_buf[12];
                    modnet_inputreg_buf[mapaddress+4]=hart_recv_buf[13];        //单位代码,1byte,需要1个Reg
                    modnet_inputreg_buf[mapaddress+5]=hart_recv_buf[14]*0x100+hart_recv_buf[15];    //上限,浮点,4Byte,需要2个Reg
                    modnet_inputreg_buf[mapaddress+6]=hart_recv_buf[16]*0x100+hart_recv_buf[17];
                    modnet_inputreg_buf[mapaddress+7]=hart_recv_buf[18]*0x100+hart_recv_buf[19];    //下限,浮点,4Byte,需要2个Reg
                    modnet_inputreg_buf[mapaddress+8]=hart_recv_buf[20]*0x100+hart_recv_buf[21];
                    modnet_inputreg_buf[mapaddress+9]=hart_recv_buf[22]*0x100+hart_recv_buf[23];    //最小精度,浮点,4Byte,需要2个Reg
                    modnet_inputreg_buf[mapaddress+10]=hart_recv_buf[24]*0x100+hart_recv_buf[25];
                semaphore_v(sem_mb_inputreg_id);                                //退出临界区
                if(hart_debug_mode)
                {
                    syslog(LOG_DEBUG,"[DEBUG][HartGW][RECV_CMD=14][%s]Write to InputReg=%d",coms.sPortName,mapaddress+1);
                }
                break;
            case 15:                                                            //报警选择代码1Byte,功能代码1Byte,量程单位代码1Byte,
                                                                                //量程上限,浮点,4Byte,量程下限,浮点,4Byte,阻尼值,浮点,4Byte,
                                                                                //写保护代码1Byte,发行商代码1Byte
                                                                                //需要13个Reg
                semaphore_p(sem_mb_inputreg_id);                                //进入临界区
                    modnet_inputreg_buf[mapaddress]=hart_recv_buf[8]*0x100+hart_recv_buf[9];        //保存响应码
                    modnet_inputreg_buf[mapaddress+1]=in_len;                   //保存返回命令的数据长度
                    modnet_inputreg_buf[mapaddress+2]=hart_recv_buf[10];            //报警选择代码,1Byte,需要1个Reg
                    modnet_inputreg_buf[mapaddress+3]=hart_recv_buf[11];            //功能代码,1Byte,需要1Reg
                    modnet_inputreg_buf[mapaddress+4]=hart_recv_buf[12];            //量程单位代码,1byte,需要1个Reg
                    modnet_inputreg_buf[mapaddress+5]=hart_recv_buf[13]*0x100+hart_recv_buf[14];    //量程上限,浮点,4Byte,需要2个Reg
                    modnet_inputreg_buf[mapaddress+6]=hart_recv_buf[15]*0x100+hart_recv_buf[16];
                    modnet_inputreg_buf[mapaddress+7]=hart_recv_buf[17]*0x100+hart_recv_buf[18];    //量程下限,浮点,4Byte,需要2个Reg
                    modnet_inputreg_buf[mapaddress+8]=hart_recv_buf[19]*0x100+hart_recv_buf[20];
                    modnet_inputreg_buf[mapaddress+9]=hart_recv_buf[21]*0x100+hart_recv_buf[22];    //阻尼,浮点,4Byte,需要2个Reg
                    modnet_inputreg_buf[mapaddress+10]=hart_recv_buf[23]*0x100+hart_recv_buf[24];
                    modnet_inputreg_buf[mapaddress+11]=hart_recv_buf[25];           //写保护代码,1Byte,需要1个Reg
                    modnet_inputreg_buf[mapaddress+12]=hart_recv_buf[26];           //发行商代码,1Byte,需要1个Reg
                semaphore_v(sem_mb_inputreg_id);                                    //退出临界区
                if(hart_debug_mode)
                {
                    syslog(LOG_DEBUG,"[DEBUG][HartGW][RECV_CMD=15][%s]Write to InputReg=%d",coms.sPortName,mapaddress+1);
                }
                break;
            case 16:                                                             //读取最终装配号3Byte,需要5Reg
                semaphore_p(sem_mb_inputreg_id);                                 //进入临界区
                    modnet_inputreg_buf[mapaddress]=hart_recv_buf[8]*0x100+hart_recv_buf[9];    //保存响应码
                    modnet_inputreg_buf[mapaddress+1]=in_len;                    //保存返回命令的数据长度
                    modnet_inputreg_buf[mapaddress+2]=hart_recv_buf[10];                        //最终装配号,3Byte
                    modnet_inputreg_buf[mapaddress+3]=hart_recv_buf[11]*0x100+hart_recv_buf[12];
                    modnet_inputreg_buf[mapaddress+4]=hart_recv_buf[13];        //功能代码,1Byte,需要1Reg
                semaphore_v(sem_mb_inputreg_id);                                //退出临界区
                if(hart_debug_mode)
                {
                    syslog(LOG_DEBUG,"[DEBUG][HartGW][RECV_CMD=16][%s]Write to InputReg=%d",coms.sPortName,mapaddress+1);
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
        for(i=0;i<HART_CMDS_NUM_MAX;i++)   //遍历每一条命令
        {
            if(HT.HTCmds[i].iEn)
            {
                modnet_inputreg_buf[hart_run_status_reg+10+i*3]=HT.HTCmds[i].iSuccCount;       //通讯成功的次数
                modnet_inputreg_buf[hart_run_status_reg+10+i*3+1]=HT.HTCmds[i].iFailedCount;   //通讯失败的次数
                modnet_inputreg_buf[hart_run_status_reg+10+i*3+2]=HT.HTCmds[i].iTimeoutCount;  //通讯超时的次数
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
        //延迟5ms,然后进行下一次处理,经过测试证明usleep给出的参数<5000延迟5ms,5000-10000延迟10ms
        usleep(5000);
    }

    return 0;           
}