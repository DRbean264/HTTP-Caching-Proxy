#include <iostream>
#include "cache_sychronized.h"
#include <thread>
#include <mutex>  
std::mutex mtx;
void putter(int id, CacheSYN<int, int> *ca){
    for(int i=0; i<5; i++){
        ca->set(id*10+i, i*id, i*id);
        std::unique_lock<std::mutex> ulck(mtx);//cout也需要锁去保护, 否则输出乱序
        // std::cout << "id:" << id <<"put "<<i*10+id<<":" <<i*id <<std::endl;
        std::cout <<*ca << std::endl;
    }
}

int main(int argc, char **argv){
    const int worker_num = 20;
    CacheSYN<int, int> ca(5, 1, 1);
    std:: thread worker[worker_num];
    for(int i=0; i < worker_num; i++){
        worker[i]= std::thread(putter, i, &ca);
    }
    for(int i=0; i < worker_num; i++){
        worker[i].join();
    }
    return EXIT_SUCCESS;
}