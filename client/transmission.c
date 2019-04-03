#include "transmission.h"

int send_cycle(int fd, const char* data, int send_len)
{
    int total = 0;
    int ret;
    while(total < send_len)
    {
        ret = send(fd, data + total, send_len - total, 0);
        if (ret == -1)
        {
            return -1;
        }
        total = total + ret;
    }
    return 0;
}

int recv_cycle(int fd, char* data, int recv_len)
{
    int total = 0;
    int ret;
    while (total < recv_len)
    {
        ret = recv(fd, data + total, recv_len - total, 0);
        total = total + ret;
    }
    return 0;
}

void print_help()
{
    printf("----------welcome to Evilolipop Netdisk----------\n\n");
    printf("Usage:\n\n");
    printf("list file:                 ls [<file>]\n");
    printf("print working directory:   pwd\n");
    printf("change directory:          cd <path>\n");
    printf("download file:             gets <file>, directory not supported\n");
    printf("upload file:               puts <file>, directory not supported\n");
    printf("this page:                 --help\n");
}

int connect_server(int* socketFd, const char* ip, const char* port)
{
    int ret;
    *socketFd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serAddr;
    serAddr.sin_family = AF_INET;
    serAddr.sin_addr.s_addr = inet_addr(ip);
    serAddr.sin_port = htons(atoi(port));
    ret = connect(*socketFd, (struct sockaddr*)&serAddr, sizeof(struct sockaddr));
    if (ret == -1)
    {
        printf("connect failed\n");
        return -1;
    }
    return 0;
}

int tran_authen(int* socketFd, const char* ip, const char* port, char* user_name, DataPackage* data, int err)
{
    int ret;
    int flag, err_flag = 0;
    do
    {
        flag = 0;
        system("clear");
        if (err == -1)
        {
            printf("wrong username or password\n");
            err = 0;
        }
        if (err_flag == 1)
        {
            printf("username too long!\n");
            err_flag = 0;
        }
        printf("Enter username: ");
        fflush(stdout);
        while (1)
        {
            ret = read(STDIN_FILENO, user_name, USER_LEN);
            if (ret >= 20)
            {
                flag = 1;
                err_flag = 1;       //too long err
            }
            else
            {
                break;
            }
        }
    } while (flag == 1);
    user_name[ret - 1] = '\0';

    char* password;
    err_flag = 0;
    while (1)
    {
        if (err_flag == 1)
        {
            system("clear");
            printf("Enter username: %s\n", user_name);
            printf("password too long!\n");
            err_flag = 0;
        }
        password = getpass("Enter password: ");
        if (strlen(password) > 20)
        {
            err_flag = 1;
        }
        else
        {
            break;
        }
    }

    //connect to server
    ret = connect_server(socketFd, ip, port);
    if (ret == -1)
    {
        return -1;
    }
    data->data_len = 0;
    send_cycle(*socketFd, (char*)data, sizeof(int));        //0 for login
    data->data_len = strlen(user_name) + 1;
    strcpy(data->buf, user_name);
    send_cycle(*socketFd, (char*)data, data->data_len + sizeof(int));   //send username
    data->data_len = strlen(password) + 1;
    strcpy(data->buf, password);
    send_cycle(*socketFd, (char*)data, data->data_len + sizeof(int));   //send password

    recv_cycle(*socketFd, (char*)&data->data_len, sizeof(int));         //recv token
    recv_cycle(*socketFd, data->buf, data->data_len);
    if (data->data_len == -1)
    {
        close(*socketFd);
        return -1;
    }
    else
    {
        return 0;
    }
}

int cmd_interpret(const DataPackage* data)
{
    if (strcmp(data->buf, "--help") == 0)
    {
        system("clear");
        print_help();
        return 1;
    }
    else
    {
        int i = 0;
        int space = 0;
        char prefix[10];
        while (1)
        {
            if ((data->buf[i] == ' ' || data->buf[i] == '\0') && space == 0)
            {
                strncpy(prefix, data->buf, i);
                prefix[i] = '\0';
                if (strcmp(prefix, "ls") && strcmp(prefix, "cd") && strcmp(prefix, "pwd")
                    && strcmp(prefix, "puts") && strcmp(prefix, "gets") && strcmp(prefix, "remove"))
                {
                    system("clear");
                    printf("-----$ %s\n", data->buf);
                    printf("invaild command\n");
                    return -1;
                }
            }
            if (data->buf[i] == ' ')
            {
                space++;
                if (space == 2)
                {
                    system("clear");
                    printf("invaild command\n");
                    printf("-----$ %s\n", data->buf);
                    return -1;
                }
            }
            if (data->buf[i] == '\0')
            {
                if (space == 0)
                {
                    if (!strcmp(prefix, "cd") || !strcmp(prefix, "puts")
                        || !strcmp(prefix, "gets") || !strcmp(prefix, "remove"))
                    {
                        system("clear");
                        printf("please enter file path\n");
                        printf("-----$ %s\n", data->buf);
                        return -1;
                    }
                }
                break;
            }
            i++;
        }

        if (strcmp(prefix, "gets") == 0)
        {
            system("clear");
            printf("-----$ %s\n", data->buf);
            return 2;
        }
        if (strcmp(prefix, "puts") == 0)
        {
            system("clear");
            printf("-----$ %s\n", data->buf);
            return 3;
        }
        return 0;
    }
}

int tran_cmd(int socket_fd, DataPackage* data)
{
    send_cycle(socket_fd, (char*)data, data->data_len + 4);
    system("clear");
    printf("-----$ %s\n", data->buf);
    while (1)
    {
        recv_cycle(socket_fd, (char*)&data->data_len, sizeof(int));
        if (data->data_len == 0)
        {
            break;
        }
        recv_cycle(socket_fd, data->buf, data->data_len);
        printf("%s\n", data->buf);
    }
    return 0;
}

void* get_files(void* p)
{
    int ret, socketFd;
    DataPackage data;
    TransInfo* trans_info = (TransInfo*)p;
    ret = connect_server(&socketFd, trans_info->ip_address, trans_info->port);
    if (ret == -1)
    {
        close(socketFd);
        pthread_exit(NULL);
    }
    data.data_len = 2;
    send_cycle(socketFd, (char*)&data, sizeof(int));        //2 for gets
    data.data_len = strlen(trans_info->token) + 1;
    strcpy(data.buf, trans_info->token);
    send_cycle(socketFd, (char*)&data, sizeof(int) + data.data_len);//send token
    recv_cycle(socketFd, (char*)&data.data_len, sizeof(int));
    if (data.data_len == -1)
    {
        close(socketFd);
        pthread_exit(NULL);
    }

    data.data_len = strlen(trans_info->cmd) + 1;
    strcpy(data.buf, trans_info->cmd);
    send_cycle(socketFd, (char*)&data, sizeof(int) + data.data_len);//send command
    recv_cycle(socketFd, (char*)&data.data_len, sizeof(int));
    if (data.data_len == -1)
    {
        close(socketFd);
        pthread_exit(NULL);
    }

    recv_cycle(socketFd, (char*)&data.data_len, sizeof(int));       //recv filename
    recv_cycle(socketFd, data.buf, data.data_len);
    char path_name[CMD_LEN] = "./downloads/";
    strcat(path_name, data.buf);
    int fd = open(path_name, O_CREAT|O_RDWR, 0666);
    if (fd == -1)
    {
        close(socketFd);
        pthread_exit(NULL);
    }

    recv_cycle(socketFd, (char*)&data.data_len, sizeof(int));       //recv size
    recv_cycle(socketFd, data.buf, data.data_len);
    off_t size = atoi(data.buf);

    int transfered = 0;
    time_t start, end;
    start = time(NULL);
    printf("\r%4.1f%%", (float)transfered / size * 100);
    fflush(stdout);
    while (1)
    {
        recv_cycle(socketFd, (char*)&data.data_len, sizeof(int));
        if (data.data_len > 0)
        {
            recv_cycle(socketFd, data.buf, data.data_len);
            write(fd, data.buf, data.data_len);
            transfered += data.data_len;
            end = time(NULL);
            if (end - start >= 1)
            {
                printf("\r%4.1f%%", (float)transfered / size * 100);
                start = end;
                fflush(stdout);
            }
        }
        else
        {
            printf("\r%4.1f%%\n", (float)transfered / size * 100);
            close(fd);
            break;
        }
    }
    close(socketFd);
    pthread_exit(NULL);
}

void* put_files(void* p)
{
    int ret, socketFd;
    DataPackage data;
    TransInfo* trans_info = (TransInfo*)p;
    ret = connect_server(&socketFd, trans_info->ip_address, trans_info->port);
    if (ret == -1)
    {
        close(socketFd);
        pthread_exit(NULL);
    }
    data.data_len = 3;
    send_cycle(socketFd, (char*)&data, sizeof(int));        //3 for puts
    data.data_len = strlen(trans_info->token) + 1;
    strcpy(data.buf, trans_info->token);
    send_cycle(socketFd, (char*)&data, sizeof(int) + data.data_len);//send token
    recv_cycle(socketFd, (char*)&data.data_len, sizeof(int));
    if (data.data_len == -1)
    {
        close(socketFd);
        pthread_exit(NULL);
    }

    data.data_len = strlen(trans_info->cmd) + 1;
    strcpy(data.buf, trans_info->cmd);
    send_cycle(socketFd, (char*)&data, sizeof(int) + data.data_len);//send command
    recv_cycle(socketFd, (char*)&data.data_len, sizeof(int));
    if (data.data_len != 0)
    {
        close(socketFd);
        pthread_exit(NULL);
    }

    //open file
    char file_path[CMD_LEN];
    int i = 0;
    while (trans_info->cmd[i] != '\0')
    {
        file_path[i] = trans_info->cmd[i + 5];
        i++;
    }
    file_path[i] = '\0';
    int fd = open(file_path, O_RDONLY);
    if (fd == -1)
    {
        close(socketFd);
        pthread_exit(NULL);
    }

    //send filename
    char file_name[FILE_NAME_LEN];
    i = i + 5;
    while (trans_info->cmd[i] != '/' && trans_info->cmd[i] != ' ')
    {
        i--;
    }
    i++;
    int k = 0;
    while (trans_info->cmd[i] != '\0')
    {
        file_name[k++] = trans_info->cmd[i++];
    }
    file_name[k] = '\0';
    data.data_len = strlen(file_name) + 1;
    strcpy(data.buf, file_name);
    ret = send_cycle(socketFd, (char*)&data, data.data_len + sizeof(int));
    if (ret == -1)
    {
        close(fd);
        close(socketFd);
        pthread_exit(NULL);
    }

    //send file size
    struct stat buf;
    ret = fstat(fd, &buf);
    if (ret == -1)
    {
        close(fd);
        close(socketFd);
        pthread_exit(NULL);
    }
    off_t file_size = buf.st_size;
    sprintf(data.buf, "%ld", file_size);
    data.data_len = strlen(data.buf);
    send_cycle(socketFd, (char*)&data, data.data_len + sizeof(int));
    if (ret == -1)
    {
        close(fd);
        close(socketFd);
        pthread_exit(NULL);
    }

    //send file
    while ((data.data_len = read(fd, data.buf, sizeof(data.buf))) > 0)
    {
        ret = send_cycle(socketFd, (char*)&data, data.data_len + sizeof(int));
        if (ret == -1)
        {
            close(fd);
            close(socketFd);
            pthread_exit(NULL);
        }
    }

    //send end of transmission
    data.data_len = 0;
    ret = send_cycle(socketFd, (char*)&data, sizeof(int));
    if (ret == -1)
    {
        close(fd);
        close(socketFd);
        pthread_exit(NULL);
    }
    printf("upload success\n");
    close(fd);
    close(socketFd);
    pthread_exit(NULL);
}
