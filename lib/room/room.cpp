#include "room.h"
#include <iostream>

bool Room::Room::JoinRoom(int guestID){
    this->_guestID = guestID;
    this->_count++;
    return true;
}

bool Room::Room::StartGaming(){
    if(this->_guestID == -1) {
        std::cout << "尚未设置guestID，RoomID为：" << this->_roomID << std::endl;
        return false;
    }
    if(this->_state == RoomState::Gaming) {
        std::cout << "游戏已经开始，RoomID为：" << this->_roomID << std::endl;
        return false;
    }
    this->_state = RoomState::Gaming;
}