InteractiveObjectEx = {
	type = "InteractiveObjectEx",
	Properties = {
		
		Interaction = {

			-- All interactions share a single CGA which loads the geometry (in the case of one interaction this also supplies the ANM)
			object_Model = "Objects/prototype/ElevatorDoors/AnimatedElevatorDoors.cga",

			-- Old style specify one specific interaction
			Interaction =  "OpenElevatorDoor", 
			InteractionRadius = 1.5,
			InteractionAngle = 70,

			bRemoveDecalsOnUse = 0,
			bStartInteractionOnExplosion = 0,
		},
		
		Physics = {
			bRigidBody = 0,
		},
	},
	
	Editor={
		Icon="animobject.bmp",
		IconOnTop=1,
	},
	
	userId = NULL_ENTITY,
}

function InteractiveObjectEx:OnInit()
	self:OnReset();
end

function InteractiveObjectEx:OnPropertyChange()
	self:OnReset();
end

function InteractiveObjectEx:OnReset()
	self.bUsable=self.Properties.bUsable;
end

function InteractiveObjectEx:OnSave(tbl)
	tbl.bUsable=self.bUsable;
	tbl.userId = self.userId;
end

function InteractiveObjectEx:OnLoad(tbl)
	self.bUsable=tbl.bUsable;
	self.userId = tbl.userId;
end

function InteractiveObjectEx:OnShutDown()
end


----------------------------------------------------------------------------------
------------------------------------Events----------------------------------------
----------------------------------------------------------------------------------

function InteractiveObjectEx:Event_Use()
	self.interactiveObject:Use(self.userId);
	self:ActivateOutput( "Started", true );
end;

function InteractiveObjectEx:Event_UserId(sender, user)
	self.userId = user.id;
end;

function InteractiveObjectEx:Event_Hide()
	self:Hide(1);
	self:ActivateOutput( "Hide", true );
end;

function InteractiveObjectEx:Event_UnHide()
	self:Hide(0);
	self:ActivateOutput( "UnHide", true );
end;

function InteractiveObjectEx:Event_EnableUsable()
	self.bUsable=1;
	self:ActivateOutput( "Enabled", true );
end;

function InteractiveObjectEx:Event_DisableUsable()
	self.bUsable=0;
	self:ActivateOutput( "Disabled", true );
end;

function InteractiveObjectEx:Event_Reset()
	Game.ResetEntity(self.id);
	self:ActivateOutput( "Reset", true );
end;

------------------------------------------------------------------------------------------------------
-- Flow events.
------------------------------------------------------------------------------------------------------

InteractiveObjectEx.FlowEvents =
{
	Inputs =
	{
		Use = { InteractiveObjectEx.Event_Use, "bool" },
		UserId = { InteractiveObjectEx.Event_UserId, "entity" },
		Hide = { InteractiveObjectEx.Event_Hide, "bool" },
		UnHide = { InteractiveObjectEx.Event_UnHide, "bool" },
		Enable = { InteractiveObjectEx.Event_EnableUsable, "bool" },
		Disable = { InteractiveObjectEx.Event_DisableUsable, "bool" },
		Reset = { InteractiveObjectEx.Event_Reset, "bool" },
	},
	Outputs =
	{
		Started = "bool",
		Finished = "bool",
		Aborted = "bool",
		Enabled = "bool",
		Disabled = "bool",
		Hide = "bool",
		UnHide = "bool",
		Reset = "bool",
		UsedBy = "entity"
	},
}

MakeUsable(InteractiveObjectEx);

function InteractiveObjectEx:IsUsable(user)

	if self.bUsable ~= 0 then
		 -- returns 0 if cannot use, otherwise returns action index + 1
		 return self.interactiveObject:CanUse(user.id);
	else
		return 0; 
	end
	
end

function InteractiveObjectEx:OnUsed(user, idx)
	self.interactiveObject:Use(user.id);
	self:ActivateOutput( "UsedBy", user.id );	
	self:ActivateOutput( "Started", true );
	BroadcastEvent(self, "Used");
end;

function InteractiveObjectEx:StopUse(user, idx)
	self.interactiveObject:StopUse(user);	
	self:ActivateOutput( "Finished", true );
end;

function InteractiveObjectEx:AbortUse(user)
	self.interactiveObject:AbortUse();	
	self:ActivateOutput( "Aborted", true );
end;
