#include "ext.h"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <mutex>
#include <regex>
#include <stdlib.h>
#include <thread>

#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem.hpp>
#include <boost/property_tree/ini_parser.hpp>

#include <boost/random/random_device.hpp>
#include <boost/random/uniform_int_distribution.hpp>

#include "abstract_ext.h"
#include "mariaDB/exceptions.h"
#include "md5/md5.h"

#include "protocols/abstract_protocol.h"
#include "protocols/sql.h"
#include "protocols/sql_custom.h"
#include "protocols/log.h"


#pragma warning(disable : 4996)

INITIALIZE_EASYLOGGINGPP
 
Ext::Ext(std::string shared_library_path)
{
	uptime_start = std::chrono::steady_clock::now();
	std::setlocale(LC_ALL, "");
	std::locale::global(std::locale(""));
	std::setlocale(LC_NUMERIC, "C");
	mysql_library_init(0, NULL, NULL);

	try
	{
		bool conf_found = false;
		bool conf_randomized = false;

		boost::filesystem::path config_path;

		// extDB4 Shared Library Location
		config_path = shared_library_path;
		config_path = config_path.parent_path();
		config_path /= "extDB4-conf.ini";
		if (boost::filesystem::is_regular_file(config_path))
		{
			conf_found = true;
			ext_info.path = config_path.parent_path().string();
		}	else {
			// Search for Randomize Config File -- Legacy Security Support For Arma2Servers

			config_path = config_path.parent_path();
			// CHECK DLL PATH FOR CONFIG)
			if (!config_path.string().empty())
			{
				search(config_path, conf_found, conf_randomized);
			}
		}

		// Load config
		if (conf_found)
		{
			boost::property_tree::ini_parser::read_ini(config_path.string(), ptree);

			// Search for Randomize Config File -- Legacy Security Support For Arma2Servers

			if ((ptree.get("Main.Randomize Config File",false)) && (!conf_randomized))
			// Only Gonna Randomize Once, Keeps things Simple
			{
				std::string chars("ABCDEFGHIJKLMNOPQRSTUVWXYZ"
									"1234567890");
				// Skipping Lowercase, this function only for arma2 + extensions only available on windows.
				boost::random::random_device rng;
				boost::random::uniform_int_distribution<> index_dist(0, chars.size() - 1);

				std::string randomized_filename = "extDB4-conf-";
				for (int i = 0; i < 8; ++i)
				{
					randomized_filename += chars[index_dist(rng)];
				}
				randomized_filename += ".ini";

				boost::filesystem::path randomize_configfile_path = config_path.parent_path() /= randomized_filename;
				boost::filesystem::rename(config_path, randomize_configfile_path);
			}
		}

		// Logger path
		std::time_t t = std::time(nullptr);
		std::tm tm = *std::localtime(&t); //Not Threadsafe
		std::string dayofweek[] = { "Sun","Mon","Tue","Wed","Thu","Fri","Sat" };
		boost::filesystem::path log_relative_path;
		log_relative_path = boost::filesystem::path(config_path.parent_path());
		log_relative_path /= "logs";
		log_relative_path /= std::to_string((tm.tm_year + 1900));
		
		switch (tm.tm_mon)
		{
			case 0:
				log_relative_path /= "January";
				break;
			case 1:
				log_relative_path /= "Febuary";
				break;
			case 2:
				log_relative_path /= "March";
				break;
			case 3:
				log_relative_path /= "April";
				break;
			case 4:
				log_relative_path /= "May";
				break;
			case 5:
				log_relative_path /= "June";
				break;
			case 6:
				log_relative_path /= "July";
				break;
			case 7:
				log_relative_path /= "August";
				break;
			case 8:
				log_relative_path /= "September";
				break;
			case 9:
				log_relative_path /= "October";
				break;
			case 10:
				log_relative_path /= "November";
				break;
			default:
				log_relative_path /= "December";
				break;
		}

		log_relative_path /= dayofweek[tm.tm_wday] + " (" + NumberToOrdinal(tm.tm_mday) + ")";

		ext_info.log_path = log_relative_path.make_preferred().string();
		boost::filesystem::create_directories(log_relative_path);

		// Logger File name
		std::string logfilename = "ExtDB4_";
			
		if (tm.tm_hour >= 0 && tm.tm_hour <= 9) logfilename += "0";
		logfilename += std::to_string(tm.tm_hour) + "-" + std::to_string(tm.tm_min) + "-" + std::to_string(tm.tm_sec);
		logfilename += (tm.tm_hour >= 12 && tm.tm_hour <= 23 ? " PM" : " AM");

		log_relative_path /= logfilename + ".log";

		// Setup easy logger
		el::Configurations logger;
		logger.setGlobally(el::ConfigurationType::Filename, log_relative_path.make_preferred().string());
		logger.setGlobally(el::ConfigurationType::MaxLogFileSize, "10240");
		el::Loggers::reconfigureAllLoggers(el::ConfigurationType::Format, "%datetime-{%level}- %msg");
		el::Loggers::setDefaultConfigurations(logger, true);
		
		LOG(INFO) << "extDB4: Version: " << EXTDB_VERSION;

		#ifdef _MSC_VER
			LOG(INFO) << "extDB4: Windows Compiled on " << __DATE__ << " @ " << std::string{ __TIME__ };
		#endif
		
		if (!conf_found)
		{
			LOG(ERROR) << "extDB4: Unable to find extDB4-conf.ini";
			// Kill Server no config file found -- Evil
			std::exit(EXIT_SUCCESS);
		}
		else
		{
			LOG(INFO) << "extDB4: Found extDB4-conf.ini";

			if ((ptree.get("Main.Version",0)) != EXTDB_CONF_VERSION)
			{
				LOG(ERROR) << "extDB4: Incompatiable Config Version: " << ptree.get("Main.Version", 0) << ",  Required Version: " << EXTDB_CONF_VERSION;
				// Kill Server if wrong config version -- Evil
				std::exit(EXIT_SUCCESS);
			}

			//
			ext_info.allow_reset = ptree.get("Main.Allow Reset",false);

			// Start Threads + ASIO
			ext_info.max_threads = ptree.get("Main.Threads",0);
			int detected_cpu_cores = boost::thread::hardware_concurrency();
			if (ext_info.max_threads <= 0)
			{
				// Auto-Detect
				if (detected_cpu_cores > 6)
				{
					LOG(INFO) << "extDB4: Detected " << detected_cpu_cores << " Cores, Setting up 6 Worker Threads";
					ext_info.max_threads = 6;
				}
				else if (detected_cpu_cores <= 2)
				{
					LOG(INFO) << "extDB4: Detected " << detected_cpu_cores << " Cores, Setting up 2 Worker Threads";
					ext_info.max_threads = 2;
				}	else {
					ext_info.max_threads = detected_cpu_cores;
					LOG(INFO) << "extDB4: Detected " << detected_cpu_cores << " Cores, Setting up " << ext_info.max_threads << " Worker Threads";
				}
			}

			// Setup ASIO Worker Pool
			io_work_ptr.reset(new boost::asio::io_service::work(io_service));
			for (int i = 0; i < ext_info.max_threads; ++i)
			{
				threads.create_thread(boost::bind(&boost::asio::io_service::run, &io_service));
			}
		}
	}
	catch (boost::property_tree::ini_parser::ini_parser_error const &e)
	{
		LOG(ERROR) << "BOOST INI PARSER: " << e.what();
		std::exit(EXIT_FAILURE);
	}
	catch(boost::filesystem::filesystem_error const &e)
	{
		LOG(ERROR) << "BOOST FILESYSTEM ERROR: " << e.what();
		std::exit(EXIT_FAILURE);
	}
}

Ext::~Ext(void)
{
	stop();
	mysql_library_end();
}


void Ext::reset()
{
	stop();
	std::lock_guard<std::mutex> lock(mutex_vec_protocols);
	{
		vec_protocols.clear();
	}
	mariadb_databases.clear();

	// Setup ASIO Worker Pool
	io_service.reset();
	io_work_ptr.reset(new boost::asio::io_service::work(io_service));
	for (int i = 0; i < ext_info.max_threads; ++i)
	{
		threads.create_thread(boost::bind(&boost::asio::io_service::run, &io_service));
	}
	mariadb_idle_cleanup_timer.reset(new boost::asio::deadline_timer(io_service));
	mariadb_idle_cleanup_timer->expires_at(mariadb_idle_cleanup_timer->expires_at() + boost::posix_time::seconds(600));
	mariadb_idle_cleanup_timer->async_wait(boost::bind(&Ext::idleCleanup, this, _1));
	LOG(DEBUG) << "extDB4: Reset... ";
}

void Ext::stop()
{
	std::lock_guard<std::mutex> lock(mutex_mariadb_idle_cleanup_timer);
	{
		if (mariadb_idle_cleanup_timer)
		{
			mariadb_idle_cleanup_timer->cancel();
			mariadb_idle_cleanup_timer.reset(nullptr);
		}
	}
	io_work_ptr.reset(nullptr);
	threads.join_all();
	io_service.stop();
	LOG(DEBUG) << "extDB4: Closing... ";
}

void Ext::idleCleanup(const boost::system::error_code& ec)
{
	if (!ec)
	{
		for(auto &dbpool : mariadb_databases)
		{
				dbpool.second.idleCleanup();
		}
		std::lock_guard<std::mutex> lock(mutex_mariadb_idle_cleanup_timer);
		{
			mariadb_idle_cleanup_timer->expires_at(mariadb_idle_cleanup_timer->expires_at() + boost::posix_time::seconds(300));
			mariadb_idle_cleanup_timer->async_wait(boost::bind(&Ext::idleCleanup, this, _1));
		}
	}
}

void Ext::search(boost::filesystem::path &config_path, bool &conf_found, bool &conf_randomized)
{
	std::regex expression("extDB4-conf.*ini");
	for (boost::filesystem::directory_iterator it(config_path); it != boost::filesystem::directory_iterator(); ++it)
	{
		if (boost::filesystem::is_regular_file(it->path()))
		{
			if(std::regex_search(it->path().string(), expression))
			{
				conf_found = true;
				conf_randomized = true;
				config_path = boost::filesystem::path(it->path().string());
				ext_info.path = config_path.parent_path().string();
				break;
			}
		}
	}
}

bool Ext::connectDatabase(const std::string &database_conf, const std::string &database_id)
// Connection to Database, database_id used when connecting to multiple different database.
{
	if (mariadb_databases.count(database_id) > 0)
	{
		LOG(INFO) << "extDB4: Already Connected to a Database";
		return true;
	} else {
		try
		{
			std::string ip = ptree.get<std::string>(database_conf + ".IP");
			unsigned int port = ptree.get<unsigned int>(database_conf + ".Port");
			std::string username = ptree.get<std::string>(database_conf + ".Username");
			std::string password = ptree.get<std::string>(database_conf + ".Password");
			std::string database = ptree.get<std::string>(database_conf + ".Database");

			MariaDBPool *database_pool = &mariadb_databases[database_id];
			database_pool->init(ip, port, username, password, database);

			if (!mariadb_idle_cleanup_timer)
			{
				mariadb_idle_cleanup_timer.reset(new boost::asio::deadline_timer(io_service));
				mariadb_idle_cleanup_timer->expires_at(mariadb_idle_cleanup_timer->expires_at() + boost::posix_time::seconds(600));
				mariadb_idle_cleanup_timer->async_wait(boost::bind(&Ext::idleCleanup, this, _1));
			}
			return true;
		}
		catch (boost::property_tree::ptree_bad_path &e)
		{ 
			mariadb_databases.erase(database_id);
			LOG(ERROR) << "extDB4: Config Error: " << database_conf << " : " << e.what();
			return false;
		}
		catch (MariaDBConnectorException &e)
		{
			mariadb_databases.erase(database_id);
			LOG(ERROR) << "extDB4: MariaDBConnectorException: " << database_conf << " : " << e.what();
			return false;
		}
	}
}

bool Ext::addProtocol(const std::string &database_id, const std::string &protocol, const std::string &protocol_name, const std::string &init_data)
{
	std::lock_guard<std::mutex> lock(mutex_vec_protocols);
	auto foo = (std::find_if(vec_protocols.begin(), vec_protocols.end(), [=](const protocol_struct& elem) { return protocol_name == elem.name; }));
	if (foo != vec_protocols.end())
	{
		LOG(ERROR) << "extDB4: Error Protocol Name Already Taken: " << protocol_name;
		return false;
	}
	else
	{
		protocol_struct protocol_data;
		protocol_data.name = protocol_name;
		if (database_id.empty())
		{
			if (boost::algorithm::iequals(protocol, std::string("LOG")) == 1)
			{
				protocol_data.protocol.reset(new LOGProtocol());
			}
			else 
			{
				LOG(ERROR) << "extDB4: Failed to Load Unknown Protocol: " << protocol;
				return false;
			}
		}
		else
		{
			if (boost::algorithm::iequals(protocol, std::string("SQL")) == 1)
			{
				protocol_data.protocol.reset(new SQL());
			}
			else if (boost::algorithm::iequals(protocol, std::string("SQL_CUSTOM")) == 1)
			{
				protocol_data.protocol.reset(new SQL_CUSTOM());
			}
			else 
			{
				LOG(ERROR) << "extDB4: Failed to Load Unknown Protocol: " << protocol;
				return false;
			};
		};

		if (!protocol_data.protocol->init(this, database_id, init_data))
		{
			LOG(ERROR) << "extDB4: Failed to Load Protocol: " << protocol;
			return false;
		};

		vec_protocols.push_back(std::move(protocol_data));
		return true;
	};
}


static int output_size = 9999; // Temp

std::string Ext::getSinglePartResult_mutexlock(const unsigned long &unique_id)
// Gets Result String from unordered map array -- Result Formt == Single-Message
//   If <=, then sends output to arma, and removes entry from unordered map array
//   If >, sends [5] to indicate MultiPartResult
{
	std::lock_guard<std::mutex> lock(mutex_results);

	auto const_itr = stored_results.find(unique_id);
	if (const_itr == stored_results.end()) // NO UNIQUE ID
	{
		return "";
	}
	else // SEND MSG (Part)
	{
		if (const_itr->second.wait) // WAIT
		{
			return "[3]";
		}
		else if (const_itr->second.message.length() > output_size)
		{
			return "[5]";
		}
		else
		{
			return const_itr->second.message.c_str();
			stored_results.erase(const_itr);
		}
	}
}

std::string Ext::getMultiPartResult_mutexlock(const unsigned long &unique_id)
// Gets Result String from unordered map array  -- Result Format = Multi-Message
//   If length of String = 0, sends arma "", and removes entry from unordered map array
//   If <=, then sends output to arma
//   If >, then sends 1 part to arma + stores rest.
{
	std::lock_guard<std::mutex> lock(mutex_results);

	auto const_itr = stored_results.find(unique_id);
	if (const_itr == stored_results.end()) // NO UNIQUE ID or WAIT
	{
		return "";
	}
	else if (const_itr->second.wait)
	{
		return "[3]";
	}
	else if (const_itr->second.message.empty()) // END of MSG
	{
		stored_results.erase(const_itr);
		return "";
	}
	else // SEND MSG (Part)
	{
		if (const_itr->second.message.length() > output_size)
		{
			const_itr->second.message = const_itr->second.message.substr(output_size);
			return const_itr->second.message.substr(0, output_size).c_str();
		}
		else
		{
			const_itr->second.message.clear();
			return const_itr->second.message.c_str();
		}
	}
}

const unsigned long Ext::saveResult_mutexlock(const resultData &result_data)
// Stores Result String and returns Unique ID, used by SYNC Calls where message > outputsize
{
	std::lock_guard<std::mutex> lock(mutex_results);
	const unsigned long unique_id = unique_id_counter++;
	stored_results[unique_id] = std::move(result_data);
	stored_results[unique_id].wait = false;
	return unique_id;
}

void Ext::saveResult_mutexlock(const unsigned long &unique_id, const resultData &result_data)
// Stores Result String for Unique ID
{
	std::lock_guard<std::mutex> lock(mutex_results);
	stored_results[unique_id] = std::move(result_data);
	stored_results[unique_id].wait = false;
}

void Ext::saveResult_mutexlock(std::vector<unsigned long> &unique_ids, const resultData &result_data)
// Stores Result for multiple Unique IDs (used by Rcon Backend)
{
	std::lock_guard<std::mutex> lock(mutex_results);
	for (auto &unique_id : unique_ids)
	{
		stored_results[unique_id] = result_data;
		stored_results[unique_id].wait = false;
	}
}

std::string Ext::syncCallProtocol(std::string &protocol_name, std::string &query_str)
// Sync callPlugin
{
	auto const_itr = (std::find_if(vec_protocols.begin(), vec_protocols.end(), [=](const protocol_struct& elem) { return protocol_name == elem.name; }));
	if (const_itr == vec_protocols.end())
	{ 
		return "[0,\"Error Unknown Protocol\"]";
	}
	else
	{
		resultData result_data;
		result_data.message.reserve(output_size);

		const_itr->protocol->callProtocol(query_str, result_data.message, false);
		if (result_data.message.length() <= output_size)
		{
			return result_data.message.c_str();
		}
		else
		{
			const unsigned long unique_id = saveResult_mutexlock(result_data);
			return ("[2,\"" + std::to_string(unique_id) + "\"]").c_str();
		}
	} 
}

void Ext::onewayCallProtocol(std::string &input_str)
// ASync callProtocol
{
	const std::string::size_type found = input_str.find(":",2);
	if ((found==std::string::npos) || (found == (input_str.size() - 1)))
	{
		LOG(ERROR) << "extDB4: Invalid Format: " << input_str;
	}
	else
	{
		std::string protocol_name = input_str.substr(2, (found - 2));
		auto const_itr = (std::find_if(vec_protocols.begin(), vec_protocols.end(), [=](const protocol_struct& elem) { return protocol_name == elem.name; }));
		if (const_itr != vec_protocols.end())
		{
			resultData result_data;
			const_itr->protocol->callProtocol(input_str.substr(found+1), result_data.message, true);
		}
	}
}

void Ext::asyncCallProtocol(const int &output_size, const std::string &protocol_name, const std::string &data, const unsigned long unique_id)
// ASync + Save callProtocol
// We check if Protocol exists here, since its a thread (less time spent blocking arma) and it shouldnt happen anyways
{
	resultData result_data;
	result_data.message.reserve(output_size);
	auto const_itr = (std::find_if(vec_protocols.begin(), vec_protocols.end(), [=](const protocol_struct& elem) { return protocol_name == elem.name; }));
	if (const_itr->protocol->callProtocol(data, result_data.message, true, unique_id))
	{
		saveResult_mutexlock(unique_id, result_data);
	}
}

void Ext::getUPTime(std::string &token, std::string &result)
{
	uptime_current = std::chrono::steady_clock::now();
	auto uptime_diff = uptime_current - uptime_start;

	if (token == "HOURS")
		result = std::to_string(std::chrono::duration_cast<std::chrono::hours>(uptime_diff).count());
	else if (token == "MINUTES")
		result = std::to_string(std::chrono::duration_cast<std::chrono::minutes>(uptime_diff).count());
	else
		result = std::to_string(std::chrono::duration_cast<std::chrono::seconds>(uptime_diff).count());
}

void Ext::getDateAdd(std::string &token, std::string &token2, std::string &result)
{
	try
	{
		if (token.length() > 2)
		{
			token.erase(0, 1);
			token.pop_back();
			std::vector<std::string> vec;
			boost::split(vec, token, boost::is_any_of(","));

			if (vec.size() == 6)
			{
				int year = std::stoi(vec[0], nullptr);
				year = year - 1900;
				int month = std::stoi(vec[1], nullptr);
				month = month - 1;
				int day = std::stoi(vec[2], nullptr);
				int hour = std::stoi(vec[3], nullptr);
				int minute = std::stoi(vec[4], nullptr);
				int second = std::stoi(vec[5], nullptr);
				
				struct tm ptime_tm = { second,minute,hour,day,month,year };
				ptime = boost::posix_time::ptime_from_tm(ptime_tm);

				if (token2.length() > 2)
				{
					token2.erase(0, 1);
					token2.pop_back();
					std::vector<std::string> vec;
					boost::split(vec, token2, boost::is_any_of(","));
					if (vec.size() == 4)
					{
						int hours = std::stoi(vec[0], nullptr) * 24;
						hours = hours + std::stoi(vec[1], nullptr);
						int minutes = std::stoi(vec[2], nullptr);
						int seconds = std::stoi(vec[3], nullptr);

						boost::posix_time::time_duration diff(hours, minutes, seconds, 0);
						ptime = ptime + diff;

						facet = new boost::posix_time::time_facet();
						facet->format("[1,[%Y,%m,%d,%H,%M,%S]]");
						std::stringstream ss2;
						ss2.imbue(std::locale(std::locale::classic(), facet));
						ss2 << ptime;
						result = ss2.str();
					}
					else {
						result = "[0,\"Error Invalid Format\"]";
						LOG(ERROR) << "extDB4: addDate: invalid input: " << token2;
					}
				}
				else {
					result = "[0,\"Error Invalid Format\"]";
					LOG(ERROR) << "extDB4: addDate: invalid input2: " << token2;
				}
			}
			else {
				result = "[0,\"Error Invalid Format\"]";
				LOG(ERROR) << "extDB4: addDate: invalid input3: " << token;
			}

		}
	}
	catch(std::exception const &e)
	{
		result = "[0,\"Error Invalid Format\"]";
		LOG(ERROR) << "extDB4: addDate: stdException: " <<  e.what();
	}
}

void Ext::getLocalTime(std::string &result)
{
	ptime = boost::posix_time::second_clock::local_time();
	std::stringstream stream;
	facet = new boost::posix_time::time_facet();
	facet->format("[1,[%Y,%m,%d,%H,%M,%S]]");
	stream.imbue(std::locale(std::locale::classic(), facet));
	stream << ptime;
	result = stream.str();
}

void Ext::getLocalTime(std::string &input_str, std::string &result)
{
	try
	{
		ptime = boost::posix_time::second_clock::local_time();

		if (!(input_str.empty()))
		{
			int offset;
			if (input_str[0] == '[')
			{
				input_str.erase(0,1);
				input_str.pop_back();

				std::vector<std::string> tokens;
				boost::split(tokens, input_str, boost::is_any_of(","));
				for (unsigned int i = 0; i < tokens.size(); i++)
				{
					offset = std::stoi(tokens[i]);
					switch (i)
					{
						case 0:
							break;
						case 1:
							break;
						case 2:
							ptime += boost::posix_time::hours(offset * 24);
						case 3:
							ptime += boost::posix_time::hours(offset);
						case 4:
							ptime += boost::posix_time::minutes(offset);
						case 5:
							ptime += boost::posix_time::seconds(offset);
						default:
							break;
					}
				}
			} else {
				offset = std::stoi(input_str);
				ptime += boost::posix_time::hours(offset);
			}
		}

		std::stringstream stream;
		facet = new boost::posix_time::time_facet();
		facet->format("[1,[%Y,%m,%d,%H,%M,%S]]");
		stream.imbue(std::locale(std::locale::classic(), facet));
		stream << ptime;
		result = stream.str();
	}
	catch(std::exception& e)
	{
		result = "[0,\"ERROR\"]";
	}
}

std::string Ext::NumberToOrdinal(size_t number) {
	std::string suffix = "th";
	if (number % 100 < 11 || number % 100 > 13) {
		switch (number % 10) {
		case 1:
			suffix = "st";
			break;
		case 2:
			suffix = "nd";
			break;
		case 3:
			suffix = "rd";
			break;
		}
	}
	return std::to_string(number) + suffix;
}
