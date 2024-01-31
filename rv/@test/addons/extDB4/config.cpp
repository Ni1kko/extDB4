class CfgPatches 
{
    class extDB4 
    {
        units[] = {""};
        weapons[] = {};
        requiredVersion = 0.1;
        requiredAddons[] = { "Intercept_Core" };
        version = 0.1;
    };
};

//--- Limits RPT file output to one file only!
rptFileLimit=1;

class Intercept 
{
    class ExternalDatabase 
    {
        class extDB4 
        {
			//certificate = "core";
            pluginName = "extDB4";
        };
    };
};

class CfgFunctions 
{
    class ExtDB4
	{
		class Root
		{
            file = "\extDB4";
            class preInit {preInit = 1;};
		};
	};
};