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

        friend std::ostream & operator << (std::ostream & s, const CacheSYN<K, T> & cache){
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

template <typename K, typename T>
std::pair<T,int> CacheSYN<K, T>::getWithExpiredTime(K key){
    std:: unique_lock<std::shared_mutex> lock(mutex_);
    //std:: cout<< "get: " << key << std::endl;
    if(h.find(key) != h.cend()){
        BiNode<K, T> *node = h[key];
        bool is_expired = (std::time(0) > node->expired_ts);
        if(!is_expired){ 
            add_node_to_head(node);
        }
        return std::make_pair(node->val, node->expired_ts);
    }
    return std::make_pair(T(), -1);
}

template<typename K, typename T>
void CacheSYN<K, T>::clean() {
    int t = std::time(0);
    std:: unique_lock<std::shared_mutex> lock(mutex_);
    BiNode<K, T> *node = tail->prev;
    BiNode<K, T> *prev = nullptr;
    while (node != head && t > node->expired_ts){
        prev = node->prev;
        remove_node_from_list(node);
        delete node;
        h.erase(node->key);
        size -= 1;
        node = prev;
    }
}


template <typename K, typename T>
void CacheSYN<K, T>:: delete_key(K key){
    std:: unique_lock<std::shared_mutex> lock(mutex_);
    BiNode<K, T> *node = h[key];
    remove_node_from_list(node);
    delete node;
    h.erase(key);
    size -= 1;
}


template<typename K, typename T>
void CacheSYN<K, T>::refresh(K key, int expired){
    std:: unique_lock<std::shared_mutex> lock(mutex_);
    if(h.find(key) != h.cend()){
        BiNode<K, T> *cur = h[key];
        set(key, cur->val, expired);
    }
}

template<typename K, typename T>
bool CacheSYN<K, T>::has_key(K key){
    std:: shared_lock<std::shared_mutex> lock(mutex_);
    return (h.count(key) > 0); 
}

template<typename K, typename T>
void CacheSYN<K, T>:: remove_last_node(){
    if(size  > 0){
        BiNode<K, T> *lnode= tail->prev;
        remove_node_from_list(lnode);
        K key = lnode->key;
        delete lnode;
       // std:: cout<<"remove:" << key << std::endl;
        h.erase(key);
        size -= 1;
    }
}

template<typename K, typename T>
void CacheSYN<K, T>:: remove_node_from_list(BiNode<K, T> *node){
    if(node){
        node->prev->next = node->next;
        node->next->prev = node->prev;
        node->next = nullptr;
        node->prev = nullptr;
    }
}

template<typename K, typename T>
void CacheSYN<K, T>:: add_node_to_head(BiNode<K,T> *node){
    if(node){
        node->next = head->next;
        node->prev = head;
        head->next->prev = node;
        head->next = node;
    }
}

template<typename K, typename T>
CacheSYN<K, T>:: CacheSYN(int cap, K init_k, T init_t){
    capacity = cap;
    size = 0;
    head = new BiNode<K,T>(init_k, init_t, 0);
    tail = new BiNode<K,T>(init_k, init_t, 0, head, nullptr);
    head->next = tail;
}

template<typename K, typename T>
CacheSYN<K, T>:: ~CacheSYN(){
    BiNode<K,T> * cur = head;
    BiNode<K,T> * nex = nullptr;
    while(cur){
        nex = cur->next;
        delete cur;
        cur = nex;
    }
    h.clear();
}

template<typename K, typename T>
void CacheSYN<K, T>:: set(K key, T data, int expired){
    //std::cout<<"set:" << key << ":" << data << std::endl;
    // std:: unique_lock
    BiNode<K, T> *node = nullptr;
    if(h.find(key) != h.cend()){
        std:: unique_lock<std::shared_mutex> lock(mutex_);
        node = h[key];
        node->val = data;
        node->expired_ts = expired;
        remove_node_from_list(node);
    }else{
        std:: unique_lock<std::shared_mutex> lock(mutex_);
        size += 1;
        node = new BiNode<K,T>(key, data, expired);
        h[key] = node;
        if(size > capacity){
            remove_last_node();
        }
    }
    add_node_to_head(node);
}

template<typename K, typename T>
std::pair<T, bool> CacheSYN<K, T>:: get(K key){
    std:: unique_lock<std::shared_mutex> lock(mutex_);
    //std:: cout<< "get: " << key << std::endl;
    if(h.find(key) != h.cend()){
        BiNode<K, T> *node = h[key];
        // std::cout << "Inside of cache get:\n";
        // std::cout << "Current time: " << std::time(0) << ' ' << "Expiration time: " <<
        // node->expired_ts << std::endl;
        bool is_expired = (std::time(0) > node->expired_ts);
        if(!is_expired){ 
            add_node_to_head(node);
        }
        return std::make_pair(node->val, is_expired);
    }
    return std::make_pair(T(), true);
}

#endif