/*
    Project: ExtDB4
    Repo: https://github.com/Ni1kko/extDB4
*/

private  _lines = [[_this], _this] select (typeName _this isEqualTo "ARRAY");
  
{
    private _line = param[0,""];
    if(count _line > 0) then {
        private _logEntry = format ["ExtDB4: %1",_line];
        if(hasInterface)then{systemChat _logEntry};
        diag_log _logEntry;
    };
}forEach _lines;

if(hasInterface)then{ 
    private _headerWithLines = [["<t size='2.0'>ExtDB4</t>"], _lines, 1] call BIS_fnc_arrayInsert;
    hint parseText (_headerWithLines joinString "<br/>");  
};

true