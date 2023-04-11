#include "ChatClient.h"

int main() {
	try {
		boost::asio::io_service io_service;

		// Prompt the user to enter the IP address and port number
		std::string ip_address;
		std::cout << "Enter the IP address: ";
		std::cin >> ip_address;

		unsigned short port_number;
		std::cout << "Enter the port number: ";
		std::cin >> port_number;

		ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string(ip_address), port_number);
		ChatClient client(io_service, endpoint);

		boost::thread t(boost::bind(&boost::asio::io_service::run, &io_service));

		// 연결이 완료될 때까지 대기
		while (!client.is_connected()) {
			// 100밀리초 마다 확인
			std::cout << "Connecting..." << std::endl;
			boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
		}

		std::string nickname;
		std::cout << "Enter your nickname: ";
		std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
		while (true) {
			std::getline(std::cin, nickname);
			if (!nickname.empty() && nickname.find_first_not_of(' ') != std::string::npos) {
				break;
			}
			std::cout << "Nickname cannot be empty or only contain spaces. Please enter a valid nickname: ";
		}



		std::string msg;
		while (std::getline(std::cin, msg)) {
			if (msg == "/quit") {
				break;
			}
			client.write(nickname + ": " + msg + "\n");
		}


		client.close();
		t.join();
	}
	catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
	}

	return 0;
}