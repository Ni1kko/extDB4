#include "ext.h"

#include <boost/filesystem.hpp>

#pragma warning(disable : 4996)

Ext *extension;

#ifdef __GNUC__
	#include <dlfcn.h>
	// Code for GNU C compiler
	static void __attribute__((constructor))
	extension_init(void)
	{

		Dl_info dl_info;
		dladdr((void*)extension_init, &dl_info);
		extension = new Ext(boost::filesystem::path (dl_info.dli_fname).string());
	}

	static void __attribute__((destructor))
	extension_destroy(void)
	{
		extension->stop();
	}

#elif _MSC_VER
	// Code for MSVC compiler
	//#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers   // Now Defined VIA CMake Build System

	#include <windows.h>
	#include <shellapi.h>
	#include <stdio.h>
	#include <cstdint>
	#include <sstream> 

	EXTERN_C IMAGE_DOS_HEADER __ImageBase;

	BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
	{
		switch (ul_reason_for_call)
		{
			case DLL_PROCESS_ATTACH:
				{
					WCHAR path[MAX_PATH + 1];
					GetModuleFileNameW((HINSTANCE)&__ImageBase, path, (MAX_PATH + 1));
					extension = new Ext(boost::filesystem::path(path).string());
				}
				break;
			case DLL_PROCESS_DETACH:
				extension->stop();
				break;
			case DLL_THREAD_ATTACH:
			case DLL_THREAD_DETACH:
				break;
		}
		return TRUE;
	}

#endif
 
int intercept::api_version() {
	return INTERCEPT_SDK_API_VERSION;
}

void intercept::pre_start(){
	ExtDB4::SQFCommands::Register();
}

void intercept::pre_init() {
	intercept::sqf::system_chat("ExtDB4 -> Preinit Running!");
}


using namespace intercept::types;

// getDatabaseVersion -> returns string
game_value ExtDB4::SQFCommands::Version() {
	return EXTDB_VERSION;
}

// getDatabaseLock -> returns bool
game_value ExtDB4::SQFCommands::Locked() {
	return extension->ext_info.extDB_lock;
}

// setDatabaseLock -> returns bool
game_value ExtDB4::SQFCommands::Lock(game_value_parameter left_arg, game_value_parameter right_arg) {
	
	bool inputState = (bool)right_arg;
	std::string inputCode = (std::string)left_arg;

	if (extension->ext_info.extDB_lock) {
	
		if (!inputState && inputCode == extension->ext_info.extDB_lockCode)
		{
			extension->ext_info.extDB_lockCode.clear();
			extension->ext_info.extDB_lock = false;
			return true;
		}
	}
	else 
	{
		if (inputState && !inputCode.empty())
		{
			extension->ext_info.extDB_lockCode = inputCode;
			extension->ext_info.extDB_lock = true;
			return true;
		}
	}
}

// resetDatabaseLock -> returns bool
game_value ExtDB4::SQFCommands::Reset() {
	if (!extension->ext_info.allow_reset) return false;
	extension->reset();
	return true;
}

// setDatabaseProfile -> returns bool
game_value ExtDB4::SQFCommands::SetProfile(game_value_parameter right_arg) {
	//return extension->connectDatabase((std::string)left_arg, (std::string)right_arg);
	return extension->connectDatabase((std::string)right_arg, (std::string)right_arg);
}

// setDatabaseProfileProtocol -> returns bool
game_value ExtDB4::SQFCommands::SetProfileProtocol(game_value_parameter left_arg, game_value_parameter right_arg) {

	if (extension->ext_info.extDB_lock) return false;

	auto& databaseID = left_arg;
	auto& params = right_arg.to_array();

	if (params.size() == 3)
		return extension->addProtocol(databaseID, params[0], params[1], params[2]); // ADD Database Protocol + Options
	else if (params.size() == 2)
		return extension->addProtocol(databaseID, params[0], params[1], ""); // ADD Database Protocol + No Options
	else
		return false;
}

// setDatabaseProtocol -> returns bool
game_value ExtDB4::SQFCommands::AddProtocol(game_value_parameter left_arg, game_value_parameter right_arg) {
	if (extension->ext_info.extDB_lock) return false;
	//return extension->addProtocol("", (std::string)left_arg,  right_arg[0],  right_arg[1]); // ADD + Init Options
	return extension->addProtocol("", (std::string)left_arg, (std::string)right_arg, "");
}

// getDatabaseUpTime -> returns string
game_value ExtDB4::SQFCommands::UpTime(game_value_parameter right_arg) {
	std::string result;
	extension->getUPTime((std::string)right_arg, result);
	return result;
}

// getDatabaseLocalTime -> returns string
game_value ExtDB4::SQFCommands::LocalTime() {
	std::string result;
	extension->getLocalTime(result);
	return result.c_str();
}

// getDatabaseDateAdd -> returns string
game_value ExtDB4::SQFCommands::DateAdd(game_value_parameter left_arg, game_value_parameter right_arg) {
	std::string result;
	extension->getDateAdd((std::string)left_arg, (std::string)right_arg, result);
	return result;
}

// databaseQuery -> returns string
game_value ExtDB4::SQFCommands::Query(game_value_parameter left_arg, game_value_parameter right_arg) {
	return extension->syncCallProtocol(std::string(left_arg), std::string(right_arg));
}

// databaseFireAndForget -> returns string
game_value ExtDB4::SQFCommands::FireAndForget(game_value_parameter left_arg, game_value_parameter right_arg) {
	std::string input_str = "1";
	input_str += ":";
	input_str += left_arg;
	input_str += ":";
	input_str += right_arg;
	extension->io_service.post(boost::bind(&Ext::onewayCallProtocol, extension, std::move(input_str)));//ASYNC
	LOG(INFO) << "databaseFireAndForget: Invoked -> Result: " << input_str;
	return true;
}

// databaseAsyncQuery -> returns string
game_value ExtDB4::SQFCommands::AsyncQuery(game_value_parameter left_arg, game_value_parameter right_arg) {
	std::string input_str = "2";
	input_str += ":";
	input_str += left_arg;
	input_str += ":";
	input_str += right_arg;

	// Protocol
	const std::string::size_type found = input_str.find(":", 2);
	if (found == std::string::npos)
	{
		LOG(ERROR) << "extDB4:Error Invalid Format";
		return "";
	}
	else {
		// Check for Protocol Name Exists...
		// Do this so if someone manages to get server, the error message wont get stored in the result unordered map
		const std::string protocol_name = input_str.substr(2, (found - 2));
		if ((std::find_if(extension->vec_protocols.begin(), extension->vec_protocols.end(), [=](const Ext::protocol_struct& elem) { return protocol_name == elem.name; })) != extension->vec_protocols.end()) //TODO Change to ITER
		{
			unsigned long unique_id;
			{
				std::lock_guard<std::mutex> lock(extension->mutex_results);
				unique_id = extension->unique_id_counter++;
				extension->stored_results[unique_id].wait = true;
			}
			extension->io_service.post(boost::bind(&Ext::asyncCallProtocol, extension, 999999, std::move(protocol_name), input_str.substr(found + 1), std::move(unique_id)));
		
			return "[2,\"" + std::to_string(unique_id) + "\"]";
		}
		else 
		{
			LOG(ERROR) << "extDB4: Error Unknown Protocol: " << protocol_name << "  Input String: " << input_str;
			return "";
		}
	}
}

// getDatabaseSinglePartMessage -> returns string
game_value ExtDB4::SQFCommands::SinglePartMessage(game_value_parameter right_arg) {
	const unsigned long unique_id = strtoul(((std::string)right_arg).c_str(), NULL, 0);
	return extension->getSinglePartResult_mutexlock(unique_id);
}

// getDatabaseMultiPartMessage -> returns string
game_value ExtDB4::SQFCommands::MultiPartMessage(game_value_parameter right_arg) {
	const unsigned long unique_id = strtoul(((std::string)right_arg).c_str(), NULL, 0);
	return extension->getMultiPartResult_mutexlock(unique_id);
}

// getDatabaseRandomString -> returns string
game_value ExtDB4::SQFCommands::RandomString(game_value_parameter right_arg) {
	int len(right_arg);
	static const char alphanum[] = {"0123456789" "ABCDEFGHIJKLMNOPQRSTUVWXYZ" "abcdefghijklmnopqrstuvwxyz"};
	
	bool makenew = true;
	std::string ouputString;

	while (makenew)
	{
		ouputString.reserve(len);
		for (int i = 0; i < len; ++i) ouputString += alphanum[rand() % (sizeof(alphanum) - 1)];

		if (StoredRandomStrings.empty()) {
			makenew = false;
			StoredRandomStrings.push_back(ouputString);
		}else if (std::find(StoredRandomStrings.begin(), StoredRandomStrings.end(), ouputString) != StoredRandomStrings.end()) {
		}else {
			makenew = false;
			StoredRandomStrings.push_back(ouputString);
		}
	}
	
	return ouputString;
}

// databaseQueryNew -> returns bool or string
game_value ExtDB4::SQFCommands::QueryNew(game_value_parameter left_arg, game_value_parameter right_arg) {

	auto const_itr = (std::find_if(extension->vec_protocols.begin(), extension->vec_protocols.end(), [=](const Ext::protocol_struct& elem) { return std::string(left_arg) == elem.name; }));

	if (const_itr == extension->vec_protocols.end())
		return false;

	AbstractExt::resultData result_data;
	const_itr->protocol->callProtocol(std::string(right_arg), result_data.message, false);
	return result_data.message.c_str();
}

// databaseShutdown -> returns bool
game_value ExtDB4::SQFCommands::Shutdown() {
	if(extension->ext_info.extDB_lock) return false;
	extension->stop();
	LOG(INFO) << "extDB4: Shutdown Requested";
	std::exit(EXIT_SUCCESS);
	return true;
}

// numberToOrdinal -> returns string
game_value ExtDB4::SQFCommands::NumberToOrdinal(game_value_parameter right_arg) {
	return extension->NumberToOrdinal(int(right_arg));
}

static void ExtDB4::SQFCommands::Register() {
	_Version = host::register_sqf_command("getDatabaseVersion", "", userFunctionWrapper<SQFCommands::Version>, GameDataType::STRING);
	_Locked = host::register_sqf_command("getDatabaseLock", "", userFunctionWrapper<SQFCommands::Locked>, GameDataType::BOOL);
	_Lock = host::register_sqf_command("setDatabaseLock", "", userFunctionWrapper<SQFCommands::Lock>, GameDataType::BOOL, GameDataType::STRING, GameDataType::BOOL);
	_Reset = host::register_sqf_command("resetDatabaseLock", "", userFunctionWrapper<SQFCommands::Reset>, GameDataType::BOOL);
	_SetProfile = host::register_sqf_command("setDatabaseProfile", "", userFunctionWrapper<SQFCommands::SetProfile>, GameDataType::BOOL, GameDataType::STRING);
	_SetProfileProtocol = host::register_sqf_command("setDatabaseProfileProtocol", "", userFunctionWrapper<SQFCommands::SetProfileProtocol>, GameDataType::BOOL, GameDataType::STRING, GameDataType::ARRAY);
	_AddProtocol = host::register_sqf_command("addDatabaseProtocol", "", userFunctionWrapper<SQFCommands::AddProtocol>, GameDataType::BOOL, GameDataType::STRING, GameDataType::STRING);
	_UpTime = host::register_sqf_command("getDatabaseUpTime", "", userFunctionWrapper<SQFCommands::UpTime>, GameDataType::STRING, GameDataType::STRING);
	_LocalTime = host::register_sqf_command("getDatabaseLocalTime", "", userFunctionWrapper<SQFCommands::LocalTime>, GameDataType::STRING);
	_DateAdd = host::register_sqf_command("getDatabaseDateAdd", "", userFunctionWrapper<SQFCommands::DateAdd>, GameDataType::STRING, GameDataType::STRING, GameDataType::STRING);
	_FireAndForget = host::register_sqf_command("databaseFireAndForget", "", userFunctionWrapper<SQFCommands::FireAndForget>, GameDataType::BOOL, GameDataType::STRING, GameDataType::STRING);
	_AsyncQuery = host::register_sqf_command("databaseAsyncQuery", "", userFunctionWrapper<SQFCommands::AsyncQuery>, GameDataType::STRING, GameDataType::STRING, GameDataType::STRING);
	_Query = host::register_sqf_command("databaseQuery", "", userFunctionWrapper<SQFCommands::Query>, GameDataType::STRING, GameDataType::STRING, GameDataType::STRING);
	_SinglePartMessage = host::register_sqf_command("getDatabaseSinglePartMessage", "", userFunctionWrapper<SQFCommands::SinglePartMessage>, GameDataType::STRING, GameDataType::STRING);
	_MultiPartMessage = host::register_sqf_command("getDatabaseMultiPartMessage", "", userFunctionWrapper<SQFCommands::MultiPartMessage>, GameDataType::STRING, GameDataType::STRING);
	_RandomString = host::register_sqf_command("getDatabaseRandomString", "", userFunctionWrapper<SQFCommands::RandomString>, GameDataType::STRING, GameDataType::SCALAR);
	_QueryNew = host::register_sqf_command("databaseQueryNew", "", userFunctionWrapper<SQFCommands::QueryNew>, GameDataType::ANY, GameDataType::STRING, GameDataType::STRING);
	_Shutdown = host::register_sqf_command("databaseShutdown", "", userFunctionWrapper<SQFCommands::Shutdown>, GameDataType::BOOL);
	_NumberToOrdinal = host::register_sqf_command("numberToOrdinal", "", userFunctionWrapper<SQFCommands::NumberToOrdinal>, GameDataType::STRING, GameDataType::SCALAR);
}