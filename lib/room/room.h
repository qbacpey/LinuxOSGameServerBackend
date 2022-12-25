#ifndef GAME_SERVER_H
#define GAME_SERVER_H

#include <string>

namespace Room
{
    enum class RoomState
    {
        Waiting,
        Gaming
    };

    class Room
    {
    public:
        // 构造函数
        Room(int roomID, const std::string &roomName, int hostID) : _roomID(roomID),
                                                                    _roomName(roomName),
                                                                    _hostID(hostID)
                                                                    {};

        bool JoinRoom(int guestID);
        bool StartGaming();

        // 下面是各种get函数
        int roomID() const { return _roomID; }
        const std::string &roomName() const { return _roomName; }
        int count() const { return _count; }
        int hostID() const { return _hostID; }
        int guestID() const { return _guestID; }
        const RoomState state() const { return _state; }

    private:
        int _roomID;
        std::string _roomName;
        int _count = 1;
        int _hostID;
        int _guestID = -1;
        RoomState _state = RoomState::Waiting;
    };
} // namespace Room

#endif // GAME_SERVER_H
