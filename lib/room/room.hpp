#ifndef GAME_SERVER_ROOM_H
#define GAME_SERVER_ROOM_H

#include <string>

namespace Room
{
    enum class RoomState
    {
        kWaiting,
        kGaming
    };

    // 获取一个新的房间ID，需要从零开始递增
    int FetchNewRoomID();
    int GlobalRoomID = 0;

    class Room
    {
    public:
        // 构造函数
        Room(int room_id, const std::string &room_name, int host_id) : room_id_(room_id),
                                                                    room_name_(room_name),
                                                                    host_id_(host_id)
                                                                    {};

        bool JoinRoom(int guest_id);
        bool StartGaming();

        // 下面是各种get函数
        int room_id() const { return room_id_; }
        const std::string &room_name() const { return room_name_; }
        int count() const { return count_; }
        int host_id() const { return host_id_; }
        int guest_id() const { return guest_id_; }
        const RoomState state() const { return state_; }

    private:
        int room_id_;
        std::string room_name_;
        int count_ = 1;
        int host_id_;
        int guest_id_ = -1;
        RoomState state_ = RoomState::kWaiting;
    };
} // namespace Room

#endif // GAME_SERVER_ROOM_H
