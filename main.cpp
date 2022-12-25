#include <csignal>
#include "lib/server/server.hpp"

int main()
{
    int listen_fd = passive_server(8008, 20);
    int connFd;

    struct sockaddr_in client_addr;
    socklen_t addr_length = sizeof(client_addr);

    int epoll_fd = epoll_create(MAX_EVENTS);

    if (epoll_fd == -1)
    {
        perror("epoll_create failed\n");
        exit(EXIT_FAILURE);
    }

    struct epoll_event event;
    struct epoll_event events[MAX_EVENTS];
    event.events = EPOLLIN;
    event.data.fd = listen_fd;

    signal(SIGPIPE, SIG_IGN);

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &event) == -1) // 注册新的listen_fd到epoll_fd中
    {
        perror("epll_ctl:servaddr register failed\n");
        exit(EXIT_FAILURE);
    }
    printf("listen:%d\n", listen_fd);
    int nfds;
    while (true)
    {
        nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, 10);
        if (nfds != 0)
            printf("\nnfds:%d\n", nfds);

        if (nfds == -1)
        {
            perror("start epoll_wait failed\n");
            continue;
        }

        for (int i = 0; i < nfds; ++i)
        {
            if (events[i].data.fd == listen_fd)
            {
                if ((connFd = accept(listen_fd, (struct sockaddr *)&client_addr, &addr_length)) < 0)
                {
                    perror("accept conn_fd failed");
                    exit(EXIT_FAILURE);
                }
                printf("accept:%d\n", connFd);
                shakehands(connFd);
                printf("shakerhands\n");
                player newPlayer;
                mapPlayer[connFd] = newPlayer;

                event.events = EPOLLOUT; // 表示对应的文件描述符可读（包括对端SOCKET正常关闭）
                mapPlayer[connFd].event = 1;
                event.data.fd = connFd; // 将connFd设置为要读取的文件描述符
                // event.data.ptr = &head;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, connFd, &event) == -1)
                {
                    perror("epoll_ctl:conn_fd register failed");
                    exit(EXIT_FAILURE);
                }
            }
            else if (events[i].events & EPOLLIN)
            {
            }
        }
    }
}