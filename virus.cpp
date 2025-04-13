#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sstream>

int main() {

	for(int i = 0; ; i++) {
int sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serv_addr{AF_INET, htons(8080)};
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);
    
    if (connect(sock, (sockaddr*)&serv_addr, sizeof(serv_addr))) {
        std::cerr << "Connection failed\n";
        return 1;
    }

    // Жестко заданные данные
    std::ostringstream oss;
    const int s_size = 3;
    const double s_points[3][2] = {{0,0}, {2,0}, {1,3}};
    
    const int p_size = 3;
    const double p_points[3][2] = {{0,2}, {1,-1}, {2,2}};

    // Формирование сообщения
    oss << s_size << " ";
    for (int i = 0; i < s_size; ++i) {
        oss << s_points[i][0] << " " << s_points[i][1] << " ";
    }
    
    oss << p_size << " ";
    for (int i = 0; i < p_size; ++i) {
        oss << p_points[i][0] << " " << p_points[i][1] << " ";
    }

    std::string data = oss.str();
    if (send(sock, data.c_str(), data.size(), 0) < 0) {
        std::cerr << "Send failed\n";
        close(sock);
        return 1;
    }

    // Получение ответа
    char buffer[1024];
    std::string response;
    ssize_t bytes_read;
    while ((bytes_read = recv(sock, buffer, sizeof(buffer), 0)) > 0) {
        response.append(buffer, bytes_read);
    }

    // Парсинг и вывод результата
    std::istringstream iss(response);
    std::string status;
    iss >> status;
    
    if (status == "OK") {
        int size;
        iss >> size;
        std::cout << "есть пробитие номер " << i <<std::endl;
    } else {
        std::cout << "нет пробития или сервер повержен\n";
    }

    close(sock);
}
    return 0;
}