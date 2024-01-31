/*
    Project: ExtDB4
    Repo: https://github.com/Ni1kko/extDB4
*/

private _query = param [0,""];
private _profileName = param [1,""];

if(count _query isEqualTo 0)exitWith{false};
if(count _profileName isEqualTo 0)exitWith{false};

private _protocol = [_profileName,"protocolname"] call ExtDB4_fnc_getProtocolKey;
private _uniqueID = (parseSimpleArray(_protocol databaseAsyncQuery _query)) param [1,""];
private _queryResult = getDatabaseSinglePartMessage _uniqueID;

//Make sure the data is received
if (_queryResult isEqualTo "[3]") then {
    for "_i" from 0 to 1 step 0 do {
        if (_queryResult isNotEqualTo "[3]") exitWith {};
        _queryResult = getDatabaseSinglePartMessage _uniqueID;
    };
};

if (_queryResult isEqualTo "[5]") then {
    private _loop = true;
    for "_i" from 0 to 1 step 0 do { // extDB4 returned that result is Multi-Part Message
        _queryResult = "";
        for "_i" from 0 to 1 step 0 do {
            private _pipe = getDatabaseMultiPartMessage _uniqueID;
            if (_pipe isEqualTo "") exitWith {_loop = false};
            _queryResult = _queryResult + _pipe;
        };
        if (!_loop) exitWith {};
    };
};

_queryResult = call compile _queryResult;
if ((_queryResult select 0) isEqualTo 0) exitWith {
    [format ["Protocol Error: %1", _queryResult]] call Extdb4_fnc_log;
    []
};

(_queryResult select 1);