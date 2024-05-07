#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <stdbool.h>
#include <arpa/inet.h>

#define WAITING 0
#define BARGING 1
#define SOLD 2
#define ACCEPTED 2
#define DECLINED 0
typedef struct Product Product;

struct Product
{
    char *name;
    long long port;
    int status;
    int pro_sock;
    int price;
};

int connectServer(int port)
{
    int fd;
    struct sockaddr_in server_address;

    fd = socket(AF_INET, SOCK_STREAM, 0);

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    { // checking for errors
        printf("Error in connecting to server\n");
    }

    return fd;
}
int setupUDPserver(int port)
{
    int sock, broadcast = 1, opt = 1;
    struct sockaddr_in bc_address;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
    setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));

    bc_address.sin_family = AF_INET;
    bc_address.sin_port = htons(port);
    bc_address.sin_addr.s_addr = inet_addr("255.255.255.255");

    bind(sock, (struct sockaddr *)&bc_address, sizeof(bc_address));
    return sock;
}

void menu(const char *name)
{
    printf("*******************\n");
    printf("Buyer %s please choose:\n", name);
    printf("*******************\n");
    printf("products: to see products\n");
    printf("productName price: to buy some shit!!\n\n\n");
}

int findProduct(Product **products, char *name, int product_num)
{
    for (int i = 0; i < product_num; i++)
    {
        if (strcmp(products[i]->name, name) == 0)
        {
            return i;
        }
    }
    return -1;
}

void addProduct(Product ***products, int *product_num, int *product_capa, Product *pro)
{
    *product_num = (*product_num) + 1;
    if (product_capa == product_num)
    {
        *product_capa = 2 * (*product_capa);
        (*products) = realloc(*products, (*product_capa) * sizeof(Product *));
    }
    (*products)[(*product_num) - 1] = pro;
}

void showProducts(Product **products, int product_num)
{
    for (int i = 0; i < product_num; i++)
    {
        printf("%d.%s: %s\n", i, products[i]->name, products[i]->status == WAITING ? "WAITING" : "not available");
    }
}

void alarm_handler(int sig)
{
    printf("TIMEOUT: Seller didn't respond.\n");
}

int main(int argc, char const *argv[])
{
    if (argc != 2)
    {
        printf("please enter your port\n");
        exit(-1);
    }
    int udp_port = atoi(argv[1]);
    printf("please enter your name:\n");
    char name[1024] = {0};
    read(0, name, 1024);
    name[strlen(name) - 1] = '\0';

    int new_socket, udp_server;
    char buffer[1024] = {0};
    fd_set master_set;

    int product_num = 0, product_capa = 4;
    Product **products = malloc(product_capa * sizeof(Product *));

    udp_server = setupUDPserver(udp_port);
    if (udp_server < 0)
    {
        printf("erroe in udp setup");
        exit(-1);
    }

    write(1, "Server is running\n", 18);
    menu(name);

    while (1)
    {
        menu(name);
        FD_ZERO(&master_set);
        FD_SET(0, &master_set);
        FD_SET(udp_server, &master_set);
        memset(buffer, 0, 1024);
        printf("processing\n\n");
        if (select(FD_SETSIZE, &master_set, NULL, NULL, NULL) < 0)
        {
            printf("error on selecting\n");
            exit(-1);
        }
        for (int i = 0; i < FD_SETSIZE; i++)
        {
            memset(buffer, 0, 1024);
            if (FD_ISSET(i, &master_set))
            {
                printf("fd: %d\n", i);
                if (i == udp_server)
                { // massage from network :sold | declined | bargin | new product
                    recv(udp_server, buffer, 1024, 0);
                    printf("164: seller said: %s\n", buffer);
                    char *pro_name = strtok(buffer, " ");

                    char *token = strtok(NULL, " ");
                    if (strcmp(token, "sold") == 0)
                    {
                        int pro_index = findProduct(products, pro_name, product_num);
                        if (pro_index < 0)
                        {
                            printf("error in finding product\n");
                        }
                        else
                        {
                            products[pro_index]->status = SOLD;
                            printf("178: product %s is sold\n", pro_name);
                        }
                    }
                    else if (strcmp(token, "declined") == 0)
                    {
                        int pro_index = findProduct(products, pro_name, product_num);
                        if (pro_index < 0)
                        {
                            printf("error in finding product\n");
                        }
                        else
                        {
                            products[pro_index]->status = WAITING;
                            printf("product %s is waiting\n", pro_name);
                        }
                    }
                    else
                    { // pro_name port status
                        int port = atoi(token);
                        int status = atoi(strtok(NULL, " ")); // barging or waiting
                        if (status == BARGING)
                        {
                            int pro_index = findProduct(products, pro_name, product_num);
                            if (pro_index < 0)
                            {
                                printf("error in finding product\n");
                            }
                            else
                            {
                                products[pro_index]->status = BARGING;
                                printf("product %s is bargining\n", pro_name);
                            }
                        }
                        else
                        {
                            Product *pro = malloc(sizeof(Product));
                            pro->name = malloc(strlen(pro_name));
                            strcpy(pro->name, pro_name);
                            pro->port = port;
                            pro->status = WAITING;
                            pro->price = 0;
                            pro->pro_sock = NULL;
                            addProduct(&products, &product_num, &product_capa, pro);
                            printf("new product %s is waiting\n", pro_name);
                        }
                    }
                }
                else if (i == 0)
                { // stdin
                    read(0, buffer, 1024);
                    buffer[strlen(buffer) - 1] = '\0';
                    if (strcmp(buffer, "products") == 0)
                    {
                        showProducts(products, product_num);
                    }
                    else
                    { // buy product
                        char *pro_name = strtok(buffer, " ");
                        int pro_index = findProduct(products, pro_name, product_num);
                        if (pro_index < 0)
                        {
                            printf("error in finding product\n");
                        }
                        else
                        {
                            Product *pro = products[pro_index];
                            printf("244:name: %s, status: %d port: %d\n", pro->name, pro->status, pro->port);
                            if (pro->status == WAITING)
                            {
                                int price = atoi(strtok(NULL, " "));
                                pro->price = price;

                                pro->pro_sock = connectServer(pro->port);
                                if (pro->pro_sock < 0)
                                {
                                    printf("error in connecting to product\n");
                                }
                                else
                                {
                                    char massage[1024] = {0};
                                    sprintf(massage, "%s %d", pro->name, pro->price);
                                    send(products[pro_index]->pro_sock, massage, 1024, 0);
                                    products[pro_index]->status = BARGING;
                                    printf("261: product %s is bargining\n", pro_name);
                                    recv(pro->pro_sock, massage, 1024, 0);
                                    printf("263:sller says: %s\n", massage);
                                    if (strcmp(massage, "price is recieved") == 0)
                                    {
                                        signal(SIGALRM, alarm_handler);
                                        siginterrupt(SIGALRM, 1);
                                        alarm(10);
                                        memset(massage, 0, 1024);
                                        recv(pro->pro_sock, massage, 1024, 0);
                                        printf("271: sller says: %s\n", massage);
                                        alarm(0);
                                        if (massage[0] == 0)
                                        {
                                            send(pro->pro_sock, "declined", 1024, 0);
                                            printf("product %s is declined due to the timeout\n", pro->name);
                                            printf("%s\n", massage);
                                            pro->status = WAITING;
                                        }
                                        else
                                        {
                                            char result[1024] = {0};
                                            sprintf(result, "%s sold %d\n", pro->name, pro->price);
                                            if (strcmp(massage, result) == 0)
                                            {
                                                printf("286: product %s is sold\n", pro->name);
                                                pro->status = SOLD;
                                            } else {
                                                pro->status = WAITING;
                                            }
                                        }
                                    }
                                    else
                                    {
                                        printf("there was a error\n");
                                        products[pro_index]->status = WAITING;
                                    }
                                    close(pro->pro_sock);
                                }
                            }
                            else
                            {
                                printf("product %s is not available\n", pro_name);
                            }
                        }
                    }
                }
                else
                { // seller sending msg
                    printf("we shouldnt be here!!!\n");
                    int bytes_received;
                    bytes_received = recv(i, buffer, 1024, 0);
                    if (bytes_received == 0)
                    { // EOF
                        printf("client fd = %d closed\n", i);

                        close(i);
                        FD_CLR(i, &master_set);
                        continue;
                    }

                    printf("seller said:  %d: %s\n", i, buffer);
                    // memset(buffer, 0, 1024);
                }
            }
        }
    }

    return 0;
}