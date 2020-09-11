#pragma once

/**
 * @file kredis.cpp
 * @brief 
 * @author arthur
 * @version 1.0.0
 * @date 2020-08-01
 */

#include <string>
#include <vector>
#include <tuple>
#include <cassert> 
#include <variant> 

class RedisResult {
	public:
		struct ErrorTag {};

		RedisResult()
			: value(NullTag())
			  , error(false) {}

		RedisResult(RedisResult&& other)
			: value(std::move(other.value))
			  , error(other.error) {}

		RedisResult(int64_t i)
			: value(i)
			  , error(false) {}

		RedisResult(const char* s)
			: value(std::vector<char>(s, s + strlen(s)))
			  , error(false) {}

		RedisResult(const std::string& s)
			: value(std::vector<char>(s.begin(), s.end()))
			  , error(false) {}

		RedisResult(std::vector<char> buf)
			: value(std::move(buf))
			  , error(false) {}

		RedisResult(std::vector<char> buf, struct ErrorTag)
			: value(std::move(buf))
			  , error(true) {}

		RedisResult(std::vector<RedisResult> array)
			: value(std::move(array))
			  , error(false) {}


		RedisResult(const RedisResult&) = default;
		RedisResult& operator=(const RedisResult&) = default;
		RedisResult& operator=(RedisResult&&) = default;


		std::vector<RedisResult> to_array() const { return cast_to<std::vector<RedisResult>>(); }

		std::string to_string() const {
			const std::vector<char>& buf = to_byte_array();
			return std::string(buf.begin(), buf.end());
		}

		std::vector<char> to_byte_array() const { return cast_to<std::vector<char>>(); }

		int64_t to_int() const { return cast_to<int64_t>(); }

		std::string inspect() const {
			if (is_error()) {
				static std::string err = "error: ";
				std::string result;

				result = err;
				result += to_string();

				return result;
			} else if (is_null()) {
				static std::string null = "(null)";
				return null;
			} else if (is_int()) {
				return std::to_string(to_int());
			} else if (is_string()) {
				return to_string();
			} else {
				std::vector<RedisResult> values = to_array();
				std::string result = "[";

				if (values.empty() == false) {
					for (size_t i = 0; i < values.size(); ++i) {
						result += values[i].inspect();
						result += ", ";
					}

					result.resize(result.size() - 1);
					result[result.size() - 1] = ']';
				} else {
					result += ']';
				}

				return result;
			}
		}

		bool is_ok() const { return !is_error(); }

		bool is_error() const { return error; }

		bool is_null() const { return type_equal<NullTag>(); }

		bool is_int() const { return type_equal<int64_t>(); }

		bool is_string() const { return type_equal<std::vector<char>>(); }

		bool is_byte_array() const { return type_equal<std::vector<char>>(); }

		bool is_array() const { return type_equal<std::vector<RedisResult>>(); }

		std::vector<char>& get_byte_array() {
			//	assert(is_byte_array());
			return std::get<std::vector<char>>(value);
		}

		const std::vector<char>& get_byte_array() const {
			assert(is_byte_array());
			return std::get<std::vector<char>>(value);
		}

		std::vector<RedisResult>& get_array() {
			assert(is_array());
			return std::get<std::vector<RedisResult>>(value);
		}

		const std::vector<RedisResult>& get_array() const {
			assert(is_array());
			return std::get<std::vector<RedisResult>>(value);
		}


		bool operator==(const RedisResult& rhs) const { return value == rhs.value; }

		bool operator!=(const RedisResult& rhs) const { return !(value == rhs.value); }

	protected:
		template <typename T>
			T cast_to() const{
				return std::get<T>(value);
			}

		template <typename T>
			bool type_equal() const{
				if (auto val = std::get_if<T>(&value) ) {
					return true; 

				} else { 

					return false; 
				}
				//return typeid(value) == typeid(T); 
			}

	private:
		struct NullTag {
			inline bool operator==(const NullTag&) const { return true; }
		};

		std::variant<NullTag, int64_t, std::vector<char>, std::vector<RedisResult>> value;
		bool error;
};
