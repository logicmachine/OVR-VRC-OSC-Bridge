#pragma once

#include <string>
#include <vector>
#include <memory>
#include <iterator>
#include <chrono>
#include <cstdint>

#pragma warning(push, 0)
#include <codeanalysis/warnings.h>
#pragma warning(disable: ALL_CODE_ANALYSIS_WARNINGS)
#include <boost/asio.hpp>
#pragma warning(pop, 0)


class OSCMessage {

private:
	std::vector<uint8_t> payload_;


	template <typename... Args>
	struct TagWriter { };

	template <typename... Args>
	struct TagWriter<int, Args...> {
		static void write(std::vector<uint8_t>& buffer) {
			buffer.push_back('i');
			TagWriter<Args...>::write(buffer);
		}
	};

	template <typename... Args>
	struct TagWriter<float, Args...> {
		static void write(std::vector<uint8_t>& buffer) {
			buffer.push_back('f');
			TagWriter<Args...>::write(buffer);
		}
	};

	template <>
	struct TagWriter<> {
		static void write(std::vector<uint8_t>&) { }
	};


	template <typename... Args>
	struct DataWriter { };

	template <typename... Args>
	struct DataWriter<int, Args...> {
		static void write(std::vector<uint8_t>& buffer, int value, const Args&... rest) {
			const uint32_t u = static_cast<uint32_t>(value);
			buffer.push_back(static_cast<uint8_t>((u >> 24) & 0xff));
			buffer.push_back(static_cast<uint8_t>((u >> 16) & 0xff));
			buffer.push_back(static_cast<uint8_t>((u >>  8) & 0xff));
			buffer.push_back(static_cast<uint8_t>((u >>  0) & 0xff));
			DataWriter<Args...>::write(buffer, rest...);
		}
	};

	template <typename... Args>
	struct DataWriter<float, Args...> {
		static void write(std::vector<uint8_t>& buffer, float value, const Args&... rest) {
			union { float f; uint32_t u; } fu;
			fu.f = value;
			buffer.push_back(static_cast<uint8_t>((fu.u >> 24) & 0xff));
			buffer.push_back(static_cast<uint8_t>((fu.u >> 16) & 0xff));
			buffer.push_back(static_cast<uint8_t>((fu.u >>  8) & 0xff));
			buffer.push_back(static_cast<uint8_t>((fu.u >>  0) & 0xff));
			DataWriter<Args...>::write(buffer, rest...);
		}
	};

	template <>
	struct DataWriter<> {
		static void write(std::vector<uint8_t>&) { }
	};


	void insert_null_padding() {
		while ((payload_.size() & 3) != 0) { payload_.push_back('\0'); }
	}


public:
	OSCMessage()
		: payload_()
	{ }

	template <typename... Args>
	explicit OSCMessage(const std::string& addr, const Args&... args)
		: payload_()
	{
		std::copy(addr.begin(), addr.end(), std::back_inserter(payload_));
		payload_.push_back('\0');
		insert_null_padding();
		payload_.push_back(',');
		TagWriter<Args...>::write(payload_);
		payload_.push_back('\0');
		insert_null_padding();
		DataWriter<Args...>::write(payload_, args...);
	}

	const std::vector<uint8_t>& payload() const { return payload_; }

};


class OSCBundle {

private:
	std::vector<uint8_t> payload_;

	static uint64_t get_time_tag() {
		return 1u;  // immediate
	}

	void add_element(const std::vector<uint8_t>& data) {
		const auto s = static_cast<uint32_t>(data.size());
		payload_.push_back(static_cast<uint8_t>((s >> 24) & 0xff));
		payload_.push_back(static_cast<uint8_t>((s >> 16) & 0xff));
		payload_.push_back(static_cast<uint8_t>((s >>  8) & 0xff));
		payload_.push_back(static_cast<uint8_t>((s >>  0) & 0xff));
		std::copy(data.begin(), data.end(), std::back_inserter(payload_));
	}

public:
	OSCBundle()
		: payload_()
	{
		static const char* MAGIC = "#bundle";
		std::copy(MAGIC, MAGIC + 8, std::back_inserter(payload_));
		const auto t = get_time_tag();
		payload_.push_back(static_cast<uint8_t>((t >> 56) & 0xff));
		payload_.push_back(static_cast<uint8_t>((t >> 48) & 0xff));
		payload_.push_back(static_cast<uint8_t>((t >> 40) & 0xff));
		payload_.push_back(static_cast<uint8_t>((t >> 32) & 0xff));
		payload_.push_back(static_cast<uint8_t>((t >> 24) & 0xff));
		payload_.push_back(static_cast<uint8_t>((t >> 16) & 0xff));
		payload_.push_back(static_cast<uint8_t>((t >>  8) & 0xff));
		payload_.push_back(static_cast<uint8_t>((t >>  0) & 0xff));
	}

	OSCBundle& add(const OSCBundle& bundle) {
		add_element(bundle.payload());
		return *this;
	}

	OSCBundle& add(const OSCMessage& message) {
		add_element(message.payload());
		return *this;
	}

	const std::vector<uint8_t>& payload() const { return payload_; }

};


class OSCSender {

private:
	std::unique_ptr<boost::asio::ip::udp::socket> socket_;
	std::unique_ptr<boost::asio::ip::udp::endpoint> endpoint_;

public:
	OSCSender()
		: socket_()
		, endpoint_()
	{ }

	OSCSender(boost::asio::io_service& io_service, const std::string& host, int port)
		: socket_()
		, endpoint_()
	{
		using boost::asio::ip::udp;
		socket_ = std::make_unique<udp::socket>(io_service, udp::v4());
		endpoint_ = std::make_unique<udp::endpoint>(boost::asio::ip::address::from_string(host), port);
	}

	void send(const OSCMessage& message) {
		for (const auto c : message.payload()) {
			std::cout << std::hex << static_cast<int>(c) << " ";
		}
		std::cout << std::endl;
		auto buffer = boost::asio::buffer(message.payload());
		socket_->send_to(buffer, *endpoint_);
	}

	void send(const OSCBundle& bundle) {
		auto buffer = boost::asio::buffer(bundle.payload());
		socket_->send_to(buffer, *endpoint_);
	}

};
