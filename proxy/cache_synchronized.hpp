#ifndef __CACHE_SYN__
#define __CACHE_SYN__
#include <mutex> 
#include <iostream>
#include <unordered_map>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <shared_mutex>
#include <thread>


template<typename K, typename T>
class BiNode{
public:
    K key; 
    T val;
    int expired_ts; // timestamp
    BiNode *prev;
    BiNode *next;
    //BiNode(): key(K()), val(T()), expired_ts(0), prev(nullptr), next(nullptr){};
    BiNode(K k, T t, int exp): key(k), val(t), expired_ts(exp), prev(nullptr), next(nullptr){};
    BiNode(K k, T t, int exp, BiNode*p, BiNode*n): key(k), val(t), expired_ts(exp), prev(p), next(n){};
};

template<typename K, typename T>
class CacheSYN{
    private:
        int capacity;
        int size;
        BiNode<K, T>* head = nullptr;
        BiNode<K, T> * tail = nullptr;
        std::unordered_map<K, BiNode<K, T>*> h;
        void add_node_to_head(BiNode<K, T> *node);
        void remove_node_from_list(BiNode<K, T> *node);
        void remove_last_node();
        // std:: mutex mutex_;
        mutable std::shared_mutex mutex_;
       
    public:
        CacheSYN(int cap);
        CacheSYN(int cap, K init_k, T init_t);
        ~CacheSYN();
        void set(K key, T data, int expired);
        bool has_key(K key);
        std::pair <T, int> getWithExpiredTime(K key); // return (data, timestamp)
        std::pair<T, bool> get(K key); // return: (data, is_expired)
        
        void refresh(K key, int expired);

        void delete_key(K key);

        void clean();
        friend std::ostream &operator<<(std::ostream & s, const CacheSYN<K, T> & cache){
            std:: shared_lock<std::shared_mutex> lock(cache.mutex_);
            BiNode<K, T> * cur = cache.head;
            while(cur){
                if (cur == cache.head) {
                    cur = cur -> next;
                    continue;
                } else if (cur == cache.tail) {
                    std::cout << "end" << std::endl;
                    break;
                }
                s << "Key: " << cur->key << "  Value: " << cur->val;
                cur = cur -> next;
                if(cur) s << " -> ";
            }
            return s;
        }
};

#endif