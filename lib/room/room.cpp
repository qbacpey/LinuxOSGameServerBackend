#include "room.hpp"
#include <iostream>

bool room::Room::JoinRoom(int guest_id){
    this->guest_id_ = guest_id;
    this->count_++;
    return true;
}

bool room::Room::StartGaming(){
    if(this->guest_id_ == -1) {
        std::cout << "尚未设置guestID，RoomID为：" << this->room_id_ << std::endl;
        return false;
    }
    if(this->state_ == RoomState::kGaming) {
        std::cout << "游戏已经开始，RoomID为：" << this->room_id_ << std::endl;
        return false;
    }
    this->state_ = RoomState::kGaming;
}

// TODO 需要是线程安全的，到时候再上保护
int room::FetchNewRoomID(){
    GlobalRoomID++;
    return GlobalRoomID;
}