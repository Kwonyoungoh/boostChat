#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <nlohmann/json.hpp>
#include <mysqlx/xdevapi.h>
#include <iostream>
#include <vector>
#include <set>
#include <mutex>
#include <deque>

using json = nlohmann::json;
using namespace mysqlx;
using namespace boost::asio;

class ChatSession;
class ChatServer;

class ChatServer {
public:
	ChatServer(io_context& io_context, short port, std::size_t thread_pool_size);

	~ChatServer();

	void join(boost::shared_ptr<ChatSession> session);
	void leave(boost::shared_ptr<ChatSession> session);
	void broadcast(const std::string& msg, boost::shared_ptr<ChatSession> sender);
	void save_chat_data(const std::string& steamid, const std::string& _chatdata);
private:
	void start_accept();
	void handle_accept(boost::shared_ptr<ChatSession> new_session, const boost::system::error_code& error);
	Session connect_to_database();
	io_context& io_context_;
	ip::tcp::acceptor acceptor_;
	boost::thread_group threads_;
	std::set<boost::shared_ptr<ChatSession>> clients_;
	std::mutex clients_mutex_;
};

class ChatSession : public boost::enable_shared_from_this<ChatSession> {
public:
	ChatSession(io_context& io_context, ChatServer& server);

	ip::tcp::socket& socket();
	void start();
	void deliver(const std::string& msg);

private:
	void handle_read(const boost::system::error_code& error, std::size_t bytes_transferred);
	void write();
	void handle_write(const boost::system::error_code& error);

	ip::tcp::socket socket_;
	streambuf read_buffer_;
	ChatServer& server_;
	std::deque<std::string> write_msgs_;
};

ChatServer::ChatServer(io_context& io_context, short port, std::size_t thread_pool_size)
	: io_context_(io_context), acceptor_(io_context, ip::tcp::endpoint(ip::address_v4::any(), port)) {
	std::cout<< "ChatServer start" << std::endl;
	start_accept();
	for (std::size_t i = 0; i < thread_pool_size; ++i) {
		threads_.create_thread(boost::bind(&io_context::run, &io_context_));
	}
}

ChatServer::~ChatServer() {
	threads_.join_all();
}

void ChatServer::join(boost::shared_ptr<ChatSession> session) {
	std::unique_lock<std::mutex> lock(clients_mutex_);
	std::cout<< "join"<< session << std::endl;
	clients_.insert(session);
}

void ChatServer::leave(boost::shared_ptr<ChatSession> session) {
	std::unique_lock<std::mutex> lock(clients_mutex_);
	std::cout << "leave" << session << std::endl;
	clients_.erase(session);
}

void ChatServer::broadcast(const std::string& msg, boost::shared_ptr<ChatSession> sender) {
	std::unique_lock<std::mutex> lock(clients_mutex_);
	for (const auto& session : clients_) {

		// 메세지를 보낸 클라이언트는 제외
		if (session != sender) { 
			session->deliver(msg);
		}
	}
}

void ChatServer::start_accept() {
	boost::shared_ptr<ChatSession> new_session(new ChatSession(io_context_, *this));
	acceptor_.async_accept(new_session->socket(),
		boost::bind(&ChatServer::handle_accept, this, new_session,
			placeholders::error));
}

void ChatServer::handle_accept(boost::shared_ptr<ChatSession> new_session, const boost::system::error_code& error) {
	if (!error) {
		new_session->start();
		join(new_session);
		start_accept();
	}
	else {
		std::cerr << "handle_accept error" << error.message() << std::endl;
	}
}

Session ChatServer::connect_to_database() {
	try {
		Session sess("localhost", 3306, "root", "password", "user_info_db");
		return sess;
	}
	catch (const std::exception& e) {
		std::cerr << "Database connection error: " << e.what() << std::endl;
	}
}

void ChatServer::save_chat_data(const std::string& steamid, const std::string& _chatdata) {
	try {
		Session sess = connect_to_database();
		Schema db = sess.getSchema("user_info_db");
		Table chat_data = db.getTable("chat_data");

		chat_data.insert("_steamid", "_chatdata")
			.values(steamid, _chatdata)
			.execute();
	}
	catch (const std::exception& e) {
		std::cerr << "Error executing query: " << e.what() << std::endl;
	}
}

ChatSession::ChatSession(io_context& io_context, ChatServer& server)
	: socket_(io_context), server_(server) {}

ip::tcp::socket& ChatSession::socket() {
	return socket_;
}

void ChatSession::start() {
	async_read_until(socket_, read_buffer_, "\n",
		boost::bind(&ChatSession::handle_read, shared_from_this(),
			placeholders::error,
			placeholders::bytes_transferred));
}

void ChatSession::deliver(const std::string& msg) {
	bool write_in_progress = !write_msgs_.empty();
	write_msgs_.push_back(msg);
	if (!write_in_progress) {
		write();
	}
}

void ChatSession::handle_read(const boost::system::error_code& error, std::size_t bytes_transferred) {
	if (!error) {
		std::string msg;
		std::istream is(&read_buffer_);
		std::getline(is, msg);
		msg += '\n';

		try {
			json received_data = json::parse(msg);

			std::string steamid = received_data["_steamid"].get<std::string>();
			std::string _chatdata = received_data["_chatdata"].get<std::string>();

			std::cout << "Received data: " << msg << std::endl;
			server_.save_chat_data(steamid, _chatdata);
		}
		catch (const std::exception& e) {
			std::cerr << "Error parsing JSON data: " << e.what() << std::endl;
		}

		server_.broadcast(msg, shared_from_this());
		async_read_until(socket_, read_buffer_, "\n",
			boost::bind(&ChatSession::handle_read, shared_from_this(),
				placeholders::error,
				placeholders::bytes_transferred));
	}
	else {
		server_.leave(shared_from_this());
		std::cerr << "handle_read error" << error.message() << std::endl;
	}
}

void ChatSession::write() {
	async_write(socket_, buffer(write_msgs_.front()),
		boost::bind(&ChatSession::handle_write, shared_from_this(),
			placeholders::error));
}

void ChatSession::handle_write(const boost::system::error_code& error) {
	if (!error) {
		write_msgs_.pop_front();
		if (!write_msgs_.empty()) {
			write();
		}
	}
	else {
		server_.leave(shared_from_this());
	}
}