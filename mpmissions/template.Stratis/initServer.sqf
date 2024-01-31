/*
    Author: Ni1kko (Uncle Scrooge)
    Repo: https://github.com/Ni1kko/extDB4
*/

//-- Connects to profile (ExtDB_Profile1) inside (extdb4-conf.ini)
//ExtDB4_var_setupConnectionThread = ["ExtDB_Profile1"] spawn ExtDB4_fnc_setupConnection;

//-- Database request without a response
//_query = "INSERT INTO `players` (`steamid`, `name`, `aliases`, `cash`, `bank`, `gear`, `admin`, `banned`, `alive`, `position`, `insert_time`, `last_seen`) VALUES ('76561199242507277', 'Nikko R. (Uncle Scrooge)', 'Ni1kk', 0, 100000, '[]', 1, 1, 1, '[]', '2023-10-25 02:09:08', '2023-11-11 18:27:13')";
//[_query, "ExtDB_Profile1"] call ExtDB4_fnc_fireAndForget;

//-- Database request with a response
//_query = "SELECT * FROM players WHERE steamid='76561199242507277'";
//[_query, "ExtDB_Profile1"] call ExtDB4_fnc_asyncRequest;

//-- Setup Database connections
[] spawn 
{
    systemChat "Waiting for ExtDB4 addon to load!";
    waitUntil {not(isNil "ExtDB4_var_isReady")};
    
    systemChat "Waiting for ExtDB4 addon to ready up!";
    waitUntil {missionNamespace getVariable ["ExtDB4_var_isReady",false]};

    private _profiles = [
        //["ExtDB_Profile1", "SQL", "TEXT2"]
        //["ExtDB_Profile2", "SQL", "TEXT2-NULL"]
        //["ExtDB_Profile3", "SQL_CUSTOM_V2", "extdb4-sqlcustom.ini"]
        ["ExtDB_Profile1", "SQL", "TEXT2"]
    ];
    
    //-- Connects to profiles inside (extdb4-conf.ini)
    {
        private _connected = _x call ExtDB4_fnc_setupConnection;
        if _connected then{
            systemChat "Database connected!";
        };
    }forEach _profiles;

    //-- Database tests
    ExtDB4_var_testLoadPlayer_Tries = 0;
    ExtDB4_fnc_testLoadPlayer = {
        private _protocol = "ExtDB_Profile1" call ExtDB4_fnc_getProtocolKey;
        private _results = parseSimpleArray(_protocol databaseQueryNew "SELECT name, aliases, cash, bank FROM players WHERE steamid='76561199242507277'");

        if(ExtDB4_var_testLoadPlayer_Tries > 3) exitWith {
            systemChat "Database request: loading: player: failed <76561199242507277>!";
        };

        if(count _results isEqualTo 0)exitWith{
            systemChat "Database request: failed: player: <76561199242507277> not found!";
            _query = "INSERT INTO `players` (`steamid`, `name`, `aliases`, `cash`, `bank`, `gear`, `admin`, `banned`, `alive`, `position`, `insert_time`, `last_seen`) VALUES ('76561199242507277', 'Nikko R. (Uncle Scrooge)', 'Ni1kk', 0, 100000, '[]', 1, 1, 1, '[]', '2023-10-25 02:09:08', '2023-11-11 18:27:13')";
            [_query, "ExtDB_Profile1"] call ExtDB4_fnc_fireAndForget;
            systemChat "Database request: inserted: player: <76561199242507277>!";
            ExtDB4_var_testLoadPlayer_Tries = ExtDB4_var_testLoadPlayer_Tries + 1;
            uiSleep 1;
            [] spawn ExtDB4_fnc_testLoadPlayer;
        };

        private _selectedResult = _results param [0,[]];
        
        _selectedResult params [
            ["_name", ""],
            ["_aliases", []],
            ["_cash", 0],
            ["_bank", 0]
        ];
        
        systemChat "Database request: success";
        systemChat format ["Database request: _name = %1", _name];
        systemChat format ["Database request: _aliases = %1", _aliases];
        systemChat format ["Database request: _cash = %1", _cash];
        systemChat format ["Database request: _bank = %1", _bank];

    };
    
    systemChat "Starting ExtDB4 Test!";
    for "_i" from 0 to 10 do {
        uiSleep 1;
        systemChat str (10 - _i);
    };
    [] call ExtDB4_fnc_testLoadPlayer;
    
    
    for "_i" from 0 to 60 do {
        if(_i mod 10 == 0)then{
            systemChat format ["Shutdown ExtDB4 Test - %1 Seconds left!", 60 - _i];
        };
        uiSleep 1;
    };

    if(not(databaseShutdown))then{;
        private _lockKey = ["lock"] call ExtDB4_fnc_getProtocolKey;
        _lockKey setDatabaseLock false;
        systemChat "Shuting down ExtDB4";
        uiSleep 1;
        databaseShutdown;
    };
};
