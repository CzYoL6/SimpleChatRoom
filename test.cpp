#include<iostream>
#include<ctime>
#include<chrono>

int main(){
    //当前的时间点
    std::chrono::system_clock::time_point tp = std::chrono::system_clock::now();
    //转换为time_t类型
    std::time_t t = std::chrono::system_clock::to_time_t(tp); 
    //转换为字符串
    std::string now = ctime(&t);
    //去除末尾的换行
    now.resize(now.size() - 1); 
    std::cout << now << std::endl;
    return 0;
}