#include "server.hpp"
#include <iostream>

std::map<server::PlayerId, server::Player> *players;
int epoll_instance_fd;
// TODO 需要是线程安全的，到时候再上保护
int global_room_id = 0;

// 将webSocket中的数据读取到payload_data中
bool ParseRequest(epoll_event *event, char *payload_data);
bool ProressRequest(epoll_event *event, char *payload_data);

void server::initialize_server(int epoll_fd)
{
    players = new std::map<PlayerId, Player>;
    epoll_instance_fd = epoll_fd;
}

void server::add_player(PlayerId player_id, Player &player)
{
    std::cout << "new player connected, player id=" << player.player_id << std::endl;
    players->insert({player_id, player});
}

server::Player &server::get_player(PlayerId player_id)
{
    return players->at(player_id);
}

int server::get_epoll_fd()
{
    return epoll_instance_fd;
}

void server::set_epoll_fd(int epoll_fd)
{
    epoll_instance_fd = epoll_fd;
}
int server::get_new_global_room_id()
{
    return ++global_room_id;
}

void server::set_global_room_id(int _global_room_id)
{
    global_room_id = _global_room_id;
}

// 先不考虑写入中断的情况，必然是EPOLLIN事件
void server::ServerRequest(epoll_event event)
{
    char *payload_data = new char[102400];
    if (false == ParseRequest(&event, payload_data))
    {
        throw("ParseRequest");
    }
    ProressRequest(&event, payload_data);
}

bool ProressRequest(epoll_event *event, char *payload_data)
{
    printf("ProressRequest\n");
    cJSON *data = cJSON_Parse(payload_data);
    cJSON *response = cJSON_CreateObject();
    if (!data)
    {
        printf("Error before:[%s]\n", cJSON_GetErrorPtr());
    }
    else
    {
        using server::RoomURL;
        using std::string;
        RoomURL type = server::RoomURL(cJSON_GetObjectItem(data, "type")->valueint);
        printf("type:%d\n", type);
        switch (type)
        {
        case RoomURL::kCreateRoom:
        {
            printf("kCreateRoom\n");
            string roomName(cJSON_GetStringValue(cJSON_GetObjectItem(cJSON_GetObjectItem(data, "body"), "roomName")));
            std::cout << "roomName: " << roomName << std::endl;
            return true;
        }
        }
    }
}

bool ParseRequest(epoll_event *event, char *payload_data)
{
    printf("receive %d\n", event->data.fd);
    frame_head head;
    int rul = recv_frame_head(event->data.fd, &head);
    if (rul < 0)
    {
        close(event->data.fd);
        return false;
    }
    //                send_frame_head(socketFd, &head);
    // read payload data
    int size = 0;
    do
    {
        int rul;
        rul = read(event->data.fd, payload_data, 1024);
        if (rul <= 0)
            break;
        size += rul;

        umask(payload_data, size, head.masking_key);

    } while (size < head.payload_length);
    if (head.opcode == 0x8)
    {
        printf("socket %d close\n", event->data.fd, payload_data, 1024);
        // TODO deleteFromEpoll(epoll_fd, event->data.fd);
    }
    printf("receive data(%d)\n", head.payload_length);
    return true;
}