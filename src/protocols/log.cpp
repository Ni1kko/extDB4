#include "log.h"

#include <boost/filesystem.hpp>


bool LOGProtocol::init(AbstractExt *extension, const std::string &database_id, const std::string &init_str)
{
	/*extension_ptr = extension;

	bool status = false;

	if ((init_str.empty())) return false;
 
	try
	{
		boost::filesystem::path customlog(extension_ptr->ext_info.log_path);
		customlog /= init_str;
		if (customlog.parent_path().make_preferred().string() == extension_ptr->ext_info.log_path)
		{
			
			status = true;
		}
	}
	catch (std::exception e)
	{
		status = false;
	}

	return status; */
	return false;
}


bool LOGProtocol::callProtocol(std::string input_str, std::string &result, const bool async_method, const unsigned int unique_id)
{
	//logger->info(input_str.c_str());
	//result = "[1]";
	return true;
}
