#ifndef GAME_SERVER_SERVER_H
#define GAME_SERVER_SERVER_H

#include "lib/room/room.hpp"
#include "websocket/websocket.h"
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
        kHeartbeat
    };

    using PlayerId = int;
    std::map<PlayerId, Player> players{};
    int epoll_fd;

    // 请求处理函数1.0
    void ServerRequest(epoll_event event);

} // namespace server

#endif // GAME_SERVER_SERVER_H
