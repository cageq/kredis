#pragma once 

/**
 * @file kredis.cpp
 * @brief 
 * @author arthur
 * @version 1.0.0
 * @date 2020-08-01
 */

#include "klog.hpp"

template <class T> struct ParamSize {
    static size_t type_size(const T & t){ 
        //return sizeof(typename std::decay<T>::type) ;
        return sizeof(T) ;
    }    
}; 
template <> struct ParamSize<std::string> {
    static size_t type_size(const std::string & t){ 
        return  t.size();
    }
}; 

template <> struct ParamSize<std::string_view> {
    static size_t type_size(const std::string_view & t){ 
        return  t.size();
    }
}; 
template <> struct ParamSize<const char * > {
    static size_t type_size(const char * t){ 
        return  strlen(t);
    }
}; 
 
  

class RedisQuery{

public:  
    template <class... Args>
	RedisQuery( Args... rest) { 
        fmt::format_to(query_buffer,"*{}\r\n",sizeof...(rest) );
        this->push(rest ... );  
    } 
 
    std::string  command() const {
        return  fmt::to_string(query_buffer); 
    }

private:
    template <class T , class... Args>
	void push(const T& first, Args... rest) { 
        fmt::format_to(query_buffer,"${}\r\n{}\r\n", ParamSize<T>::type_size(first),first ); 

        push (rest ...); 
    }
    void push( ) {  }

    fmt::memory_buffer query_buffer; 
}; 
