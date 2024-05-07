#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <sys/time.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <stdio.h>

#define WAITING 0
#define BARGING 1
#define SOLD 2
#define ACCEPTED 2
#define DECLINED 0
typedef struct Product Product;
typedef struct UDP_Config UDP_Config;

struct Product
{
    char *name;
    int port;
    int status;
    int client_fd;
    int server_fd;
    int price;
};

int setupServer(int port)
{
    struct sockaddr_in address;
    int server_fd;
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed\n");
        printf("error in binding\n");
        return -1;
    }

    listen(server_fd, 4);

    return server_fd;
}

int acceptClient(int server_fd)
{
    int client_fd;
    struct sockaddr_in client_address;
    int address_len = sizeof(client_address);
    client_fd = accept(server_fd, (struct sockaddr *)&client_address, (socklen_t *)&address_len);

    return client_fd;
}

struct UDP_Config
{
    int sock;
    struct sockaddr_in bc_address;
};

UDP_Config setupUDPserver(int port)
{
    int sock, broadcast = 1, opt = 1;
    struct sockaddr_in bc_address;
    UDP_Config result;
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
    setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));

    bc_address.sin_family = AF_INET;
    bc_address.sin_port = htons(port);
    bc_address.sin_addr.s_addr = inet_addr("255.255.255.255");
    result.bc_address = bc_address;
    if (bind(sock, (struct sockaddr *)&bc_address, sizeof(bc_address)) < 0)
    {
        sock = -1;
    }
    result.sock = sock;
    return result;
}

void menu(const char *name)
{
    printf("*******************\n");
    printf("seller %s please choose:\n", name);
    printf("*******************\n");
    printf("products: to see products\n");
    printf("add product port: to add new product\n");
    printf("res product yes/no: to accept/decline\n\n");
}

void showProducts(Product **products, int product_num)
{
    for (int i = 0; i < product_num; i++)
    {
        printf("%d.%s server:%d, status: %s\n", i, products[i]->name, products[i]->server_fd, products[i]->status == WAITING ? "WAITING" : "not available");
    }
}

void addProduct(Product ***products, int *product_num, int *product_capa, Product *pro)
{
    *product_num = (*product_num) + 1;
    if (*product_capa == *product_num)
    {
        *product_capa = 2 * (*product_capa);
        (*products) = realloc(*products, (*product_capa) * sizeof(Product *));
    }
    (*products)[(*product_num) - 1] = pro;
    printf("new product add:%s\n", (*products)[(*product_num) - 1]->name);
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

int findProSer(Product **products, int fd, int product_num)
{
    for (int i = 0; i < product_num; i++)
    {
        if (products[i]->server_fd == fd)
        {
            return i;
        }
    }
    return -1;
}

int findProCli(Product **products, int fd, int product_num)
{
    for (int i = 0; i < product_num; i++)
    {
        if (products[i]->client_fd == fd)
        {
            return i;
        }
    }
    return -1;
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

    int udp_server;
    char buffer[1024] = {0};
    fd_set master_set, working_set;

    int product_num = 0, product_capa = 4;
    Product **products = malloc(product_capa * sizeof(Product *));

    FD_ZERO(&master_set);
    // FD_SET(0, &master_set);

    UDP_Config udp_conf = setupUDPserver(udp_port);
    struct sockaddr_in bc_address = udp_conf.bc_address;
    udp_server = udp_conf.sock;
    if (udp_server < 0)
    {
        printf("error in udp setup\n");
        exit(-1);
    }

    printf("Server is running\n");

    while (1)
    {
        // printf("processing\n");
        // menu(name);
        FD_SET(0, &master_set);
        working_set = master_set;
        memset(buffer, 0, 1024);
        printf(">>\n");
        if (select(FD_SETSIZE, &working_set, NULL, NULL, NULL) < 0)
        {
            printf("error on selecting\n");
            // exit(-1);
        }
        for (int i = 0; i < FD_SETSIZE; i++)
        {
            memset(buffer, 0, 1024);
            if (FD_ISSET(i, &working_set))
            {
                printf("\nfd: %d\n", i);
                if (i == 0) // stdin
                {
                    printf("reading console\n");
                    read(0, buffer, 1024);
                    buffer[strlen(buffer) - 1] = '\0';
                    printf("the buffer is: %s\n", buffer);
                    char *token = strtok(buffer, " ");
                    if (strcmp(token, "menu") == 0) {
                        menu(name);
                        continue;
                    }
                    if (strcmp(token, "products") == 0)
                    {
                        if (strcmp(buffer, "products") != 0)
                        {
                            printf("command is products\n");
                            continue;
                        }
                        // printf("new product add:%s\n", products[product_num - 1]->name);
                        showProducts(products, product_num);
                    }
                    else if (strcmp(token, "add") == 0)
                    {
                        token = strtok(NULL, " ");
                        char *pro_name = token;
                        printf("%s\n", token);
                        token = strtok(NULL, " ");
                        printf("%s\n", token);
                        printf("%d\n", atoi(token));
                        int port;
                        if (((port = atoi(token)) < 0) || (strtok(NULL, " ") != NULL) || strlen(pro_name) == 0)
                        {
                            printf("command is add product port\n");
                            continue;
                        }
                        printf("port: %d\n", port);
                        int pro_sk = setupServer(port);
                        if (pro_sk < 0)
                        {
                            printf("error in binding sokc: %d\n", pro_sk);
                            continue;
                        }
                        Product *pro = malloc(sizeof(Product));
                        pro->name = malloc(strlen(pro_name));
                        strcpy(pro->name, pro_name);
                        pro->client_fd = -1;
                        pro->port = port;
                        pro->status = WAITING;
                        pro->server_fd = pro_sk;
                        pro->price = 0;
                        printf("productNum: %d, pro_capa: %d\n", product_num, product_capa);
                        addProduct(&products, &product_num, &product_capa, pro);
                        printf("productNum: %d, pro_capa: %d\n", product_num, product_capa);
                        printf("new product add:%s\n", products[product_num - 1]->name);
                        FD_SET(pro_sk, &master_set);
                        char massage[1024];
                        sprintf(massage, "%s %d %d", pro_name, port, WAITING);
                        printf("your product created: %s\n", massage);
                        sendto(udp_server, massage, strlen(massage), 0, (struct sockaddr *)&bc_address, sizeof(bc_address));
                    }
                    else if (strcmp(token, "res") == 0)
                    {
                        token = strtok(NULL, " ");
                        char *pro_name = token;
                        int pro_index = findProduct(products, pro_name, product_num);
                        if (pro_index == -1)
                        {
                            printf("product not found\n");
                            continue;
                        }
                        token = strtok(NULL, " ");
                        char *response = token;
                        char massage[1024];
                        if (strcmp(response, "yes") == 0)
                        {
                            products[pro_index]->status = ACCEPTED; // log
                            char *log_name = strcat(name, ".txt");
                            int log_fd = open(log_name, O_APPEND | O_CREAT |O_WRONLY);
                            printf("log_fd: %d, fileName: %s\n", log_fd, log_name);
                            sprintf(massage, "%s sold %d\n", products[pro_index]->name, products[pro_index]->price);
                            printf("saving to file: %s\n", massage);
                            write(log_fd, massage, strlen(massage));
                            close(log_fd);
                            send(products[pro_index]->client_fd, massage, strlen(massage), 0);
                            close(products[pro_index]->client_fd);
                            FD_CLR(products[pro_index]->client_fd, &master_set);
                            products[pro_index]->client_fd = -1;
                            products[pro_index]->status = SOLD;
                            sendto(udp_server, massage, strlen(massage), 0, (struct sockaddr *)&bc_address, sizeof(bc_address));
                        }
                        else if (strcmp(response, "no") == 0)
                        {
                            products[pro_index]->status = WAITING;
                            products[pro_index]->price = 0;
                            sprintf(massage, "%s declined %d\n", products[pro_index]->name, products[pro_index]->price);
                            send(products[pro_index]->client_fd, massage, strlen(massage), 0);
                            close(products[pro_index]->client_fd);
                            FD_CLR(products[pro_index]->client_fd, &master_set);
                            products[pro_index]->client_fd = -1;
                            sendto(udp_server, massage, strlen(massage), 0, (struct sockaddr *)&bc_address, sizeof(bc_address));
                        }
                        else
                        {
                            printf("command is res product yes/no\n");
                            continue;
                        }
                    }
                    else
                    {
                        printf("incorrect command\n");
                        menu(name);
                    }
                }
                else
                { // new buyer wants to connect to a product socket
                    int index;
                    if ((index = findProSer(products, i, product_num)) >= 0) // start connection
                    {
                        Product *pro = products[index];
                        printf("new buyer wants to connect to the product: %s\n", pro->name);
                        int new_cli = acceptClient(i);
                        if (new_cli < 0)
                        {
                            printf("error in accepting client\n");
                            continue;
                        }
                        if (pro->status == WAITING)
                        { // start dealing with the buyer
                            FD_SET(new_cli, &master_set);
                            pro->client_fd = new_cli;
                            pro->status = BARGING;
                            char massage[1024];
                            sprintf(massage, "%s %d %d", pro->name, pro->port, pro->status);
                            sendto(udp_server, massage, strlen(massage), 0, (struct sockaddr *)&bc_address, sizeof(bc_address));
                        }
                        else
                        {
                            char *massage = "product is not available";
                            printf("%s\n", massage);
                            send(new_cli, massage, strlen(massage), 0);
                            close(new_cli);
                            FD_CLR(new_cli, &master_set);

                            continue;
                        }
                    }
                    else if ((index = findProCli(products, i, product_num)) >= 0) // send price
                    {
                        Product *pro = products[index];
                        int bytes_received;
                        memset(buffer, 0, 1024);
                        bytes_received = recv(i, buffer, 1024, 0);
                        printf("358: buyer %d sent: %s\n", i, buffer);
                        if (strcmp(buffer, "declined") == 0)
                        {
                            pro->status = WAITING;
                            pro->price = 0;
                            close(pro->client_fd);
                            FD_CLR(pro->client_fd, &master_set);
                            pro->client_fd = -1;
                            char massage[1024];
                            sprintf(massage, "%s declined %d\n", pro->name, pro->price);
                            sendto(udp_server, massage, strlen(massage), 0, (struct sockaddr *)&bc_address, sizeof(bc_address));
                            continue;
                        }
                        if (bytes_received == 0)
                        { // EOF
                            printf("client fd = %d closed\n", i);
                            close(i);
                            FD_CLR(i, &master_set);
                            pro->client_fd = -1;
                            continue;
                        }
                        char *token = strtok(buffer, " ");
                        char *pro_name = token;
                        // int index = findProduct(products, pro_name, product_num);
                        if (strcmp(pro_name, pro->name) != 0)
                        {
                            printf("product name is not correct\n");
                            send(pro->client_fd, "product name is not correct", strlen("product name is not correct"), 0);
                            continue;
                        }
                        token = strtok(NULL, " ");
                        int price = atoi(token);
                        if (price < 0 || strtok(NULL, " ") != NULL)
                        {
                            printf("command is product price\n");
                            send(pro->client_fd, "command is product price", strlen("command is product price"), 0);
                            continue;
                        }
                        pro->price = price;
                        send(pro->client_fd, "price is recieved", strlen("price is recieved"), 0);
                        printf("client %d wants to buy: price: %d, pro_name: %s\n", i, price, pro_name);
                    }
                }
            }
        }
    }

    return 0;
}