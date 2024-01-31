/*
 * extDB4
 * © 2016 Declan Ireland
 */

#pragma once

#include <iostream>
#include <exception>
#include <string>


class extDB4Exception : public std::exception
{
public:
	extDB4Exception(std::string msg) : msg(msg) {}
	virtual const char* what() const throw()
	{
		return msg.c_str();
	}
private:
	std::string msg;
};


class MariaDBConnectorException: public std::exception
{
public:
	MariaDBConnectorException(MYSQL *mysql_ptr) : mysql_ptr(mysql_ptr) {}
	virtual const char* what() const throw()
	{
  	return mysql_error(mysql_ptr);
	}
private:
	MYSQL *mysql_ptr;
};


class MariaDBStatementException0: public std::exception
{
public:
	MariaDBStatementException0(MYSQL *mysql_ptr) : mysql_ptr(mysql_ptr) {}
	virtual const char* what() const throw()
	{
		#ifdef DEBUG_TESTING
			std::cout << "ERROR" << std::endl;
			std::cout << mysql_error(mysql_ptr) << std::endl;
		#endif
		return mysql_error(mysql_ptr);
	}
private:
  MYSQL *mysql_ptr;
};


class MariaDBStatementException1: public std::exception
{
public:
	MariaDBStatementException1(MYSQL_STMT *mysql_stmt_ptr) : mysql_stmt_ptr(mysql_stmt_ptr) {}
	virtual const char* what() const throw()
	{
		#ifdef DEBUG_TESTING
			std::cout << "ERROR" << std::endl;
			std::cout << mysql_stmt_error(mysql_stmt_ptr) << std::endl;
		#endif
		return mysql_stmt_error(mysql_stmt_ptr);
	}
private:
	MYSQL_STMT *mysql_stmt_ptr;
};


class MariaDBQueryException: public std::exception
{
public:
	MariaDBQueryException(MYSQL *mysql_ptr): mysql_ptr(mysql_ptr) {}
	virtual const char* what() const throw()
	{
		return mysql_error(mysql_ptr);
	}
private:
	MYSQL *mysql_ptr;
};
