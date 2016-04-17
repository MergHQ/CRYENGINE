Script.ReloadScript("scripts/Utils/EntityUtils.lua")

-- Basic entity
UIEntity = {
	Properties = {
		soclasses_SmartObjectClass = "",	
		object_Model = "Objects/default/primitive_box.cgf",
	},

	Client = {},
	Server = {},
	
	-- Temp.
	_Flags = {},
	
		Editor={
		Icon = "physicsobject.bmp",
		IconOnTop=1,
	  },
			
}

------------------------------------------------------------------------------------------------------
function UIEntity:OnSpawn()
	self:SetFromProperties();	
end


----------------------------------------------------------------------------------------------------
function UIEntity:OnPreFreeze(freeze, vapor)
end

----------------------------------------------------------------------------------------------------
function UIEntity:CanShatter()
	return false;
end

------------------------------------------------------------------------------------------------------
function UIEntity:SetFromProperties()
	local Properties = self.Properties;

	if (Properties.object_Model == "") then
		do return end;
	end
	
	self.freezable=tonumber(Properties.bFreezable)~=0;
	
	self:LoadObject(0,Properties.object_Model);
	self:SetSlotHud3D(0);
end

------------------------------------------------------------------------------------------------------
function UIEntity:IsRigidBody()
  	return false;
end

------------------------------------------------------------------------------------------------------
function UIEntity:PhysicalizeThis()
end

------------------------------------------------------------------------------------------------------
-- OnPropertyChange called only by the editor.
------------------------------------------------------------------------------------------------------
function UIEntity:OnPropertyChange()
	self:SetFromProperties();
end

------------------------------------------------------------------------------------------------------
-- OnReset called only by the editor.
------------------------------------------------------------------------------------------------------
function UIEntity:OnReset()
end

------------------------------------------------------------------------------------------------------
function UIEntity.Client:OnPhysicsBreak( vPos,nPartId,nOtherPartId )
end

------------------------------------------------------------------------------------------------------

function UIEntity:IsUsable(user)	
	return 0
end
