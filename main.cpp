#include"./shc/DatabaseIF.h"
#include <string >
#include <stack>
# include <iostream>

struct placeInfo {
    int number;//场所数量
    int distance[500];//各场所到当前场所的距离数组
    std:: string ** names;//场所名称的数组
    std:: string ** labels;//场所类别的数组
    std:: string ** introductions;//场所介绍的数组
};



struct path{
    Place ** places;
    int num_places;
};

class interface_place_navigation{  //路径选择模块的接口类
public:

    virtual void init_toursite(int index) =0;
    virtual void set_current_place(int index) =0;//设定当前场所的索引；
    virtual struct path get_route(int destination)=0;//通过目标场所的索引获得最短路径，用最短路径算法实现；
    virtual struct path get_route(int destinations[] , int cnt)=0;//通过目标场所的索引和目标场所数量来获得最短路径，用最短路径算法实现；
};




class implement_place_navigation : public interface_place_navigation {
public:
    int current_place;
    Database_IF * database;
    struct ToursiteTopo a;
    struct path result_path;

    implement_place_navigation( Database_IF * this_database ){
        this->database = this_database;
    }

    void init_toursite(int index) override {
        this->a = *(database->get_toursite_topo(index));
    }

    void set_current_place(int index) override {
        current_place = index;
    }

    struct path get_route(int destination) override {
        int* dist = new int[a.place_num]; // 初始化距离数组
        int* prev = new int[a.place_num]; // 初始化前驱节点数组
        bool* visited = new bool[a.place_num]; // 初始化访问状态数组

        for (int i = 0; i < a.place_num; ++i) {
            dist[i] = INT_MAX;
            prev[i] = -1;
            visited[i] = false;
        }
        dist[current_place] = 0;

        for (int i = 0; i < a.place_num - 1; ++i) { // Dijkstra主循环
            int min_dist_vertex = -1;
            int min_dist = INT_MAX;
            for (int j = 0; j < a.place_num; ++j) {
                if (!visited[j] && dist[j] < min_dist) {
                    min_dist = dist[j];
                    min_dist_vertex = j;
                }
            }

            if (min_dist_vertex == -1) break; // 如果找不到未访问的节点，则结束循环

            visited[min_dist_vertex] = true;

            for (int j = 0; j < a.place_num; ++j) {
                if (!visited[j] && a.adjacent_matrix[min_dist_vertex][j] != -1 &&
                    dist[min_dist_vertex] + a.adjacent_matrix[min_dist_vertex][j] < dist[j]) {
                    dist[j] = dist[min_dist_vertex] + a.adjacent_matrix[min_dist_vertex][j];
                    prev[j] = min_dist_vertex;
                }
            }

            if (min_dist_vertex == destination) break; // 如果到达目标地点，提前结束循环
        }

        // 构造路径
        std::stack<int> path_indices;
        int node = destination;
        while (node != -1) {
            path_indices.push(node);
            node = prev[node];
        }

        // 将路径索引转换为Place对象
        result_path.places = new Place*[path_indices.size()];
        int index = 0;
        while (!path_indices.empty()) {
            result_path.places[index++] = a.places[path_indices.top()];
            path_indices.pop();
        }
        result_path.num_places = index; // 假设result_path有一个num_places成员来记录路径长度

        // 释放动态分配的内存
        delete[] dist;
        delete[] prev;
        delete[] visited;

        return result_path;
    }

    struct path get_route(int destinations[] , int cnt) override {
        // 初始化结果路径
        result_path.places = new Place*[a.place_num];
        result_path.num_places = 0;

        // 遍历目标场所索引数组
        for (int i = 0; i < cnt; ++i) {
            // 如果当前目标场所是出发场所，则跳过
            if (destinations[i] == current_place) {
                continue;
            }

            // 获取从当前场所到目标场所的路径
            struct path sub_path = get_route(destinations[i]);

            // 将子路径添加到结果路径中
            for (int j = 0; j < sub_path.num_places; ++j) {
                result_path.places[result_path.num_places++] = sub_path.places[j];
            }
        }

        // 添加回到出发场所的路径
        struct path return_path = get_route(current_place);
        for (int i = 0; i < return_path.num_places; ++i) {
            result_path.places[result_path.num_places++] = return_path.places[i];
        }

        return result_path;
    }


};





class interface_place_choosing{//场所推荐接口类
public:

    virtual void init_toursite(int index) =0;
    virtual void set_current_place(int index) =0;//设定当前场所
    virtual int get_number_of_places(int index) =0;//返回该景点的场所数
    virtual struct placeInfo get_nearby_places(int index)=0;//用查找算法查找索引为index的场所周围的场所，并用冒泡排序算法按距离排序；
    virtual struct placeInfo filter_nearby_places(std :: string label)=0;//按类别过滤附近的场所；
    virtual struct placeInfo get_nearby_places_by_label(int index,std :: string label )=0;//用查找算法查找索引为index的场所周围的类别为label场所，并用冒泡排序算法按距离排序

};

class implement_place_choosing : public interface_place_choosing {//场所推荐实现类
public:

    int current_place;
    Database_IF * database ;
    struct ToursiteTopo a;
    struct placeInfo info;

    implement_place_choosing(Database_IF * this_database) {
        this-> database = this_database;
    }

    void init_toursite(int index) override {
        this->a =  *(database-> get_toursite_topo(index));
    }

    void set_current_place(int index) override {
        current_place = index;
    }

    int get_number_of_places(int index) override {
        return a.place_num;
    }

    struct placeInfo get_nearby_places(int index) override {
        //用查找算法查找索引为index的场所周围的场所，并用冒泡排序算法按距离排序；
        info.number = 0;
        for (int i = 0; i < a.place_num; i++) {
            if (a.adjacent_matrix[index][i] != -1) {
                info.number++;
                info.distance[info.number - 1] = a.adjacent_matrix[index][i];
                info.names[info.number - 1] = a.places[i]->get_name();
                info.labels[info.number - 1] = a.places[i]->get_label();
                info.introductions[info.number - 1] = a.places[i]->get_introduction(); // 假设没有介绍信息
            }
        }
        // 使用冒泡排序算法按距离排序
        for (int i = 0; i < info.number - 1; i++) {
            for (int j = 0; j < info.number - 1 - i; j++) {
                if (info.distance[j] > info.distance[j + 1]) {
                    std::swap(info.distance[j], info.distance[j + 1]);
                    std::swap(info.names[j], info.names[j + 1]);
                    std::swap(info.labels[j], info.labels[j + 1]);
                    std::swap(info.introductions[j], info.introductions[j + 1]);
                }
            }
        }
        return info; // 返回填充好的info
    }

    struct placeInfo filter_nearby_places(std::string label) override {
        //按类别过滤附近的场所；
        info.number = 0;
        for (int i = 0; i < a.place_num; i++) {
            if (a.adjacent_matrix[current_place][i] != -1 && *a.places[i]->get_label() == label) {
                info.number++;
                info.distance[info.number - 1] = a.adjacent_matrix[current_place][i];
                info.names[info.number - 1] = a.places[i]->get_name();
                info.labels[info.number - 1] = a.places[i]->get_label();
                info.introductions[info.number - 1] = a.places[i]->get_introduction(); // 假设没有介绍信息
            }
        }
        // 使用冒泡排序算法按距离排序
        for (int i = 0; i < info.number - 1; i++) {
            for (int j = 0; j < info.number - 1 - i; j++) {
                if (info.distance[j] > info.distance[j + 1]) {
                    std::swap(info.distance[j], info.distance[j + 1]);
                    std::swap(info.names[j], info.names[j + 1]);
                    std::swap(info.labels[j], info.labels[j + 1]);
                    std::swap(info.introductions[j], info.introductions[j + 1]);
                }
            }
        }
        return info; // 返回填充好的info
    }

    struct placeInfo get_nearby_places_by_label(int index, std::string label) override {
        //用查找算法查找索引为index的场所周围的类别为label场所，并用冒泡排序算法按距离排序
        info.number = 0;
        for (int i = 0; i < a.place_num; i++) {
            if (a.adjacent_matrix[index][i] != -1 && *a.places[i]->get_label() == label) {
                info.number++;
                info.distance[info.number - 1] = a.adjacent_matrix[index][i];
                info.names[info.number - 1] = a.places[i]->get_name();
                info.labels[info.number - 1] = a.places[i]->get_label();
                info.introductions[info.number - 1] = a.places[i]->get_introduction(); // 假设没有介绍信息
            }
        }
        // 使用冒泡排序算法按距离排序
        for (int i = 0; i < info.number - 1; i++) {
            for (int j = 0; j < info.number - 1 - i; j++) {
                if (info.distance[j] > info.distance[j + 1]) {
                    std::swap(info.distance[j], info.distance[j + 1]);
                    std::swap(info.names[j], info.names[j + 1]);
                    std::swap(info.labels[j], info.labels[j + 1]);
                    std::swap(info.introductions[j], info.introductions[j + 1]);
                }
            }
        }
        return info; // 返回填充好的info
    }
};


int main () {
    std:: cout << "test" <<std::endl;
}
