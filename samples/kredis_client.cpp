#include "kredis.hpp"
#include <chrono>
int main(int argc, char* argv[]) {

	KRedis<> aredis;
	aredis.start("127.0.0.1");
	int count = 10;
	// while(count -- > 0 )
	{

		std::this_thread::sleep_for(std::chrono::microseconds(1));
		RedisQuery query("set", "date", "2020-7-31");
		dlog("execute command :{}", query.command());
		aredis.query(query, [](const RedisResult& rst) {
			ilog("set date result is {}", rst.to_string());
		});

		RedisQuery fetchQuery("get", "date");
		aredis.query(fetchQuery, [](const RedisResult& rst) {
			ilog("get date result is {}", rst.to_string());
		});
	};

	std::this_thread::sleep_for(std::chrono::seconds(3));
	return 0;
}
