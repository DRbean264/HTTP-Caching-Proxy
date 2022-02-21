#include "CacheController.hpp"
#include "HTTPModule/HTTPResponse.hpp"
#include "HTTPModule/HTTPRequest.hpp"
#include "HTTPModule/HTTPBase.hpp"
#include "myUtils.hpp"
#include <ctime>
#include <vector>

const std::string CacheControlKey::ck_cacheable = "cacheable";
const std::string CacheControlKey::ck_cache_control = "cache-control";
const std::string CacheControlKey::ck_no_cache = "no-cache";
const std::string CacheControlKey::ck_no_store = "no-store";
const std::string CacheControlKey::ck_s_max_age = "s-maxage";
const std::string CacheControlKey::ck_max_age = "max-age";
const std::string CacheControlKey::ck_private = "private";
const std::string CacheControlKey::ck_public = "public";
const std::string CacheControlKey::ck_expires = "expires";
const std::string CacheControlKey::ck_reason = "_reason";
const std::string CacheControlKey::ck_must_revalidate = "must-revalidate";
const std::string CacheControlKey::ck_proxy_revalidate = "proxy-revalidate";
const std::string CacheControlKey::ck_etag = "etag";
const std::string CacheControlKey::ck_last_modified = "last-modified";
const std::string CacheControlKey::ck_if_none_match = "if-none-match";

CacheController::CacheController(long cap){
    std::cout << "debugging...\n";
    ca = new CacheSYN<std::string, HTTPResponse>(cap, "", HTTPResponse());
    std::cout << "debugging...\n";
    if(!ca){
        std::cerr<<"Initial cache error"<<std::endl;
        exit(EXIT_FAILURE);
    }
    is_shutdown = false;
}

CacheController::~CacheController(){
    delete ca;
}

bool CacheController::hasKey(std::string key){
    return ca->has_key(key); 
}

bool CacheController::closeCache(){
    is_shutdown = true;
    if(ca) ca->clean();
    return true;
}

std::pair<HTTPResponse, int> CacheController::get(std::string key){
    return ca->getWithExpiredTime(key);
}

bool CacheController::hasValidKey(std::string key){
    if(!ca->has_key(key)) return false;
    std::pair<HTTPResponse, bool> res = ca->get(key);
    return !res.second;
}

std::pair<bool, reasonStr> CacheController::set(std::string uri, HTTPResponse &resp){
    // std::pair<bool, reasonStr> cacheable_check = isCacheable(resp);
    std::pair<bool, reasonStr> cacheable_check = canUseCache(resp);
    if(!cacheable_check.first){
        return cacheable_check;
    }
    
    // get expiration time stamp
    int ts = getCacheTimeStamp(resp);
    // if(ts < std::time(0)){
    //     return std::make_pair(false, "response in cache expires");
    // }
    std::cout << "Expires at: " << ts << '\n';
    ca->set(uri, resp, ts);
    return std::make_pair(true, "");
}

void CacheController::addCacheableToDict(HTTPBase &base,  bool is_cacheable, reasonStr reason){
    if(is_cacheable){
        base.cacheControlFields[CacheControlKey::ck_cacheable] = CK_VALUE_TRUE;
        return;
    }
    base.cacheControlFields[CacheControlKey::ck_cacheable] = CK_VALUE_FALSE;
    base.cacheControlFields[CacheControlKey::ck_reason] = reason;
}

bool CacheController::isMustRevalidate(HTTPBase &req){
    std::pair<bool, reasonStr> p = parseCacheControlFields(req);
    // if(!p.first){
    //     std::cerr<< "can't parse the control fields" <<std::endl;
    //     return false;
    // }
    return ((req.cacheControlFields.find(CacheControlKey::ck_no_cache) != req.cacheControlFields.cend()) ||
            (req.cacheControlFields.find(CacheControlKey::ck_max_age) != req.cacheControlFields.cend() && req.cacheControlFields[CacheControlKey::ck_max_age] == "0")||
            // (req.cacheControlFields.find(CacheControlKey::ck_etag) != req.cacheControlFields.cend())||
            (req.cacheControlFields.find(CacheControlKey::ck_must_revalidate) != req.cacheControlFields.cend()) ||
            (req.cacheControlFields.find(CacheControlKey::ck_proxy_revalidate) != req.cacheControlFields.cend()) 
            );
}

void CacheController::parseStrToDict(std::unordered_map<std::string, std::string>&m,std::string raw, char di, char eq,  std::string default_str){
    size_t commaPos = raw.find(di);
    size_t startPos = 0;
    std::string directive;
    while (commaPos != std::string::npos) {
        directive = trimSpace(raw.substr(startPos, commaPos - startPos));
        convertToLower(directive);
        size_t eq_pos = directive.find(eq);
        std::string value = default_str;
        if(eq_pos != std::string::npos){
            value = trimSpace(directive.substr(eq_pos+1, directive.size()-eq_pos-1));
        }
        std::string key = trimSpace(directive.substr(0, eq_pos));
        m[key] = value;
        startPos = commaPos + 1;
        commaPos = raw.find(di, startPos);
    }
    // add the last directive
    directive = trimSpace(raw.substr(startPos, raw.size() - startPos));
    convertToLower(directive);
    size_t eq_pos = directive.find(eq);
    std::string value = default_str;
    if(eq_pos != std::string::npos){
        value = trimSpace(directive.substr(eq_pos+1, directive.size()-eq_pos-1));
    }
    std::string key = trimSpace(directive.substr(0, eq_pos));
    m[key] = value;
}

std::pair<bool, reasonStr> CacheController::checkUseProxyCache(HTTPBase &base){
    if(is_shutdown) return std::make_pair(false, "cache is shutdown");
    std::pair<bool, reasonStr> p = parseCacheControlFields(base);
    if(!p.first) return p;
    // no store, private
    // 为了防止存储差错，本来private是不能存进去的
    std::vector<std::string> no_store_arr = {CacheControlKey::ck_no_store, CacheControlKey::ck_private};
    for(size_t i = 0; i < no_store_arr.size(); i++){
        if(base.cacheControlFields.find(no_store_arr[i]) != base.cacheControlFields.cend()){
            reasonStr reason = "Header contains:" + no_store_arr[i];
            addCacheableToDict(base, false, reason);     
            return std::make_pair(false, reason);       
        }
    }
    return std::make_pair(true, "");
}

// 所有cache control key 参数相关处理
std::pair<bool, reasonStr> CacheController::parseCacheControlFields(HTTPBase &base){
    if(!base.cacheControlFields.empty()) return std::make_pair(true, "");
    std::unordered_map<std::string, std::string> hf = base.getHeaderFields();
    if(hf.find(CacheControlKey::ck_cache_control) == hf.cend()
     && hf.find(CacheControlKey::ck_expires)==hf.cend()
     && hf.find(CacheControlKey::ck_etag) == hf.cend()){
        //base.cacheControlFields[CacheControlKey.ck_cacheable] = CK_VALUE_FALSE;
        // base.cacheControlFields[CacheControlKey.ck_reason] = "lack of cache-control field";
        addCacheableToDict(base, false, "lack of any cache-control field");
        return std::make_pair(false, "lack of any cache-control field");
    }
    if(hf.find(CacheControlKey::ck_etag) != hf.cend()){
        handleEtag(base);
    }
    // if there is only expires, pass expires to cache control level
    if(hf.find(CacheControlKey::ck_expires)!=hf.cend()){
        base.cacheControlFields[CacheControlKey::ck_expires] = hf[CacheControlKey::ck_expires];
    }

    if(hf.find(CacheControlKey::ck_cache_control) == hf.cend()){
        return std::make_pair(true, "");
    }

    char di = ',';
    char eq = '=';
    parseStrToDict(base.cacheControlFields, hf[CacheControlKey::ck_cache_control], di, eq, CK_VALUE_TRUE);
    return std::make_pair(true, "");
}

// 给出的能不能使用cache中的东西只是根据cache-control的字段判断得到的，cache中的data可能过期，
// 需要在上层判断，注意
std::pair<bool, reasonStr> CacheController::canUseCache(HTTPBase &req){
    // determine cacheability
    std::pair<bool, reasonStr> p = checkUseProxyCache(req);
    if(!p.first) return p;

    // determine revalidation
    // if (isMustRevalidate(req)){
    //     return std::make_pair(false, "cache response require revalidate");
    // }
    
    addCacheableToDict(req, true, "");
    return std::make_pair(true, "");
}

std::pair<bool, reasonStr> CacheController::isCacheable(HTTPResponse &resp){
    std::pair<bool, reasonStr> p = checkUseProxyCache(resp);
    if(!p.first) return p;
    // if has parse the cacheable, just return 
    if(resp.cacheControlFields.find(CacheControlKey::ck_cacheable) != resp.cacheControlFields.cend()){
        if(resp.cacheControlFields[CacheControlKey::ck_cacheable] == CK_VALUE_FALSE){
            return std::make_pair(false, resp.cacheControlFields[CacheControlKey::ck_reason]);
        }
        return std::make_pair(true, "");
    }
    addCacheableToDict(resp, true, "");
    return std::make_pair(true, "");
}

void CacheController::handleEtag(HTTPBase &base){
    std::unordered_map<std::string, std::string> hf = base.getHeaderFields();
    if(hf.find(CacheControlKey::ck_etag) != hf.cend()){
        std::vector<std::string> etag_g = {CacheControlKey::ck_etag,
            CacheControlKey::ck_last_modified,
            CacheControlKey::ck_if_none_match};
        
        for(size_t i = 0; i < etag_g.size(); i++){
            if(hf.find(etag_g[i])!=hf.cend()){
                base.cacheControlFields[etag_g[i]] = hf[etag_g[i]];
            }
        }
    }
}

// calculate the response's expiration time stamp for cache storage
int CacheController::getCacheTimeStamp(HTTPResponse &resp){
    struct tm recvTime = resp.getReceiveTime();
    // std::cout << timeToASC(recvTime) << std::endl;

    // time_t rece_t = resp.getReceiveTime().tm_sec;
    time_t rece_t = mktime(&recvTime);
    int expired = rece_t + DEFAULT_EXPIRED_OFFSET; //default expire
    // s-max-age
    if(resp.cacheControlFields.find(CacheControlKey::ck_s_max_age) != resp.cacheControlFields.cend()){
        expired = rece_t + stoi(resp.cacheControlFields[CacheControlKey::ck_s_max_age]);
    }
    //max-age  
    else if(resp.cacheControlFields.find(CacheControlKey::ck_max_age) != resp.cacheControlFields.cend()){
        expired = rece_t + stoi(resp.cacheControlFields[CacheControlKey::ck_max_age]);
    }
    // expires
    else if (resp.cacheControlFields.find(CacheControlKey::ck_expires) != resp.cacheControlFields.cend()){
        struct tm expired_tm;
        std::string expired_str = resp.cacheControlFields[CacheControlKey::ck_expires];
        if(strptime(expired_str.c_str(), "%a, %d %b %Y %H:%M:%S GMT", &expired_tm)){
            expired = mktime(&expired_tm);
        }
    }

    return expired;
}