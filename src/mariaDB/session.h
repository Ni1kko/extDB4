/*
 * extDB4
 * © 2016 Declan Ireland
 */

#pragma once

#include "pool.h"


class MariaDBSession
{
public:
	MariaDBSession(MariaDBPool *database_pool);
	~MariaDBSession();

	std::unique_ptr<MariaDBPool::mariadb_session_struct> data;

	void resetSession();
private:
	MariaDBPool *database_pool_ptr;
};
