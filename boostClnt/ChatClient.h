#pragma once

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <functional>
#include <boost/thread.hpp>
#include <iostream>
#include <string>
#include <deque>

using namespace boost::asio;

class ChatClient {
public:
	ChatClient(io_context& io_context, ip::tcp::endpoint endpoint);
	bool is_connected() const { return connected_; }

	void write(const std::string& msg);
	void close();

private:
	void handle_connect(const boost::system::error_code& error);
	void handle_read(const boost::system::error_code& error);
	void do_write(const std::string& msg);
	void handle_write(const boost::system::error_code& error);
	void do_close();

	bool connected_;
	io_context& io_context_;
	ip::tcp::socket socket_;
	streambuf read_buffer_;
	std::deque<std::string> write_msgs_;
};

ChatClient::ChatClient(io_context& io_context, ip::tcp::endpoint endpoint)
	: io_context_(io_context), socket_(io_context) {
	socket_.async_connect(endpoint,
		[this](const boost::system::error_code& error) {
			handle_connect(error);
		});
}

void ChatClient::write(const std::string& msg) {
	io_context_.post(boost::bind(&ChatClient::do_write, this, msg));
}

void ChatClient::close() {
	io_context_.post(boost::bind(&ChatClient::do_close, this));
}

void ChatClient::handle_connect(const boost::system::error_code& error) {
	if (!error) {

		connected_ = true;

		std::cout << "Connected to server!" << std::endl;
		async_read_until(socket_, read_buffer_, "\n",
			boost::bind(&ChatClient::handle_read, this, placeholders::error));
	}
	else {

		connected_ = false;

		std::cout << "Error connecting to server: " << error.message() << std::endl;
	}
}

void ChatClient::handle_read(const boost::system::error_code& error) {
	if (!error) {
		std::string msg;
		std::istream is(&read_buffer_);
		std::getline(is, msg);
		std::cout << msg << std::endl;
		async_read_until(socket_, read_buffer_, "\n",
			boost::bind(&ChatClient::handle_read, this, placeholders::error));
	}
	else {
		do_close();
	}
}

void ChatClient::do_write(const std::string& msg) {
	bool write_in_progress = !write_msgs_.empty();
	write_msgs_.push_back(msg);
	if (!write_in_progress) {
		async_write(socket_, buffer(write_msgs_.front()),
			boost::bind(&ChatClient::handle_write, this, placeholders::error));
	}
}

void ChatClient::handle_write(const boost::system::error_code& error) {
	if (!error) {
		write_msgs_.pop_front();
		if (!write_msgs_.empty()) {
			async_write(socket_, buffer(write_msgs_.front()),
				boost::bind(&ChatClient::handle_write, this, placeholders::error));
		}
	}
	else {
		do_close();
	}
}


void ChatClient::do_close() {
	socket_.close();
}