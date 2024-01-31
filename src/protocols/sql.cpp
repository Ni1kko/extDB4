#include "sql.h"

#include "../mariaDB/exceptions.h"
#include "../mariaDB/session.h"

#include <boost/algorithm/string.hpp>
#include <logger/logging.hpp>


bool SQL::init(AbstractExt *extension, const std::string &database_id, const std::string &options_str)
{
	extension_ptr = extension;

	if (extension_ptr->mariadb_databases.count(database_id) == 0)
	{ 
		LOG(INFO) << "extDB4: SQL: No Database Connection: " << database_id;
		return false;
	}
	database_pool = &extension->mariadb_databases[database_id];

	std::vector<std::string> tokens;
	boost::split(tokens, options_str, boost::is_any_of("-"));
	for (auto& token : tokens)
	{
		if (boost::algorithm::iequals(token, std::string("TEXT")))
		{
			check_dataType_string = 1;
		}
		if (boost::algorithm::iequals(token, std::string("TEXT2")))
		{
			check_dataType_string = 2;
		}
		else if (boost::algorithm::iequals(token, std::string("NULL")))
		{
			check_dataType_null = true;
		}
	}

	if (check_dataType_string > 0)
		LOG(INFO) << "extDB4: SQL: Initialized: Add Quotes around TEXT Datatypes mode: " << check_dataType_string;
	
	if (check_dataType_null)
		LOG(INFO) << "extDB4: SQL: Initialized: NULL = objNull";
	else
		LOG(INFO) << "extDB4: SQL: Initialized: NULL = \"\"";

	return true;
}


bool SQL::callProtocol(std::string input_str, std::string &result, const bool async_method, const unsigned int unique_id)
{
	try
	{
		std::string insertID = "0";
		MariaDBSession session(database_pool);
		session.data->query.send(input_str);

		std::vector<std::vector<std::string>> result_vec;
		session.data->query.get(check_dataType_string, check_dataType_null, insertID, result_vec);

		result = "[";
		if (result_vec.size() > 0)
		{
			for(auto &row: result_vec)
			{
				result += "[";
				if (row.size() > 0)
				{
					for(auto &field: row)
					{
						if (field.empty())
						{
							result += "\"\"";
						} else {
							result += field;
						}
						result += ",";
					}
					result.pop_back();
				}
				result += "],";
			}
			result.pop_back();
		}
		result += "]";

		#ifdef DEBUG_LOGGING
			LOG(ERROR) << "extDB4: SQL: Trace: Result: " << result;
		#endif
	}
	catch (MariaDBQueryException &e)
	{
		LOG(ERROR) << "extDB4: SQL: Error MariaDBQueryException: " << e.what();
		LOG(ERROR) << "extDB4: SQL: Error MariaDBQueryException: Input:" << input_str;
		result = "[0,\"Error MariaDBQueryException Exception\"]";
	}
	catch (MariaDBConnectorException &e)
	{
		LOG(ERROR) << "extDB4: SQL: Error MariaDBConnectorException: " << e.what();
		LOG(ERROR) << "extDB4: SQL: Error MariaDBConnectorException: Input:" << input_str;
		result = "[0,\"Error MariaDBConnectorException Exception\"]";
	}
	return true;
}
