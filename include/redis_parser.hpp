#pragma once 

/**
 * @file kredis.cpp
 * @brief 
 * @author arthur
 * @version 1.0.0
 * @date 2020-08-01
 */

#include <stack>
#include <vector>
#include <utility>

#include "redis_result.hpp"

class RedisParser {
	public:

		RedisParser() : bulk_size(0) { parser_buffer.reserve(64); }

		enum ParseResult {
			Completed,
			Incompleted,
			Error,
		};

		std::pair<size_t, ParseResult> parse(const char* pData, size_t dataLen){

			return parse_chunk(pData, dataLen);
		}

		RedisResult result()const 
		{
			return std::move(redis_value);
		}

	protected:
		std::pair<size_t, ParseResult> parse_chunk(const char* pData, size_t dataLen){
			size_t position = 0;
			State state = Start;
			dlog("parse chunk length {}", dataLen);
			if (!states.empty())
			{
				state = states.top();
				states.pop();
			}

			while(position < dataLen)
			{
				char c = pData[position++];
				dlog("state : {} , c: {}, position: {}", state, c, position); 

				switch(state)
				{
					case StartArray:
					case Start:
						parser_buffer.clear();
						switch(c)
						{
							case kStringPrefix:
								state = String;
								break;
							case kErrorPrefix:
								state = ErrorString;
								break;
							case kIntegerPrefix:
								state = Integer;
								break;
							case kBulkPrefix:
								state = BulkSize;
								bulk_size = 0;
								break;
							case kArrayPrefix:
								state = ArraySize;
								break;
							default:
								return std::make_pair(position, Error);
						}
						break;
					case String:
						if( c == '\r' )
						{
							state = StringLF;
						}
						else if( isChar(c) && !isControl(c) )
						{
							parser_buffer.push_back(c);
						}
						else
						{
							std::stack<State>().swap(states);
							return std::make_pair(position, Error);
						}
						break;
					case ErrorString:
						if( c == '\r' )
						{
							state = ErrorLF;
						}
						else if( isChar(c) && !isControl(c) )
						{
							parser_buffer.push_back(c);
						}
						else
						{
							std::stack<State>().swap(states);
							return std::make_pair(position, Error);
						}
						break;
					case BulkSize:
						if( c == '\r' )
						{
							if( parser_buffer.empty() )
							{
								std::stack<State>().swap(states);
								return std::make_pair(position, Error);
							}
							else
							{
								state = BulkSizeLF;
							}
						}
						else if( isdigit(c) || c == '-' )
						{
							parser_buffer.push_back(c);
						}
						else
						{
							std::stack<State>().swap(states);
							return std::make_pair(position, Error);
						}
						break;
					case StringLF:
						if( c == '\n')
						{
							state = Start;
							redis_value = RedisResult(parser_buffer);
						}
						else
						{
							elog("return from here"); 
							std::stack<State>().swap(states);
							return std::make_pair(position, Error);
						}
						break;
					case ErrorLF:
						if( c == '\n')
						{
							state = Start;
							RedisResult::ErrorTag tag;
							redis_value = RedisResult(parser_buffer, tag);
						}
						else
						{
							elog("return from here 2232"); 
							std::stack<State>().swap(states);
							return std::make_pair(position, Error);
						}
						break;
					case BulkSizeLF:
						if( c == '\n' )
						{
							bulk_size = bufToLong(parser_buffer.data(), parser_buffer.size());
							parser_buffer.clear();

							if( bulk_size == -1 )
							{
								state = Start;
								redis_value = RedisResult(); // Nil
							}
							else if( bulk_size == 0 )
							{
								state = BulkCR;
							}
							else if( bulk_size < 0 )
							{
								std::stack<State>().swap(states);
								elog("return from here 22323333"); 
								return std::make_pair(position, Error);
							}
							else
							{
								parser_buffer.reserve(bulk_size);

								long int available = dataLen - position;
								long int canRead = std::min(bulk_size, available);

								if( canRead > 0 )
								{
									parser_buffer.assign(pData + position, pData + position + canRead);
									position += canRead;
									bulk_size -= canRead;
								}


								if (bulk_size > 0)
								{
									state = Bulk;
								}
								else
								{
									state = BulkCR;
								}
							}
						}
						else
						{
							std::stack<State>().swap(states);

							elog("return from here 223254555"); 
							return std::make_pair(position, Error);
						}
						break;
					case Bulk: {
								   assert( bulk_size > 0 );

								   long int available = dataLen - position + 1;
								   long int canRead = std::min(available, bulk_size);

								   parser_buffer.insert(parser_buffer.end(), pData + position - 1, pData + position - 1 + canRead);
								   bulk_size -= canRead;
								   position += canRead - 1;

								   if( bulk_size == 0 )
								   {
									   state = BulkCR;
								   }
								   break;
							   }
					case BulkCR:
							   if( c == '\r')
							   {
								   state = BulkLF;
							   }
							   else
							   {
								   std::stack<State>().swap(states);
								   return std::make_pair(position, Error);
							   }
							   break;
					case BulkLF:
							   if( c == '\n')
							   {
								   state = Start;
								   redis_value = RedisResult(parser_buffer);
								   ilog("parse result is {}", parser_buffer.data()); 
							   }
							   else
							   {
								   std::stack<State>().swap(states);
								   return std::make_pair(position, Error);
							   }
							   break;
					case ArraySize:
							   if( c == '\r' )
							   {
								   if( parser_buffer.empty() )
								   {
									   std::stack<State>().swap(states);
									   return std::make_pair(position, Error);
								   }
								   else
								   {
									   state = ArraySizeLF;
								   }
							   }
							   else if( isdigit(c) || c == '-' )
							   {
								   parser_buffer.push_back(c);
							   }
							   else
							   {
								   std::stack<State>().swap(states);
								   return std::make_pair(position, Error);
							   }
							   break;
					case ArraySizeLF:
							   if( c == '\n' )
							   {
								   int64_t arraySize = bufToLong(parser_buffer.data(), parser_buffer.size());
								   std::vector<RedisResult> array;

								   if( arraySize == -1 )
								   {
									   state = Start;
									   redis_value = RedisResult();  // Nil value
								   }
								   else if( arraySize == 0 )
								   {
									   state = Start;
									   redis_value = RedisResult(std::move(array));  // Empty array
								   }
								   else if( arraySize < 0 )
								   {
									   std::stack<State>().swap(states);
									   return std::make_pair(position, Error);
								   }
								   else
								   {
									   array.reserve(arraySize);
									   parser_size_stack.push(arraySize);
									   parser_value_stack.push(std::move(array));

									   state = StartArray;
								   }
							   }
							   else
							   {
								   std::stack<State>().swap(states);
								   return std::make_pair(position, Error);
							   }
							   break;
					case Integer:
							   if( c == '\r' )
							   {
								   if( parser_buffer.empty() )
								   {
									   std::stack<State>().swap(states);
									   return std::make_pair(position, Error);
								   }
								   else
								   {
									   state = IntegerLF;
								   }
							   }
							   else if( isdigit(c) || c == '-' )
							   {
								   parser_buffer.push_back(c);
							   }
							   else
							   {
								   std::stack<State>().swap(states);
								   return std::make_pair(position, Error);
							   }
							   break;
					case IntegerLF:
							   if( c == '\n' )
							   {
								   int64_t value = bufToLong(parser_buffer.data(), parser_buffer.size());

								   parser_buffer.clear();
								   redis_value = RedisResult(value);
								   state = Start;
							   }
							   else
							   {
								   std::stack<State>().swap(states);
								   return std::make_pair(position, Error);
							   }
							   break;
					default:
							   std::stack<State>().swap(states);
							   return std::make_pair(position, Error);
				}


				if (state == Start)
				{
					if (!parser_size_stack.empty())
					{
						assert(parser_size_stack.size() > 0);
						parser_value_stack.top().get_array().push_back(redis_value);

						while(!parser_size_stack.empty() && --parser_size_stack.top() == 0)
						{
							parser_size_stack.pop();
							redis_value = std::move(parser_value_stack.top());
							parser_value_stack.pop();

							if (!parser_size_stack.empty())
								parser_value_stack.top().get_array().push_back(redis_value);
						}
					}


					if (parser_size_stack.empty())
					{
						// done
						break;
					}
				}
			}

			if (parser_size_stack.empty() && state == Start)
			{
				return std::make_pair(position, Completed);
			}
			else
			{
				states.push(state);
				return std::make_pair(position, Incompleted);
			}

		}

		inline bool isChar(int c) { return c >= 0 && c <= 127; }

		inline bool isControl(int c) { return (c >= 0 && c <= 31) || (c == 127); }

		long int bufToLong(const char *str, size_t size)
		{
			long int value = 0;
			bool sign = false;

			if( str == nullptr || size == 0 )
			{
				return 0;
			}

			if( *str == '-' )
			{
				sign = true;
				++str;
				--size;

				if( size == 0 ) {
					return 0;
				}
			}

			for(const char *end = str + size; str != end; ++str)
			{
				char c = *str;
				// char must be valid, already checked in the parser
				assert(c >= '0' && c <= '9');

				value = value * 10;
				value += c - '0';
			}

			return sign ? -value : value;
		}
	private:
		enum State {
			Start = 0,
			StartArray = 1,

			String = 2,
			StringLF = 3,

			ErrorString = 4,
			ErrorLF = 5,

			Integer = 6,
			IntegerLF = 7,

			BulkSize = 8,
			BulkSizeLF = 9,
			Bulk = 10,
			BulkCR = 11,
			BulkLF = 12,

			ArraySize = 13,
			ArraySizeLF = 14,
		};

		std::stack<State> states;

		long int bulk_size = 0 ;
		std::vector<char> parser_buffer;
		RedisResult redis_value;

		// temporary variables
		std::stack<long int> parser_size_stack;
		std::stack<RedisResult> parser_value_stack;

		static const char kStringPrefix = '+';
		static const char kErrorPrefix = '-';
		static const char kIntegerPrefix = ':';
		static const char kBulkPrefix = '$';
		static const char kArrayPrefix = '*';
};
