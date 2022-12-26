#include <csignal>
#include "lib/server/server.hpp"
#include "lib/websocket/websocket.h"

int main()
{
    int listen_fd = passive_server(8008, 20);
    int conn_fd;

    struct sockaddr_in client_addr;
    socklen_t addr_length = sizeof(client_addr);

    int epoll_fd = epoll_create(MAX_EVENTS);

    if (epoll_fd == -1)
    {
        perror("epoll_create failed\n");
        exit(EXIT_FAILURE);
    }

    using server::Player;
    server::initialize_server(epoll_fd);

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
            // 建立链接
            if (events[i].data.fd == listen_fd)
            {
                if ((conn_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &addr_length)) < 0)
                {
                    perror("accept conn_fd failed");
                    exit(EXIT_FAILURE);
                }
                printf("accept:%d\n", conn_fd);
                shakehands(conn_fd);
                printf("shakerhands\n");
                Player new_player;
                new_player.player_id = conn_fd;
                new_player.socket_fd = conn_fd;
                new_player.room_id = -1;
                new_player.playing = false;
                server::add_player(new_player.player_id, new_player);

                event.events = EPOLLIN; // 监控读取事件
                // event.data.ptr = &head;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, conn_fd, &event) == -1)
                {
                    perror("epoll_ctl:conn_fd register failed");
                    exit(EXIT_FAILURE);
                }
            }
            // 处理请求
            else
            {
                server::ServerRequest(events[i]);
            }
        }
    }
}