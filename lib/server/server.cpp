#include "server.hpp"
#include <iostream>

using namespace room;

std::map<server::PlayerId, server::Player> *players;
std::map<room::RoomId, room::Room> *rooms;
int epoll_instance_fd;
// TODO 需要是线程安全的，到时候再上保护
int global_room_id = 0;

void server::initialize_server(int epoll_fd)
{
    players = new std::map<PlayerId, Player>;
    rooms = new std::map<RoomId, Room>;
    epoll_instance_fd = epoll_fd;
}
void server::add_room(room::RoomId room_id, room::Room &room)
{
    rooms->insert({room_id, room});
}
room::Room &server::get_room(room::RoomId room_id)
{
    return rooms->at(room_id);
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

// 将webSocket中的数据读取到payload_data中
bool ParseRequest(epoll_event *event, char *payload_data);
bool ProressRequest(epoll_event *event, char *payload_data);
void BroadCastRooms();

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
    // 用户socket_fd实际就是用户的ID，因此可以直接用
    int host_id = event->data.fd;
    if (!data)
    {
        printf("Error before:[%s]\n", cJSON_GetErrorPtr());
    }
    else
    {
        using room::Room;
        using server::RoomURL;
        using std::string;
        RoomURL type = server::RoomURL(cJSON_GetObjectItem(data, "type")->valueint);
        switch (type)
        {
        case RoomURL::kCreateRoom:
        {
            printf("kCreateRoom\n");
            string roomName(cJSON_GetStringValue(cJSON_GetObjectItem(cJSON_GetObjectItem(data, "body"), "roomName")));
            Room new_room(server::get_new_global_room_id(), roomName, host_id);
            server::add_room(new_room.room_id(), new_room);
            server::get_player(host_id).room_id = new_room.room_id();
            BroadCastRooms();
            return true;
        }
        case RoomURL::kJoinRoom:
        {
            printf("kJoinRoom\n");
            return true;
        }
        case RoomURL::kStartGame:
        {
            printf("kStartGame\n");
            return true;
        }
        case RoomURL::kHeartbeat:
        {
            printf("kHeartbeat\n");
            return true;
        }
        default:
        {
            perror("Unknown request type\n");
            exit(-1);
        }
        }
    }
}

bool ParseRequest(epoll_event *event, char *payload_data)
{
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
    return true;
}

// 遍历players，向所有玩家发送消息
void BroadCastRooms()
{
    cJSON *response = cJSON_CreateObject();
    cJSON_AddNumberToObject(response, "type", double(server::RoomURL::kBroadcastRoom));
    cJSON *rooms_response = cJSON_CreateArray();
    cJSON_AddItemToObject(response, "rooms", rooms_response);
    // 生成响应
    for (auto &iter : *rooms)
    {
        cJSON *item = cJSON_CreateObject();
        cJSON_AddNumberToObject(item, "room_id", iter.second.room_id());
        cJSON_AddStringToObject(item, "room_name", iter.second.room_name().c_str());
        cJSON_AddNumberToObject(item, "count", iter.second.count());
        cJSON_AddNumberToObject(item, "host_id", iter.second.host_id());
        cJSON_AddNumberToObject(item, "guest_id", iter.second.guest_id());
        cJSON_AddNumberToObject(item, "state", double(iter.second.state()));
        cJSON_AddItemToArray(rooms_response, item);
    }
    // 广播房间给所有玩家
    for (auto &iter : *players)
    {
        if (send_msg(iter.second.socket_fd, cJSON_PrintUnformatted(response)) == -1)
        {
            perror("BroadCastRooms fail!");
            exit(-1);
        }
    }
}