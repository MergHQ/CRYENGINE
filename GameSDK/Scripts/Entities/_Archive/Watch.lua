Script.ReloadScript( "Scripts/Default/Entities/Others/BasicEntity.lua" );

-- simple entity

Watch_x = {
	Name = "Watch",

	-- Test Model: Objects\Indoor\props\office_items\clock_animated.cga	
	CustomProperties = 
	{
	},
}

------------------------------------------------------------------------------------------------------
function Watch_x:OnInit()
	self.__super.OnInit(self);
	self:StartAnimation( 0,"default",0,0,1,1 );
	self:GotoState("");
end

------------------------------------------------------------------------------------------------------
function Watch_x:OnPropertyChange()
	self.__super.OnPropertyChange(self);
	self:StartAnimation( 0,"default",0,0,1,1 );
end

------------------------------------------------------------------------------------------------------
function Watch_x:OnReset()
	self.__super.OnReset(self);
	self:SetFromProperties();
	self:StartAnimation( 0,"default",0,0,1,1 );
end

function Watch_x:OnUpdate()
	local tm = System.GetLocalOSTime();
	--System.Log( "Time:"..tm.hour..":"..tm.min..":"..tm.sec );
	
	local hour = tm.hour;
	if (hour > 12) then
		hour = hour-12;
	end
	-- Animation played by default 30f/s, 720 frames.
	local frame = tm.hour*60 + tm.min + tm.sec/60;
	self:SetAnimationSpeed( 0 );
	self:SetAnimationTime( 0,0,frame/30 );
end

----------------------------------------------------------------------------------
-- Must be last in file.
----------------------------------------------------------------------------------
function CreateWatchEntity(child)
	local newt={}
	mergef(newt,BasicEntity,1);
	mergef(newt,child,1);
	
	-- Add some more properties.
	mergef(newt.Properties,child.CustomProperties,1);
	
	newt.__super = BasicEntity;
	-- Default model.
	newt.Properties.object_Model = "Objects/Indoor/props/office_items/clock_animated.cga"
	
	return newt;
end
Watch = CreateWatchEntity(Watch_x);