#ifndef __CACHE_CONTROLLER__
#define __CACHE_CONTROLLER__
#include "Cache/cache_synchronized.h"
#include "HTTPModule/HTTPResponse.hpp"
#include "HTTPModule/HTTPRequest.hpp"
#include "HTTPModule/HTTPBase.hpp"
#include <string>
#define CK_VALUE_TRUE "true"
#define CK_VALUE_FALSE "false"
#define DEFAULT_EXPIRED_OFFSET 60*60*24*30 // a month
typedef std::string reasonStr;
// reference: https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Cache-Control
// 其实可以全部保持原来头部格式， 只是Request和Response都是小写因此这里也是小写
typedef struct _CacheControlKey CacheControlKey;
struct _CacheControlKey{
    static const std::string ck_cacheable; // add by cache control
    static const std::string ck_cache_control;
    static const std::string ck_no_cache;
    static const std::string ck_no_store;
    static const std::string ck_s_max_age;
    static const std::string ck_max_age;
    static const std::string ck_private;
    static const std::string ck_public;
    static const std::string ck_expires;
    static const std::string ck_reason;
    static const std::string ck_must_revalidate;
    static const std::string ck_proxy_revalidate;
    // other about E-tag
    static const std::string ck_etag;
    static const std::string ck_last_modified;
    static const std::string ck_if_none_match;
};


// 实际上就是一个Cache中间件
class CacheController{
private:
    // CacheSYN<std::string, HTTPResponse> *ca;
    bool is_shutdown;
    std::pair<bool, reasonStr>  parseCacheControlFields(HTTPBase &base);
    // 并不是json格式， 需要额外写一个方法parse, 并且补充一下默认值
    void parseStrToDict(std::unordered_map<std::string, std::string> &m, std::string raw_data, char sp, char eq,  std::string defaul_str);
    void addCacheableToDict(HTTPBase &base, bool is_cacheable, reasonStr reason);
    void handleEtag(HTTPBase &base); // put all header etag to control fields
public:
    CacheSYN<std::string, HTTPResponse> *ca;
    CacheController(long cap=1000);
    ~CacheController();
    bool closeCache();
    
    // canUseCache: HTTPRequest => bool * reason
    std::pair<bool, reasonStr> canUseCache(HTTPBase & base); // 判断当前资源能不能用
    // isCacheable: HTTPResponse => bool * reason
    std::pair<bool, reasonStr> isCacheable(HTTPResponse &resp);
    std::pair<bool, reasonStr> set(std::string uri, HTTPResponse&resp); // return success or not
    bool isMustRevalidate(HTTPBase &req);
    bool hasKey(std::string key);
    bool hasValidKey(std::string key); // valid means no expired
    std::pair<HTTPResponse, int> get(std::string key); // get node, if not exist, return -1
    std::pair<bool, reasonStr> checkUseProxyCache(HTTPBase &base);
    int getCacheTimeStamp(HTTPResponse &resp);
};

#endif