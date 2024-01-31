/*
    Project: ExtDB4
    Repo: https://github.com/Ni1kko/extDB4
*/

//-- system vars
ExtDB4_var_isReady = false;
publicVariableServer "ExtDB4_var_isReady";

//-- Compile scripts 
{
    private _parts = _x splitString "\";
    private _folders = _parts select [1, count _parts - 2];
    if("scripts" in _folders)then{
        private _fileName = _parts call BIS_fnc_arrayPop;
        private _functionName = ["ExtDB4", "fnc", _fileName select [0, count _fileName - 4]] joinString "_";
        diag_log format["ExtDB4: Compiling function <%1>", _functionName];
        missionNamespace setVariable [_functionName, compileScript [_x, true]];
    };
}forEach addonFiles ["extdb4\", ".sqf"];

//-- update system vars
ExtDB4_var_isReady = true;
publicVariableServer "ExtDB4_var_isReady";

true