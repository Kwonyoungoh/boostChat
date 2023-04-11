#pragma once

#include <string>
#include <cstring>
#include <iostream>

struct ChatMessage {
	std::string nickname;
	std::string steam_id;
	std::string message="";

	ChatMessage(const std::string& nickname = "", const std::string& steam_id = "")
		: nickname(nickname), steam_id(steam_id) {}

	void setMessage (const std::string & message){
		this->message = message;
	}
};

// 직렬화 함수
void serialize(const ChatMessage& message, char* buffer, std::size_t buffer_size) {
	// 구조체의 크기와 데이터를 차례로 직렬화
	std::size_t size = sizeof(ChatMessage);
	std::memcpy(buffer, &size, sizeof(size));
	std::memcpy(buffer + sizeof(size), message.nickname.data(), message.nickname.size());
	std::memcpy(buffer + sizeof(size) + message.nickname.size(), message.steam_id.data(), message.steam_id.size());
	std::memcpy(buffer + sizeof(size) + message.nickname.size() + message.steam_id.size(), message.message.data(), message.message.size());
}

// 역직렬화 함수
ChatMessage deserialize(const char* buffer, std::size_t buffer_size) {
	ChatMessage message;

	// 데이터를 차례로 역직렬화하여 구조체에 저장
	std::size_t size = 0;
	std::memcpy(&size, buffer, sizeof(size));
	std::string nickname(buffer + sizeof(size), buffer + sizeof(size) + size);
	message.nickname = nickname;
	std::string steam_id(buffer + sizeof(size) + size, buffer + sizeof(size) + size + size);
	message.steam_id = steam_id;
	std::string msg(buffer + sizeof(size) + size + size, buffer + buffer_size);
	message.message = msg;

	return message;
}