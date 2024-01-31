/*
    Project: ExtDB4
    Repo: https://github.com/Ni1kko/extDB4
*/

private _profileName = param [0,""];
private _prefix = param [1,"protocolname"];
if(count _prefix isEqualTo 0)exitWith{""};
if(count _profileName isEqualTo 0)exitWith{""};

private _protocolNameVariable = (["extdb_var_database"] + ([[_prefix, _profileName], ["lock"]] select (_profileName == "lock"))) joinString "_";
private _protocolkey = (uiNamespace getvariable _protocolNameVariable);
if(isNil "_protocolkey" or {typeName _protocolkey isNotEqualTo "CODE"})exitWith{""};

call _protocolkey