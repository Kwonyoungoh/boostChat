//#include "ChatServer.h"
//
//ChatServer::ChatServer(io_context& io_context, short port, std::size_t thread_pool_size)
//	: io_context_(io_context), acceptor_(io_context, ip::tcp::endpoint(ip::tcp::v4(), port)) {
//	start_accept();
//	for (std::size_t i = 0; i < thread_pool_size; ++i) {
//		threads_.create_thread(boost::bind(&io_context::run, &io_context_));
//	}
//}
//
//ChatServer::~ChatServer() {
//	threads_.join_all();
//}
//
//void ChatServer::join(boost::shared_ptr<ChatSession> session) {
//	std::unique_lock<std::mutex> lock(clients_mutex_);
//	clients_.insert(session);
//}
//
//void ChatServer::leave(boost::shared_ptr<ChatSession> session) {
//	std::unique_lock<std::mutex> lock(clients_mutex_);
//	clients_.erase(session);
//}
//
//void ChatServer::broadcast(const std::string& msg) {
//	std::unique_lock<std::mutex> lock(clients_mutex_);
//	for (const auto& session : clients_) {
//		session->deliver(msg);
//	}
//}
//
//void ChatServer::start_accept() {
//	boost::shared_ptr<ChatSession> new_session(new ChatSession(io_context_, *this));
//	acceptor_.async_accept(new_session->socket(),
//		boost::bind(&ChatServer::handle_accept, this, new_session,
//			placeholders::error));
//}
//
//void ChatServer::handle_accept(boost::shared_ptr<ChatSession> new_session, const boost::system::error_code& error) {
//	if (!error) {
//		new_session->start();
//		join(new_session);
//		start_accept();
//	}
//}
//
//ChatSession::ChatSession(io_context& io_context, ChatServer& server)
//	: socket_(io_context), server_(server) {}
//
//ip::tcp::socket& ChatSession::socket() {
//	return socket_;
//}
//
//void ChatSession::start() {
//	async_read_until(socket_, read_buffer_, "\n",
//		boost::bind(&ChatSession::handle_read, shared_from_this(),
//			placeholders::error,
//			placeholders::bytes_transferred));
//}
//
//void ChatSession::deliver(const std::string& msg) {
//	bool write_in_progress = !write_msgs_.empty();
//	write_msgs_.push_back(msg);
//	if (!write_in_progress) {
//		write();
//	}
//}
//
//void ChatSession::handle_read(const boost::system::error_code& error, std::size_t bytes_transferred) {
//	if (!error) {
//		std::string msg;
//		std::istream is(&read_buffer_);
//		std::getline(is, msg);
//		msg += '\n';
//		server_.broadcast(msg);
//		async_read_until(socket_, read_buffer_, "\n",
//			boost::bind(&ChatSession::handle_read, shared_from_this(),
//				placeholders::error,
//				placeholders::bytes_transferred));
//	}
//	else {
//		server_.leave(shared_from_this());
//	}
//}
//
//void ChatSession::write() {
//	async_write(socket_, buffer(write_msgs_.front()),
//		boost::bind(&ChatSession::handle_write, shared_from_this(),
//			placeholders::error));
//}
//
//void ChatSession::handle_write(const boost::system::error_code& error) {
//	if (!error) {
//		write_msgs_.pop_front();
//		if (!write_msgs_.empty()) {
//			write();
//		}
//	}
//	else {
//		server_.leave(shared_from_this());
//	}
//}
