#pragma once

#include "abstract_protocol.h"


class SQL: public AbstractProtocol
{
public:
	bool init(AbstractExt *extension, const std::string &database_id, const std::string &options_str);
	bool callProtocol(std::string input_str, std::string &result, const bool async_method, const unsigned int unique_id=1);

private:
	MariaDBPool *database_pool;

	int check_dataType_string = 0;
	bool check_dataType_null = false;
};
