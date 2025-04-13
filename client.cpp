/// @file client.cpp
/// @brief Клиент для взаимодействия с сервером отсечения

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sstream>

/// @brief Основная функция клиента
int main() {
    // Создание сокета
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "Socket creation error" << std::endl;
        return 1;
    }

    // Настройка адреса сервера
    sockaddr_in serv_addr{AF_INET, htons(8080)};
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    // Подключение к серверу
    if (connect(sock, (sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Connection failed" << std::endl;
        return 1;
    }

    // Формирование и отправка данных
std::ostringstream oss;
    int s_size, p_size;
    std::cout << "Введите размер первого полигона: ";
    std::cin >> s_size;
    oss << s_size << " ";
    std::cout << "Введите координаты вершин первого полигона (x,y):\n";
    for (int i = 0; i < s_size; ++i) {
        double x, y;
        std::cin >> x >> y;
        oss << x << " " << y << " ";
    }
    std::cout << "Введите размер второго полигона: ";
    std::cin >> p_size;
    oss << p_size << " ";
    std::cout << "Введите координаты вершин второго полигона (x,y):\n";
    for (int i = 0; i < p_size; ++i) {
        double x, y;
        std::cin >> x >> y;
        oss << x << " " << y << " ";
    }
    std::string data = oss.str();
    send(sock, data.c_str(), data.size(), 0);

    // Получение и обработка ответа
    char buffer[1024];
    std::string response;
    ssize_t bytes_read;
    while ((bytes_read = recv(sock, buffer, sizeof(buffer), 0)) > 0) {
        response.append(buffer, bytes_read);
    }

    std::istringstream iss(response);
    std::string status;
    iss >> status;
    
    if (status == "OK") {
        int size;
        iss >> size;
        std::cout << "Clipped polygon (" << size << " vertices):\n";
        for (int i = 0; i < size; ++i) {
            double x, y;
            iss >> x >> y;
            std::cout << "(" << x << ", " << y << ")\n";
        }
    } else {
        std::cout << "Clipping failed" << std::endl;
    }

    close(sock);
    return 0;
}