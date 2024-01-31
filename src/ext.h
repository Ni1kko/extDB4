#pragma once

#include <chrono>
#include <thread>
#include <unordered_map>

#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/random/random_device.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/thread/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "abstract_ext.h"

#include "protocols/abstract_protocol.h"
#include "logger/logging.hpp"

#include <intercept.hpp>

namespace ExtDB4
{
	using namespace intercept::client;

	static std::vector<std::string> StoredRandomStrings{};

	namespace SQFCommands
	{
		using namespace intercept::types;

		static registered_sqf_function _Version;
		static registered_sqf_function _Locked;
		static registered_sqf_function _Lock;
		static registered_sqf_function _Reset;
		static registered_sqf_function _SetProfile;
		static registered_sqf_function _SetProfileProtocol;
		static registered_sqf_function _AddProtocol;
		static registered_sqf_function _UpTime;
		static registered_sqf_function _LocalTime;
		static registered_sqf_function _DateAdd;
		static registered_sqf_function _FireAndForget;
		static registered_sqf_function _AsyncQuery;
		static registered_sqf_function _Query;
		static registered_sqf_function _SinglePartMessage;
		static registered_sqf_function _MultiPartMessage;
		static registered_sqf_function _RandomString;

		//new temp method
		static registered_sqf_function _QueryNew;
		static registered_sqf_function _Shutdown;
		static registered_sqf_function _NumberToOrdinal;

		game_value Version();
		game_value Locked();
		game_value Lock(game_value_parameter left_arg, game_value_parameter right_arg);
		game_value Reset();
		game_value SetProfile(game_value_parameter right_arg);
		game_value SetProfileProtocol(game_value_parameter left_arg, game_value_parameter right_arg);
		game_value AddProtocol(game_value_parameter left_arg, game_value_parameter right_arg);
		game_value UpTime(game_value_parameter right_arg);
		game_value LocalTime();
		game_value DateAdd(game_value_parameter left_arg, game_value_parameter right_arg);
		game_value FireAndForget(game_value_parameter left_arg, game_value_parameter right_arg);
		game_value AsyncQuery(game_value_parameter left_arg, game_value_parameter right_arg);
		game_value Query(game_value_parameter left_arg, game_value_parameter right_arg);
		game_value SinglePartMessage(game_value_parameter right_arg);
		game_value MultiPartMessage(game_value_parameter right_arg);
		game_value RandomString(game_value_parameter right_arg);

		//new temp method
		game_value QueryNew(game_value_parameter left_arg, game_value_parameter right_arg);
		game_value Shutdown();
		game_value NumberToOrdinal(game_value_parameter right_arg);

		static void Register();
	}
}

class Ext: public AbstractExt
{
public:
	Ext(std::string shared_libary_path);
	~Ext();
	void reset();
	void stop();
	void idleCleanup(const boost::system::error_code& ec);

	struct protocol_struct
	{
		std::string													name;
		std::unique_ptr<AbstractProtocol>		protocol;
	};

//private:
	// Config File
	boost::property_tree::ptree ptree;

	// Input
	std::string::size_type input_str_length;

	// Main ASIO Thread Queue
	std::unique_ptr<boost::asio::io_service::work> io_work_ptr;
	boost::asio::io_service io_service;
	boost::thread_group threads;

	std::mutex mutex_mariadb_idle_cleanup_timer;
	std::unique_ptr<boost::asio::deadline_timer> mariadb_idle_cleanup_timer;

	// Protocols
	std::vector<protocol_struct> vec_protocols;
	std::mutex mutex_vec_protocols;

	// Unique ID
	unsigned long unique_id_counter = 100; // Can't be value 1

	// Results
	std::unordered_map<unsigned long, resultData> stored_results;
	std::mutex mutex_results;  // Using Same Lock for Unique ID aswell

	// UPTimer
	std::chrono::time_point<std::chrono::steady_clock> uptime_start;
	std::chrono::time_point<std::chrono::steady_clock> uptime_current;
	
	// Clock
	boost::posix_time::ptime ptime;
	boost::posix_time::time_facet *facet;
	
	boost::posix_time::time_facet *facet_localtime;

	void search(boost::filesystem::path &extDB_config_path, bool &conf_found, bool &conf_randomized);

	bool connectDatabase(const std::string &database_conf, const std::string &database_id);

	// Protocols
	bool addProtocol(const std::string &database_id, const std::string &protocol, const std::string &protocol_name, const std::string &init_data);
	std::string getSinglePartResult_mutexlock(const unsigned long &unique_id);
	std::string getMultiPartResult_mutexlock(const unsigned long &unique_id);
	std::string syncCallProtocol(std::string& protocol_name, std::string& query_str);
	void onewayCallProtocol(std::string &input_str);
	void asyncCallProtocol(const int &output_size, const std::string &protocol_name, const std::string &data, const unsigned long unique_id);

	const unsigned long saveResult_mutexlock(const resultData &result_data);
	void saveResult_mutexlock(const unsigned long &unique_id, const resultData &result_data);
	void saveResult_mutexlock(std::vector<unsigned long> &unique_ids, const resultData &result_data);

	void getUPTime(std::string &token, std::string &result);
	void getLocalTime(std::string &result);
	void getLocalTime(std::string &input_str, std::string &result);
	void getDateAdd(std::string &token, std::string &token2, std::string &result);
	std::string NumberToOrdinal(size_t number);
};
