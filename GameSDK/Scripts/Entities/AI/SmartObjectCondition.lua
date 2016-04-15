--System.Log("Loading SmartObjectCondition.lua");

SmartObjectCondition = {
  type = "SmartObjectCondition",
  Properties = 
  {
  	bEnabled = true,
  	bIncludeInNavigation = true,
  	bRelativeToTarget = false,
  	Object = {
  		soclass_Class = "BasicEntity",
  		sostate_State = "Idle",
  		object_Model = "",
  	},
  	User = {
  		soclass_Class = "AIOBJECT_ACTOR",
  		sostate_State = "Idle",
  		object_Model = "",
  	},
  	Limits = {
  		fDistance = 10.0,
  		fOrientation = 360.0,
  	},
  	Delay = {
  		fMinimum = 0.5,
  		fMaximum = 15.0,
  		fMemory = 1.0,
  	},
  	Multipliers = {
  		fProximity = 1.0,
  		fOrientation = 0.0,
  		fVisibility = 0.0,
  		fRandomness = 0.25,
  	},
  	Action = {
  		soaction_Name = "",
  		sostate_ObjectState = "Busy",
  		sostate_UserState = "Busy",
  	},
  },
	
	Editor={
		Model="Editor/Objects/Pyramid.cgf",
	},
}

-------------------------------------------------------
function SmartObjectCondition:OnPropertyChange()
--	self:Register();
end

-------------------------------------------------------
function SmartObjectCondition:OnInit()
--	self:Register();
end

-------------------------------------------------------
function SmartObjectCondition:OnReset()
	self:Register();
end

-------------------------------------------------------
function SmartObjectCondition:Register()
--	This entity shouldn't be used anymore!
--	AI.AddSmartObjectCondition( self.Properties );
end
