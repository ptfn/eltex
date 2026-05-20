#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <linux/netlink.h>

#define NETLINK_USER 31
#define BUF_SIZE 256

int main()
{
    struct sockaddr_nl src_addr, dest_addr;
    struct nlmsghdr *nlh;
    struct iovec iov;
    struct msghdr msg;
    int sock;

    sock = socket(AF_NETLINK, SOCK_RAW, NETLINK_USER);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    memset(&src_addr, 0, sizeof(src_addr));
    src_addr.nl_family = AF_NETLINK;
    src_addr.nl_pid = getpid();

    if (bind(sock, (struct sockaddr *)&src_addr, sizeof(src_addr)) < 0) {
        perror("bind");
        close(sock);
        return 1;
    }

    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.nl_family = AF_NETLINK;
    dest_addr.nl_pid = 0;
    dest_addr.nl_groups = 0;

    nlh = malloc(NLMSG_SPACE(BUF_SIZE));
    memset(nlh, 0, NLMSG_SPACE(BUF_SIZE));
    nlh->nlmsg_len = NLMSG_SPACE(BUF_SIZE);
    nlh->nlmsg_pid = getpid();
    nlh->nlmsg_flags = 0;

    strcpy(NLMSG_DATA(nlh), "Hello from userspace");

    iov.iov_base = (void *)nlh;
    iov.iov_len = nlh->nlmsg_len;
    msg.msg_name = (void *)&dest_addr;
    msg.msg_namelen = sizeof(dest_addr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    printf("Sending: %s\n", (char *)NLMSG_DATA(nlh));

    if (sendmsg(sock, &msg, 0) < 0) {
        perror("sendmsg");
        close(sock);
        free(nlh);
        return 1;
    }

    printf("Waiting for reply...\n");

    if (recvmsg(sock, &msg, 0) < 0) {
        perror("recvmsg");
        close(sock);
        free(nlh);
        return 1;
    }

    printf("Received: %s\n", (char *)NLMSG_DATA(nlh));

    close(sock);
    free(nlh);
    return 0;
}
