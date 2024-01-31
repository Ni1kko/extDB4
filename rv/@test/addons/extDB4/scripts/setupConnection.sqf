/*
    Project: ExtDB4
    Repo: https://github.com/Ni1kko/extDB4
*/

private _profileName = param [0, ""];
private _profileProtocol = param [1, "SQL"];
private _profileProtocolInit = param [2,"TEXT2"];
private _sqlcustom = _profileProtocol isEqualTo "SQL_CUSTOM_V2";

private _protocolNameVariable = ["extdb_var_database_protocolname", _profileName] joinString "_";
private _lockVariable = "extdb_var_database_lock";
private _connectedVariable = ["extdb_var_database_connected", _profileName] joinString "_";
private _preparedVariable = ["extdb_var_database_prepared", _profileName] joinString "_";

private _isConnected = not(isNil {uiNamespace getVariable _connectedVariable});
private _connectedProfiles = uiNamespace getVariable ["extdb_var_database_profiles",[]];
private _connectedProfilesCount = (count _connectedProfiles) + 1;
private _databaseLockKey = uiNamespace getVariable [_lockVariable, ""];

try{
    //-- Nope! A client tried to call function
    if(isRemoteExecuted) throw "Error remote execution not allowed!";

    //--- bad params
    if(count(_profileName) isEqualTo 0) throw "Error no profile selected!";

    //--- Not a server
    if(not(isServer)) throw "Error plugin is only for servers!";

    //--- Loaded
    if(isNil compile "getDatabaseVersion") throw "Error plugin is not loaded!";

    //--- Version
    if(parseNumber(getDatabaseVersion) < 1.001) throw "Error plugin is outdated!";

    //--- Protocol already loaded
    if(_isConnected)then{
        format["Database#%1 Already Connected Using Profile: (%2)",_connectedProfilesCount, _profileName] call ExtDB4_fnc_log;
    }
    else
    {
        private _keyLength = random [6,9,12];
        private _protocolName = getDatabaseRandomString _keyLength;

        if(_databaseLockKey == "")then{
            _databaseLockKey = getDatabaseRandomString _keyLength;
        };

        //--- Unlock database
        if(getDatabaseLock) then{
            _databaseLockKey setDatabaseLock false;
            //-- Could not unlock
            if (getDatabaseLock) throw "Error Unlocking Database";
        };
        
        //--- Load profile
        if(not(setDatabaseProfile _profileName)) throw "Error with Database Profile";
        [format ["Profile (%1) Loaded",_profileName]] call ExtDB4_fnc_log;
    
        //--- Set protocol for loaded profile
        if(not(_profileName setDatabaseProfileProtocol [_profileProtocol,_protocolName,_profileProtocolInit])) throw "Error with Database Protocol";
        [format ["SQL%1 Protocol Loaded",["Raw","Custom"]select _sqlcustom]] call ExtDB4_fnc_log;

        //--- Lock profile
        if(not(_databaseLockKey setDatabaseLock true) or {not(getDatabaseLock)}) throw "Error Locking Database";
        [format ["Profile (%1) Locked with code: %2",_profileName,_databaseLockKey]] call ExtDB4_fnc_log;
        
        //--- Connection OKAY
        _isConnected = true;
        _connectedProfiles pushBackUnique _profileName;
        uiNamespace setvariable [_preparedVariable, compile str(_sqlcustom)];
        uiNamespace setvariable [_protocolNameVariable, compile str(_protocolName)];
        uiNamespace setvariable [_lockVariable, compile str(_databaseLockKey)];
        uiNamespace setvariable [_connectedVariable, compile str(_isConnected)];
        [format["Database#%1 Connected Using Profile: (%2)",_connectedProfilesCount, _profileName]] call ExtDB4_fnc_log;
    };
}catch{
    [_exception] call ExtDB4_fnc_log;
    _isConnected = false;
    //databaseShutdown;
};

_isConnected