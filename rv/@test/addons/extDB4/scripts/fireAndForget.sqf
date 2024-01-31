/*
    Project: ExtDB4
    Repo: https://github.com/Ni1kko/extDB4
*/

private _query = param [0,""];
private _profileName = param [1,""];

if(count _query isEqualTo 0)exitWith{false};
if(count _profileName isEqualTo 0)exitWith{false};

private _protocol = [_profileName,"protocolname"] call ExtDB4_fnc_getProtocolKey;

_protocol databaseFireAndForget _query;  