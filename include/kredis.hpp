#pragma once

/**
 * @file kredis.cpp
 * @brief 
 * @author arthur
 * @version 1.0.0
 * @date 2020-08-01
 */
#include "knet.hpp"
#include "redis_query.hpp"
#include "redis_connection.hpp"

template <class Worker = knet::EventWorker>

class KRedis {
public:
	KRedis(std::shared_ptr<Worker> worker = nullptr)
		: redis_connector(nullptr, worker) {}

	bool start(const std::string& host, uint32_t port = 6379) {
		redis_connector.start();
		auto conn = redis_connector.add_connection(host, port);
		redis_connections.push_back(conn);
		return true;
	}

	void query(const RedisQuery& query, RedisResultHandler handler = nullptr) {
		uint32_t index =
			std::hash<std::thread::id>{}(std::this_thread::get_id()) % redis_connections.size();
		redis_connections[index]->query(query, handler);
	}
	template <class T, class... Args>
	void query(RedisResultHandler handler, Args&&... args) {
		RedisQuery query(args...);
		uint32_t index =
			std::hash<std::thread::id>{}(std::this_thread::get_id()) % redis_connections.size();
		redis_connections[index]->query(query, handler);
	}
	template <class T, class... Args>
	void query(Args&&... args) {
		RedisQuery query(args...);
		uint32_t index =
			std::hash<std::thread::id>{}(std::this_thread::get_id()) % redis_connections.size();
		redis_connections[index]->query(query, nullptr);
	}

private:
	uint32_t redis_index = 0;
	std::vector<RedisConnectionPtr> redis_connections;
	TcpConnector<RedisConnection> redis_connector;
};
