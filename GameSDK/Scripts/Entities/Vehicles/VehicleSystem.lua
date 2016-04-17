--------------------------------------------------------------------------
--	Crytek Source File.
-- 	Copyright (C), Crytek Studios, 2001-2004.
--------------------------------------------------------------------------
--	$Id$
--	$DateTime$
--	Description: Common code for all the vehicle implementations
--  
--------------------------------------------------------------------------
--  History:
--  - 17/12/2004   14:43 : Created by Mathieu Pinard
--
--------------------------------------------------------------------------

V_XML_DEF = "Scripts/Entities/Vehicles/def_vehicle.xml";
V_SCRIPT_DIR = "Scripts/Entities/Vehicles/Implementations/";
V_XML_DIR = V_SCRIPT_DIR.."Xml/";

-- convenience defines to use with GetHelperPos, GetHelperDir (on vehicle and entity)
HELPER_WORLD = 0;
HELPER_LOCAL = 1;


VehicleSystem = {};


-- get vehicle implementations
if (not VehicleSystem.VehicleImpls) then  
  VehicleSystem.VehicleImpls = Vehicle.GetVehicleImplementations(V_XML_DIR);
  --Log("[VehicleSystem]: got %i vehicle implementations", count(VehicleSystem.VehicleImpls));
end


--------------------------------------------------------------------------
function ExposeVehicleToNetwork( class )
	Net.Expose {
		Class = class,
		ClientMethods = {
		},

		ServerMethods = {
		},
	}
end

--------------------------------------------------------------------------
function VehicleSystem.ReloadVehicleSystem()
  Log("Reloading VehicleSystem");
  Vehicle.ReloadSystem();
  Script.ReloadScript( "Scripts/Entities/Vehicles/VehicleSystem.lua" );        
end