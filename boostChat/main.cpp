#include "ChatServer.h"

int main() {
	const int port = 50000;
	const std::size_t thread_pool_size = 4; // 스레드 풀 크기 조절

	try {
		io_context io_context;
		ChatServer server(io_context, port, thread_pool_size);
		io_context.run();
	}
	catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
	}

	return 0;
}