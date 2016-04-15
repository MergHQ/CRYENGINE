--------------------------------------------------------------------------
--	Crytek Source File.
-- 	Copyright (C), Crytek Studios, 2001-2007.
--------------------------------------------------------------------------
--	$Id$
--	$DateTime$
--	Description: Places decals around the world
--
--------------------------------------------------------------------------
--  History:
--  - 1/8/2007     : Created by Márcio Martins
--
--------------------------------------------------------------------------

DecalPlacer =
{
	Properties =
	{
		sFileName = "",
		bMaterial = 1,
		sNormalAxis = "z",
		fSnapDistance = 0,
		fSize = 1,
		fAngle = 0,
		nLifeTime = 180,
		bPlace = 0,
	},

	Editor =
	{
		Icon = "Decal.bmp",
	},
}

function DecalPlacer:OnPropertyChange()
	self:OnReset();
end;

function DecalPlacer:OnReset()
	if (tonumber(self.Properties.bPlace)~=0) then
		self:Event_SpawnDecal();
	end
end;

----------------------------------------------------------------------------------------------------
function DecalPlacer:OnInit()
	self.hit={};
	self:OnReset();
end;

----------------------------------------------------------------------------------------------------
function DecalPlacer:Event_SpawnDecal()
	local file=self.Properties.sFileName;
	local size=self.Properties.fSize;
	local angle=self.Properties.fAngle;
	local life=self.Properties.nLifeTime;
	local mat=tonumber(self.Properties.bMaterial)~=0;
	local axis=string.lower(self.Properties.sNormalAxis);
	local neg=(axis=="-x") or (axis=="-y") or (axis=="-z");
	local snap=self.Properties.fSnapDistance;

	local normal;
	local pos;
	local dir;

	if (string.find(axis, "x")) then
		dir=self:GetDirectionVector(0, g_Vectors.temp_v1);
	elseif (string.find(axis, "y")) then
		dir=self:GetDirectionVector(1, g_Vectors.temp_v1);
	else
		dir=self:GetDirectionVector(2, g_Vectors.temp_v1);
	end

	if (neg) then
		dir.x=-dir.x;
		dir.y=-dir.y;
		dir.z=-dir.z;
	end

	local spawned=false;
	if (snap>0) then
		local raydir=g_Vectors.temp_v3;
		raydir.x=-dir.x*snap;
		raydir.y=-dir.y*snap;
		raydir.z=-dir.z*snap;

		local hittbl=self.hit;
		local hits=Physics.RayWorldIntersection(self:GetWorldPos(g_Vectors.temp_v2), raydir, 1, ent_all, nil, nil, hittbl);

		if (hits>0) then
			local hit=hittbl[1];

			if (mat) then
				Particle.CreateMatDecal(hit.pos, hit.normal, size, life, file, angle, dir, (hit.entity and hit.entity.id), hit.renderNode);
			else
				Particle.CreateDecal(hit.pos, hit.normal, size, life, file, angle);
			end

			spawned=true;
		end
	end

	if (not spawned) then
		normal=dir;
		pos=self:GetWorldPos(g_Vectors.temp_v2);

		if (mat) then
			Particle.CreateMatDecal(pos, normal, size, life, file, angle);
		else
			Particle.CreateDecal(pos, normal, size, life, file, angle);
		end
	end

	BroadcastEvent(self, "SpawnDecal");
end;

function DecalPlacer:Event_Filename( sender, sFileName )
	self.Properties.sFileName = sFileName;
	self:ActivateOutput("FileName", sFileName );
end;

DecalPlacer.FlowEvents =
{
	Inputs =
	{
		SpawnDecal = { DecalPlacer.Event_SpawnDecal, "bool" },
		FileName = { DecalPlacer.Event_Filename, "string" },
	},
	Outputs =
	{
		SpawnDecal = "bool",
		FileName = "string",
	},
}