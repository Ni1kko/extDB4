# Description
extDB4 is an Arma3 Addon for connecting to Databases (currently only MariaDB/MySQL).
The main purpose for extDB4 is for persistent missions in Arma.
Note it will require some knowledge about SQF & SQL to use.
extDB4 is also designed to be flexible & secure at the same time.

### NEW METHODS
- getDatabaseRandomString 6;                                                            // returns *string*
- databaseShutdown;                                                                     // returns *bool* true on success (requires database being unlocked!)
- "Unique_ProtocolName" databaseQueryNew "Some SQL query Here";                         // returns *string* (requested query)
- numberToOrdinal 3;                                                                    // returns *string*

### FULLY WORKING
- getDatabaseVersion;                                                                   // returns *string*
- getDatabaseLock;                                                                      // returns *bool* true if locked
- setDatabaseProfile "ExtDB_Profile1";                                                  // returns *bool* true on success
- "ExtDB_Profile1" setDatabaseProfileProtocol ["SQL","Unique_ProtocolName","TEXT2"];    // returns *bool* true on success
- "Unique_ProtocolName2" addDatabaseProtocol "LOG";                                     // returns *bool* true on success
- "Unique_LockKey" setDatabaseLock true;                                                // returns *bool* true on success
- resetDatabaseLock;                                                                    // returns *bool* true on success (requires database being unlocked!)
- "Unique_ProtocolName" databaseFireAndForget "Some SQL query Here";                    // returns *bool* (always true)
- "Unique_ProtocolName" databaseQuery "Some SQL query Here";                            // returns *string* (query "Unique_QueryID")
- "Unique_ProtocolName" databaseAsyncQuery "Some SQL query Here";                       // returns *string* (query "Unique_QueryID")
- getDatabaseSinglePartMessage "Unique_QueryID";                                        // returns *string* (requested query)
- getDatabaseMultiPartMessage "Unique_QueryID";                                         // returns *string* (requested query) 
- getDatabaseUpTime "MINUTES";                                                          // returns *string*

### FULLY WORKING BUT NEED TO HANDLE 2ND METHOD WITH DIFF PARAMS
- getDatabaseLocalTime;                                                                 // returns *string*

### ADDED BUT UNTESTED
- "" getDatabaseDateAdd "";                                                             // returns *string*

### EXAMPLE SNIPPET
```SQF
//--- Gen random strings
private _keyLength = random [6,9,12];
private _protocolName = getDatabaseRandomString _keyLength;
private _databaseLock = getDatabaseRandomString _keyLength;

//-- Connect to profile (ExtDB_Profile1) inside (extdb4-conf.ini)
private _profileName = "ExtDB_Profile1";

try{
    //--- Loaded
    if(isNil compile "getDatabaseVersion") throw "Error plugin is not loaded!";

    //--- Version
    if(parseNumber(getDatabaseVersion) < 1.001) throw "Error plugin is outdated!";

    //--- Profile
    if(not(setDatabaseProfile _profileName)) throw "Error with Database Profile";

    //--- Protocol
    if(not(_profileName setDatabaseProfileProtocol ["SQL",_protocolName,"TEXT2"])) throw "Error with Database Protocol";

    //--- Lock profile
    if(not(_databaseLock setDatabaseLock true) or {not(getDatabaseLock)}) throw "Error Locking Database Profile";

}catch{
    diag_log format ["ExtDB4: %1", _exception];
    _protocolName = "";
    _databaseLock = "";
};

//-- Fires a database request without a response
_protocolName databaseFireAndForget "INSERT INTO `players` (`steamid`, `name`, `aliases`, `cash`, `bank`, `gear`, `admin`, `banned`, `alive`, `position`, `insert_time`, `last_seen`) VALUES ('76561199242507277', 'Nikko R. (Uncle Scrooge)', 'Ni1kk', 0, 100000, '[]', 1, 1, 1, '[]', '2023-10-25 02:09:08', '2023-11-11 18:27:13')";  

//-- Database request with a response 
private _uniqueID = _protocolName databaseAsyncQuery "SELECT * FROM players WHERE steamid='76561199242507277'";
getDatabaseSinglePartMessage _uniqueID;

```

### Clone the repo 
`git clone --recurse-submodules https://github.com/Ni1kko/extDB4`

### Generate Build Files
Windows x86 - `cmake .. -G "Visual Studio 17 2022"`
Windows x64 - `cmake .. -G "Visual Studio 17 2022 Win64"`
Linux - `cmake .. -G`