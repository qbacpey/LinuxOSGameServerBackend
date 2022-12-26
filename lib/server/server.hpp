#ifndef GAME_SERVER_SERVER_H
#define GAME_SERVER_SERVER_H

#include "../websocket/websocket.h"
#include "../room/room.hpp"
#include <string>
// #include "semaphore.h"
#include <map>

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

} // namespace server

#endif // GAME_SERVER_SERVER_H
