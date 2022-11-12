#include "ftp_client.h"

void read_command_from_cmdline(char *cmdline, struct user_command *command){
    //读取用户输入的命令
    while(1){
        printf("ftp> ");
        //先将command清空
        memset(command, 0, sizeof(struct user_command));
        fgets(cmdline,MAX_CMDLINE,stdin);
        if(cmdline[0] == '\n')
            continue;
        //将用户输入的命令分割成命令码和命令参数
        char *p = strtok(cmdline, " ");
        //如果有\n,就除去\n
        if(p[strlen(p)-1] == '\n')
            p[strlen(p)-1] = '\0';
        strcpy(command->command_name, p);
        //查看命令是否是合法的
        if(
            strcmp(command->command_name, "get") != 0 
            && strcmp(command->command_name, "put") != 0 
            && strcmp(command->command_name, "delete") != 0 
            && strcmp(command->command_name, "ls") != 0 
            && strcmp(command->command_name, "cd") != 0 
            && strcmp(command->command_name, "mkdir") != 0
            && strcmp(command->command_name, "pwd") != 0
            && strcmp(command->command_name, "quit") != 0){
            print_ftp_info(500, "invalid command");
            continue;
        }
        //将命令码转换成小写
        int i = 0;
        while(command->command_name[i] != '\0'){
            command->command_name[i] = tolower(command->command_name[i]);
            i++;
        }


        p = strtok(NULL, " ");
        if(p != NULL){
            if(p[strlen(p)-1] == '\n')
                p[strlen(p)-1] = '\0';
            strcpy(command->argument, p);
        }
        // TODO: 带空格的文件名
        //三个参数,则会报错
        if(strtok(NULL, " ") != NULL){
            print_ftp_info(500, "too many arguments");
            continue;
        }
        break;
    }
}

void ftp_put(char* filename,SOCKET sclient){
    if(file_exists(filename) == 0){
        print_ftp_info(550, "file not exists");
        return;
    }
    //先发送命令
    MsgHeader msgHeader;
    memset(&msgHeader, 0, sizeof(msgHeader));
    msgHeader.msgType = MSGTYPE_PUT;
    msgHeader.msgID = MSG_FILEINFO;
    strcpy(msgHeader.info.commandInfo.argument, filename);
    send_data_to_server(sclient, (char* )&msgHeader);
    int recv_result = recv_data_from_server(sclient, (char* )&msgHeader);
    if (msgHeader.msgID == MSG_INVALID_FILENAME) {
        print_ftp_info(msgHeader.msgID, msgHeader.info.commandInfo.argument);
        return;
    }
    if(msgHeader.msgType == MSGTYPE_PUT && msgHeader.msgID == MSG_READY){
        //发送文件
        SOCKET data_client = INVALID_SOCKET;
        data_client = create_tcp_socket();
        if(data_client == INVALID_SOCKET)
            printf("socket error !");
        //绑定本机的端口
        srand(time(NULL));
        bind_socket_local_port(data_client, rand()%10000);
        //连接server端
        int connect_result = connect_to_server(data_client, "127.0.0.1",8001);
        // 在此进行状态机的变换
        if(connect_result == 0){
            printf("connect error !");
            return ;
        }
        //发送文件
        send_file_to_server(data_client, filename);
    }
}

void ftp_quit(SOCKET sclient){
    //发送命令数据包
    MsgHeader msgHeader;
    memset(&msgHeader, 0, sizeof(msgHeader));
    msgHeader.msgType = MSGTYPE_QUIT;
    send_data_to_server(sclient, (char* )&msgHeader);
    closesocket(sclient);
    printf("bye~");
}

void ftp_get(char* filename,SOCKET sclient){}

void ftp_ls(SOCKET sclient){
    //发送命令数据包
    MsgHeader msgHeader;
    memset(&msgHeader, 0, sizeof(msgHeader));
    msgHeader.msgType = MSGTYPE_LS;
    msgHeader.msgID = MSG_DIRINFO;
    char pwd_command[MAX_FILE_SIZE] = "ls";
    send_data_to_server(sclient, (char* )&msgHeader);

    //接收数据包
    int recv_result = recv_data_from_server(sclient, (char* )&msgHeader);
    printf("%s\n", msgHeader.info.commandInfo.argument);
}

void ftp_cd(char* dirname,SOCKET sclient){
    //发送命令数据包
    MsgHeader msgHeader;
    memset(&msgHeader, 0, sizeof(msgHeader));
    msgHeader.msgType = MSGTYPE_CD;
    msgHeader.msgID = MSG_DIRINFO;
    strcpy(msgHeader.info.commandInfo.argument, dirname);
    send_data_to_server(sclient, (char* )&msgHeader);
    
    //接收数据包
    int recv_result = recv_data_from_server(sclient, (char* )&msgHeader);
    printf("%s\n", msgHeader.info.commandInfo.argument);
}

void ftp_mkdir(char* dirname,SOCKET sclient){
    //发送命令数据包
    MsgHeader msgHeader;
    memset(&msgHeader, 0, sizeof(msgHeader));
    msgHeader.msgType = MSGTYPE_MKDIR;
    msgHeader.msgID = MSG_DIRINFO;
    strcpy(msgHeader.info.commandInfo.argument, dirname);
    send_data_to_server(sclient, (char* )&msgHeader);
    
    //接收数据包
    int recv_result = recv_data_from_server(sclient, (char* )&msgHeader);
    printf("%s\n", msgHeader.info.commandInfo.argument);
}

void ftp_pwd(SOCKET sclient){
    //发送命令数据包
    MsgHeader msgHeader;
    memset(&msgHeader, 0, sizeof(msgHeader));
    msgHeader.msgType = MSGTYPE_PWD;
    msgHeader.msgID = MSG_DIRINFO;
    char pwd_command[MAX_FILE_SIZE] = "pwd";
    send_data_to_server(sclient, (char* )&msgHeader);

    //接收数据包
    int recv_result = recv_data_from_server(sclient, (char* )&msgHeader);
    printf("%s\n", msgHeader.info.commandInfo.argument);
}

void ftp_delete(char* filename,SOCKET sclient){
    //发送命令数据包
    MsgHeader msgHeader;
    memset(&msgHeader, 0, sizeof(msgHeader));
    msgHeader.msgType = MSGTYPE_DELETE;
    msgHeader.msgID = MSG_DIRINFO;
    strcpy(msgHeader.info.commandInfo.argument, filename);
    send_data_to_server(sclient, (char* )&msgHeader);
    
    //接收数据包
    int recv_result = recv_data_from_server(sclient, (char* )&msgHeader);
    printf("%s\n", msgHeader.info.commandInfo.argument);
}

int main(){
    char cmdline[MAX_CMDLINE];
    struct user_command command;
    //创建tcp Socket
    SOCKET sclient = INVALID_SOCKET;
    sclient = create_tcp_socket();
    if(sclient == INVALID_SOCKET)
        printf("socket error !");
    //绑定本机的端口
    srand(time(NULL));
    bind_socket_local_port(sclient, rand()%10000);
    //连接server端
    int connect_result = connect_to_server(sclient, "127.0.0.1",8000);
    int count = 0;
    //连接失败进行轮询,轮询最多10次
    // while(connect_result == 0&&count < 10){
    //     Sleep(1000);
    //     printf("retrying...\n");
    //     connect_result = connect_to_server(sclient, "192.168.56.1",8080);
    //     count++;
    // }
    //读取命令并执行
    while(1 && connect_result){
        read_command_from_cmdline(cmdline,&command);
        if(strcmp(command.command_name, "get") == 0){
            ftp_get(command.argument,sclient);
        }
        if(strcmp(command.command_name, "put") == 0){
            ftp_put(command.argument, sclient);
        }
        if(strcmp(command.command_name, "delete") == 0){
            ftp_delete(command.argument, sclient);
        }
        if(strcmp(command.command_name, "ls") == 0){
            ftp_ls(sclient);
        }
        if(strcmp(command.command_name, "cd") == 0){
            ftp_cd(command.argument, sclient);
        }
        if(strcmp(command.command_name, "mkdir") == 0){
            ftp_mkdir(command.argument, sclient);
        }
        if(strcmp(command.command_name, "pwd") == 0){
            ftp_pwd(sclient);
        }
        if(strcmp(command.command_name, "quit") == 0){
            ftp_quit(sclient);
            break;
        }
    }
    return 0;
}