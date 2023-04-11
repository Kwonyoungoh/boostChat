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

// ����ȭ �Լ�
void serialize(const ChatMessage& message, char* buffer, std::size_t buffer_size) {
	// ����ü�� ũ��� �����͸� ���ʷ� ����ȭ
	std::size_t size = sizeof(ChatMessage);
	std::memcpy(buffer, &size, sizeof(size));
	std::memcpy(buffer + sizeof(size), message.nickname.data(), message.nickname.size());
	std::memcpy(buffer + sizeof(size) + message.nickname.size(), message.steam_id.data(), message.steam_id.size());
	std::memcpy(buffer + sizeof(size) + message.nickname.size() + message.steam_id.size(), message.message.data(), message.message.size());
}

// ������ȭ �Լ�
ChatMessage deserialize(const char* buffer, std::size_t buffer_size) {
	ChatMessage message;

	// �����͸� ���ʷ� ������ȭ�Ͽ� ����ü�� ����
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