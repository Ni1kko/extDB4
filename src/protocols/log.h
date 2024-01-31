#pragma once

#include "abstract_protocol.h"


class LOGProtocol : public AbstractProtocol
{
public:
	bool init(AbstractExt *extension, const std::string &database_id, const std::string &init_str);
	bool callProtocol(std::string input_str, std::string &result, const bool async_method, const unsigned int unique_id=1);

//private:
};
