Script.ReloadScript("scripts/Utils/EntityUtils.lua")

RopeEntity =
{
	Properties=
	{
		MultiplayerOptions = {
			bNetworked		= 0,
		},
	},
	Attachment = {},
}

------------------------------------------------------------------------------------------------------
function RopeEntity:OnSpawn()
	if (self.Properties.MultiplayerOptions.bNetworked == 0) then
		self:SetFlags(ENTITY_FLAG_CLIENT_ONLY,0);
	end
end

------------------------------------------------------------------------------------------------------
function RopeEntity:OnPhysicsBreak( vPos,nPartId,nOtherPartId )
	self:ActivateOutput("Break",nPartId+1 );
end

------------------------------------------------------------------------------------------------------
function RopeEntity:Event_Remove()
	self:DrawSlot(0,0);
	self:DestroyPhysics();
	self:ActivateOutput( "Remove", true );
end

------------------------------------------------------------------------------------------------------
function RopeEntity:Event_Hide()
	self:Hide(1);
	self:ActivateOutput( "Hide", true );
end

------------------------------------------------------------------------------------------------------
function RopeEntity:Event_UnHide()
	self:Hide(0);
	self:ActivateOutput( "UnHide", true );
end

------------------------------------------------------------------------------------------------------
function RopeEntity:Event_BreakStart( vPos,nPartId,nOtherPartId )
	local RopeParams = {}
	RopeParams.entity_name_1 = "#unattached";

	self:SetPhysicParams(PHYSICPARAM_ROPE,RopeParams);
end
function RopeEntity:Event_BreakEnd( vPos,nPartId,nOtherPartId )
	local RopeParams = {}
	RopeParams.entity_name_2 = "#unattached";

	self:SetPhysicParams(PHYSICPARAM_ROPE,RopeParams);
end
function RopeEntity:Event_BreakDist( sender, dist )
	local RopeParams = {}
	RopeParams.break_point = dist;

	self:SetPhysicParams(PHYSICPARAM_ROPE,RopeParams);
end
function RopeEntity:Event_Disable()
	local RopeParams = {}
	RopeParams.bDisabled = 1;
	self:SetPhysicParams(PHYSICPARAM_ROPE,RopeParams);
end
function RopeEntity:Event_Enable()
	local RopeParams = {}
	RopeParams.bDisabled = 0;
	self:SetPhysicParams(PHYSICPARAM_ROPE,RopeParams);
end
function RopeEntity:Event_Length(sender, length)
	local RopeParams = {}
	RopeParams.length = length;
	self:SetPhysicParams(PHYSICPARAM_ROPE,RopeParams);
end
function RopeEntity:Event_PtStart( sender, pt )	self.Attachment.end1 = pt; end
function RopeEntity:Event_PtEnd( sender, pt ) self.Attachment.end2 = pt; end
function RopeEntity:Event_PhysIdStart( sender, id )	self.Attachment.entity_phys_id_1 = id; end
function RopeEntity:Event_PhysIdEnd( sender, id ) self.Attachment.entity_phys_id_2 = id; end
function RopeEntity:Event_SegCount( sender, nsegs )
	self:SetPhysicParams(PHYSICPARAM_ROPE, self.Attachment);
	local RopeParams = {}
	RopeParams.num_segs	= nsegs;
	self:SetPhysicParams(PHYSICPARAM_ROPE, RopeParams);
	local dpos = self:GetWorldPos(); NegVector(dpos);
	local p0 = SumVectors(self.Attachment.end1, dpos);
	local p1 = SumVectors(self.Attachment.end2, dpos);
	self:SetLocalBBox( { x=__min(p0.x,p1.x), y=__min(p0.y,p1.y), z=__min(p0.z,p1.z) }, { x=__max(p0.x,p1.x), y=__max(p0.y,p1.y), z=__max(p0.z,p1.z) }	);
end
function RopeEntity:Event_GetLength( sender, doget )
	local RopeParams = { length=-1 }
	self:SetPhysicParams(PHYSICPARAM_ROPE, RopeParams);
	self:ActivateOutput("Length", RopeParams.length);
	self:ActivateOutput("Strained", RopeParams.strained);
end
function RopeEntity:Event_MaxForce( sender, maxforce )
	local RopeParams = { max_force=maxforce }
	self:SetPhysicParams(PHYSICPARAM_ROPE, RopeParams);
end
function RopeEntity:Event_SleepSpeed( sender, sleepspeed )
	local SimParams = { sleep_speed=sleepspeed }
	self:SetPhysicParams(PHYSICPARAM_SIMULATION, SimParams);
end
function RopeEntity:Event_Radius( sender, radius )
	local RopeParams = { coll_dist=radius }
	self:SetPhysicParams(PHYSICPARAM_ROPE, RopeParams);
end


RopeEntity.FlowEvents =
{
	Inputs =
	{
		Hide = { RopeEntity.Event_Hide, "bool" },
		UnHide = { RopeEntity.Event_UnHide, "bool" },
		Remove = { RopeEntity.Event_Remove, "bool" },
		BreakStart = { RopeEntity.Event_BreakStart, "bool" },
    BreakEnd = { RopeEntity.Event_BreakEnd, "bool" },
		BreakDist = { RopeEntity.Event_BreakDist, "float" },
		Disable = { RopeEntity.Event_Disable, "bool" },
		Enable = { RopeEntity.Event_Enable, "bool" },
		Length = { RopeEntity.Event_Length, "float" },
		PtStart = { RopeEntity.Event_PtStart, "Vec3" },
		PhysIdStart = { RopeEntity.Event_PhysIdStart, "int" },
		PtEnd = { RopeEntity.Event_PtEnd, "Vec3" },
		PhysIdEnd = { RopeEntity.Event_PhysIdEnd, "int" },
		SegCount = { RopeEntity.Event_SegCount, "int" },
		GetLength = { RopeEntity.Event_GetLength, "bool" },
		MaxForce = { RopeEntity.Event_MaxForce, "float" },
		SleepSpeed = { RopeEntity.Event_SleepSpeed, "float" },
		Radius = { RopeEntity.Event_Radius, "float" },
	},
	Outputs =
	{
		Hide = "bool",
		UnHide = "bool",
		Remove = "bool",
		Break = "int",
		Length = "float",
		Strained = "bool",
	},
}

