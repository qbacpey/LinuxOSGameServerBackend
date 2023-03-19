#include <csignal>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/sendfile.h>
#include "lib/server/server.hpp"
#include "lib/websocket/websocket.h"
#define THREAD_NUM 3
void setnonblocking(int fd);
struct thread_args
{
    int listenfd;
    int epollfd;
};

void *thread(void *args)
{
    using server::Player;
    int conn_fd;

    struct epoll_event *events = (struct epoll_event *)malloc(sizeof(struct epoll_event) * MAX_EVENTS);
    if (events == NULL)
    {
        perror("malloc");
    }
    struct epoll_event event;
    int epoll_fd = ((struct thread_args *)args)->epollfd;
    int listen_fd = ((struct thread_args *)args)->listenfd;

    struct sockaddr_in client_addr;
    socklen_t addr_length = sizeof(client_addr);

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
                    if (errno == EAGAIN | errno == EWOULDBLOCK)
                    {
                        break;
                    }
                    else
                    {
                        perror("accept");
                        break;
                    }
                }
                shakehands(conn_fd);
                setnonblocking(conn_fd);
                Player new_player;
                new_player.player_id = conn_fd;
                new_player.socket_fd = conn_fd;
                new_player.room_id = -1;
                new_player.playing = false;
                server::add_player(new_player.player_id, new_player);

                event.events = EPOLLIN | EPOLLET | EPOLLONESHOT; // 监控读取事件
                event.data.fd = conn_fd;
                // event.data.ptr = &head;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, conn_fd, &event) == -1)
                {
                    perror("thread::epoll_ctl fail");
                    exit(EXIT_FAILURE);
                }
                // 链接建立之后响应其id
                server::ResponseNewPlayerId(conn_fd);
            }
            // 处理请求
            else
            {
                server::ServerRequest(events[i]);
            }
        }
    }
}

int main()
{
    int listen_fd = passive_server(8008, 20);

    struct sockaddr_in client_addr;
    socklen_t addr_length = sizeof(client_addr);

    int epoll_fd = epoll_create(MAX_EVENTS);

    if (epoll_fd == -1)
    {
        perror("main::epoll_create failed\n");
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
        perror("main::epoll_ctl fail\n");
        exit(EXIT_FAILURE);
    }

    pthread_t threads[THREAD_NUM];

    struct thread_args targs;
    targs.epollfd = epoll_fd;
    targs.listenfd = listen_fd;

    for (int i = 0; i < THREAD_NUM; ++i)
    {
        if (pthread_create(&threads[i], NULL, thread, &targs) < 0)
        {
            fprintf(stderr, "error while creating %d thread\n", i);
            exit(1);
        }
    }
    printf("listen:%d\n", listen_fd);
    thread(&targs);
    close(epoll_fd);
    close(listen_fd);
}

void setnonblocking(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
      
    fcntl(fd, F_SETFL, new_option);
}