#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

#include "clounix/s3ip_netlink.h"

struct s3ip_usr_data {
    struct nlmsghdr nlh;
    union s3ip_data data;
};

static struct s3ip_usr_data u_data;
static int sockfd;
static struct sockaddr_nl proxy_addr, kernel_addr;

void process_s3ip_data(struct s3ip_usr_data *p)
{
    unsigned short type, index;
    unsigned char attr_type;

    type = p->data.msg.type;
    index = p->data.msg.index;

    printf("kernel cmd: type %d index %d attr_type %d\n", type, index, attr_type);
    p->nlh.nlmsg_len = NLMSG_SPACE(sizeof(union s3ip_data));
    p->nlh.nlmsg_flags = 0;
    p->nlh.nlmsg_type = 0;
    p->nlh.nlmsg_seq = 0;
    p->nlh.nlmsg_pid = S3IP_PORT;
    
    memcpy(p->data.buff, "this is usr msg", strlen("this is usr msg"));
    
    sendto(sockfd, &(p->nlh), p->nlh.nlmsg_len, 0, (struct sockaddr *)&kernel_addr, sizeof(struct sockaddr_nl));

    return;
}

int main(int argc, char **argv)
{
    int ret;
    socklen_t len;

    sockfd = socket(AF_NETLINK, SOCK_RAW, NETLINK_S3IP);

    memset(&proxy_addr, 0, sizeof(proxy_addr));
    proxy_addr.nl_family = AF_NETLINK;
    proxy_addr.nl_pad = 0;
    proxy_addr.nl_pid = S3IP_PORT;
    proxy_addr.nl_groups = 0;

    bind(sockfd, (struct sockaddr *)&proxy_addr, sizeof(proxy_addr));
    
    memset(&kernel_addr, 0, sizeof(kernel_addr));

    kernel_addr.nl_family = AF_NETLINK;
    kernel_addr.nl_pid = NETLINK_TO_KERNEL;
    kernel_addr.nl_groups = 0;
    
    memset(&u_data, 0, sizeof(u_data));
    len = sizeof(struct sockaddr_nl);
    
    while (1) {
        ret = recvfrom(sockfd, &u_data, sizeof(struct s3ip_usr_data), 0, (struct sockaddr *)&kernel_addr, &len);
        if (ret > 0) {
            process_s3ip_data(&u_data);
            memset(&u_data, 0, sizeof(u_data));
        }
    }
    close(sockfd);

    return 0;
}
