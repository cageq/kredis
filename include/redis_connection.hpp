#pragma once 

/**
 * @file kredis.cpp
 * @brief 
 * @author arthur
 * @version 1.0.0
 * @date 2020-08-01
 */

#include <list>
#include "knet.hpp"
#include "redis_parser.hpp"

using RedisResultHandler = std::function<void(const RedisResult & )>; 


using namespace knet::tcp; 
class RedisConnection : public Connection<RedisConnection>{
  
public: 
 
    RedisConnection(){
        bind_data_handler(&RedisConnection::process_result); 
    } 

    ~RedisConnection(){
        result_handlers.clear();
    }
	void query(const RedisQuery & query, RedisResultHandler handler ){
        
        result_handlers.emplace_back(  handler);
		send(query.command());  
    }

    uint32_t  process_result(const std::string_view &msg   , MessageStatus status)
    {
        wlog("received data {} ",msg ); 
		auto rst = redis_parser.parse(msg.data(),msg.length() ); 
		if (rst.second == RedisParser::Completed) { 
			dlog("parse result success  len {} , status  {}", rst.first, rst.second);             
			dlog("redis result :{}", redis_parser.result().inspect()); 

            if (!result_handlers.empty()){
                auto &handler = result_handlers.front(); 
                if (handler){
                    handler(redis_parser.result()); 
                } 
                result_handlers.pop_front();
            }
          
            return rst.first;  
		}

        return msg.length(); 
    }

	RedisParser  redis_parser;  
    std::list<RedisResultHandler> result_handlers; 
}; 


using RedisConnectionPtr = std::shared_ptr<RedisConnection>; 
