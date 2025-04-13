/// @file server.cpp
/// @brief Серверная часть алгоритма отсечения Сазерленда-Ходжмана

#include <iostream>
#include <stdexcept>
#include <cmath>
#include <sstream>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

/// @enum PointClass
/// @brief Классификация положения точки относительно ребра
enum PointClass { LEFT, RIGHT, BEHIND, BEYOND, BETWEEN, ORIGIN, DESTINATION };

/// @enum Rotation
/// @brief Направление обхода вершин
enum Rotation { CLOCKWISE, COUNTER_CLOCKWISE };

class Edge;

/// @class Point
/// @brief Класс для представления точки в 2D пространстве
class Point {
public:
    double x, y; ///< Координаты точки
    
    /// @brief Конструктор точки
    /// @param x Координата X (по умолчанию 0)
    /// @param y Координата Y (по умолчанию 0)
    Point(double x = 0, double y = 0) : x(x), y(y) {}
    
    /// @brief Оператор сравнения точек
    bool operator==(const Point& other) const {
        return (x - other.x)*(x - other.x) + (y - other.y)*(y - other.y) < 1e-15;
    }
    
    /// @brief Оператор неравенства точек
    bool operator!=(const Point& other) const { return !(*this == other); }
    
    /// @brief Вычитание точек (векторная операция)
    Point operator-(const Point& other) const { return Point(x - other.x, y - other.y); }
    
    /// @brief Длина вектора от начала координат
    double length() const { return std::sqrt(x * x + y * y); }
    
    /// @brief Классификация точки относительно ребра
    /// @param e Ребро для классификации
    PointClass classify(const Edge& e) const;
};

/// @class Edge
/// @brief Класс для представления ребра многоугольника
class Edge {
public:
    Point org; ///< Начальная точка ребра
    Point dest; ///< Конечная точка ребра
    
    /// @brief Конструктор ребра
    /// @param org Начальная точка
    /// @param dest Конечная точка
    Edge(const Point& org, const Point& dest) : org(org), dest(dest) {}
    
    /// @brief Получить точку на ребре по параметру t
    /// @param t Параметр от 0 до 1
    Point point(double t) const { 
        return Point(org.x + t*(dest.x - org.x), org.y + t*(dest.y - org.y)); 
    }
    
    /// @brief Поиск пересечения с другим ребром
    /// @param e Ребро для проверки пересечения
    /// @param[out] t Параметр точки пересечения
    /// @return true если пересечение существует
    bool intersect(const Edge& e, double& t) const;
};

// Реализация методов класса Point
PointClass Point::classify(const Edge& e) const {
    Point a = e.dest - e.org;
    Point b = *this - e.org;
    double sa = a.x * b.y - b.x * a.y;
    if (sa > 0.0) return LEFT;
    if (sa < 0.0) return RIGHT;
    if ((a.x * b.x < 0.0) || (a.y * b.y < 0.0)) return BEHIND;
    if (a.length() < b.length()) return BEYOND;
    if (e.org == *this) return ORIGIN;
    if (e.dest == *this) return DESTINATION;
    return BETWEEN;
}

// Реализация методов класса Edge
bool Edge::intersect(const Edge& e, double& t) const {
    double a = dest.x - org.x, b = e.org.x - e.dest.x;
    double c = dest.y - org.y, d = e.org.y - e.dest.y;
    double denom = a * d - b * c;
    if (std::abs(denom) < 1e-15) return false;
    double e1 = e.org.x - org.x, e2 = e.org.y - org.y;
    t = (e1 * d - e2 * b) / denom;
    double u = (e1 * c - e2 * a) / denom;
    return t >= 0 && t <= 1 && u >= 0 && u <= 1;
}

/// @class Vertex
/// @brief Вершина многоугольника с указателями на соседей
class Vertex : public Point {
public:
    Vertex* _next; ///< Следующая вершина (по часовой стрелке)
    Vertex* _prev; ///< Предыдущая вершина (против часовой стрелки)
    
    /// @brief Основной конструктор
    /// @param x Координата X
    /// @param y Координата Y
    Vertex(double x = 0, double y = 0) : Point(x, y), _next(nullptr), _prev(nullptr) {}
    
    /// @brief Конструктор из точки
    /// @param p Исходная точка
    Vertex(const Point& p) : Point(p), _next(nullptr), _prev(nullptr) {}
    
    /// @brief Получить следующую вершину
    Vertex* cw() { return _next; }
    
    /// @brief Получить предыдущую вершину
    Vertex* ccw() { return _prev; }
    
    /// @brief Получить соседа по направлению
    /// @param rotation Направление (CLOCKWISE/COUNTER_CLOCKWISE)
    Vertex* neighbor(int rotation) { return (rotation == CLOCKWISE) ? cw() : ccw(); }
    
    /// @brief Вставить вершину после текущей
    /// @param v Вершина для вставки
    Vertex* insert(Vertex* v) {
        v->_next = _next;
        v->_prev = this;
        if (_next) _next->_prev = v;
        _next = v;
        return v;
    }
    
    /// @brief Удалить текущую вершину
    Vertex* remove() {
        if (_prev) _prev->_next = _next;
        if (_next) _next->_prev = _prev;
        _next = _prev = nullptr;
        return this;
    }
};

/// @class Polygon
/// @brief Класс для представления многоугольника
class Polygon {
public:
    Vertex* _v; ///< Текущая вершина (окно)
    int _size;  ///< Количество вершин
    
    /// @brief Основной конструктор
    Polygon() : _v(nullptr), _size(0) {}
    
    /// @brief Конструктор копирования
    /// @param other Исходный многоугольник
    Polygon(const Polygon& other) : _v(nullptr), _size(0) {
        if (other._v) {
            Vertex* current = other._v;
            do {
                insert(*current);
                current = current->cw();
            } while (current != other._v);
        }
    }
    
    /// @brief Деструктор
    ~Polygon() {
        if (_v) {
            Vertex* w = _v->cw();
            while (w != _v) {
                delete w->remove();
                w = _v->cw();
            }
            delete _v;
        }
    }
    
    /// @brief Получить текущее ребро
    /// @throws std::runtime_error если многоугольник пуст
    Edge edge() {
        if (!_v) throw std::runtime_error("Polygon is empty");
        return Edge(*_v, *_v->cw());
    }
    
    /// @brief Добавить вершину в многоугольник
    /// @param p Точка для добавления
    void insert(const Point& p) {
        Vertex* v = new Vertex(p);
        if (!_v) {
            _v = v;
            _v->_next = _v->_prev = _v;
        } else _v->insert(v);
        _size++;
    }
    
    /// @brief Получить текущую точку
    Point getPoint() const { return *_v; }
    
    /// @brief Переместить текущую вершину
    /// @param rotation Направление перемещения
    void advance(int rotation) { _v = _v->neighbor(rotation); }
    
    /// @brief Получить количество вершин
    int size() const { return _size; }
    
    /// @brief Получить следующую вершину
    Vertex* cw() { return _v->cw(); }
};

/// @brief Отсечение многоугольника одним ребром
/// @param s Исходный многоугольник
/// @param e Ребро отсечения
/// @param result Результат отсечения
/// @return true если результат не пуст
bool clipPolygonToEdge(Polygon& s, Edge& e, Polygon*& result) {
    Polygon* p = new Polygon();
    for (int i = 0; i < s.size(); s.advance(CLOCKWISE), i++) {
        Point org = s.getPoint(), dest = *s.cw();
        bool orgInside = (org.classify(e) != LEFT), destInside = (dest.classify(e) != LEFT);
        if (orgInside != destInside) {
            double t;
            e.intersect(s.edge(), t);
            Point cross = e.point(t);
            if (orgInside) p->insert(cross);
            else { p->insert(cross); p->insert(dest); }
        } else if (orgInside) p->insert(dest);
    }
    result = p;
    return p->size() > 0;
}

/// @brief Основная функция отсечения
/// @param s Исходный многоугольник
/// @param p Отсекающий многоугольник
/// @param result Результат отсечения
/// @return true если отсечение прошло успешно
bool clipPolygon(Polygon& s, Polygon& p, Polygon*& result) {
    Polygon* q = new Polygon(s);
    for (int i = 0; i < p.size(); p.advance(CLOCKWISE), i++) {
        Edge e = p.edge();
        Polygon* r;
        if (!clipPolygonToEdge(*q, e, r)) {
            delete q;
            return false;
        }
        delete q;
        q = r;
    }
    result = q;
    return true;
}

/// @brief Основная функция сервера
int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in address{AF_INET, htons(8080), INADDR_ANY};
    bind(server_fd, (sockaddr*)&address, sizeof(address));
    listen(server_fd, 3);
    std::cout << "Server listening on port 8080..." << std::endl;

    while (true) {
        sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int client_sock = accept(server_fd, (sockaddr*)&client_addr, &addr_len);
        std::cout << "Client connected" << std::endl;
        
        std::string data;
        char buffer[1024];
        ssize_t bytes_read;
        while ((bytes_read = recv(client_sock, buffer, sizeof(buffer), 0))) {
            data.append(buffer, bytes_read);
            if (bytes_read < sizeof(buffer)) break;
        }

        std::istringstream iss(data);
        try {
            int s_size, p_size;
            iss >> s_size;
            Polygon s;
            for (int i = 0; i < s_size; ++i) {
                double x, y;
                iss >> x >> y;
                s.insert(Point(x, y));
            }
            iss >> p_size;
            Polygon p;
            for (int i = 0; i < p_size; ++i) {
                double x, y;
                iss >> x >> y;
                p.insert(Point(x, y));
            }
            
            Polygon* result = nullptr;
            std::ostringstream response;
            if (clipPolygon(s, p, result)) {
                response << "OK\n" << result->size() << "\n";
                Vertex* v = result->_v;
                do {
                    response << v->x << " " << v->y << "\n";
                    v = v->cw();
                } while (v != result->_v);
                delete result;
            } else {
                response << "FAIL\n";
            }
            
            std::string res_str = response.str();
            send(client_sock, res_str.c_str(), res_str.size(), 0);
        } catch (...) {
            send(client_sock, "ERROR\n", 6, 0);
        }
        close(client_sock);
    }
    return 0;
}