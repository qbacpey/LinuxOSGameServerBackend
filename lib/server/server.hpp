#ifndef GAME_SERVER_SERVER_H
#define GAME_SERVER_SERVER_H

#include "../websocket/websocket.h"
#include "../room/room.hpp"
#include <string>
// #include "semaphore.h"
#include <map>
#include <queue>
#include <pthread.h>

namespace server
{
    struct Player
    {
        int player_id; // 暂定就是socketfd
        int socket_fd;
        int room_id;
        bool playing;
    };

    enum class RoomURL
    {
        kCreateRoom,
        kJoinRoom,
        kBroadcastRoom,
        kStartGame,
        kHeartbeat,
        kLogin,
        kUpdateRoomState
    };

    enum class ReadStatus{
        kReadingHeader,
        kReadingBody,
        kClear
    };

    enum class WriteStatus{
        kWriteHeader,
        kClear
    };

    struct WebSocketStatus {
        int conn_fd; // 连接的文件描述符

        std::string read; // 已经读取到的数据，是字符串
        std::string::size_type read_n; // 已读取的数据
        std::string::size_type total_n; // 总体需要读取的数据
        ReadStatus read_status; // 请求读取情况

        std::queue<std::string> responses; // 等待写入的请求
        std::string::size_type write_n; // 当前请求写入情况
        WriteStatus write_status; // 请求写入情况
    };

    using PlayerId = int;

    void initialize_server(int epoll_fd);
    void add_player(PlayerId player_id, Player &player);
    Player &get_player(PlayerId player_id);
    int get_epoll_fd();
    void set_epoll_fd(int epoll_fd);
    // 获取一个新的房间ID，需要从零开始递增
    int get_new_global_room_id();
    void set_global_room_id(int _global_room_id);

    void ResponseNewPlayerId(int host_id);

    
    void add_room(room::RoomId room_id, room::Room &room);
    room::Room &get_room(room::RoomId room_id);

    // 请求处理函数1.0
    void ServerRequest(epoll_event event);
    void ServerRequest(epoll_event event, struct WebSocketStatus *status);

} // namespace server

#endif // GAME_SERVER_SERVER_H
