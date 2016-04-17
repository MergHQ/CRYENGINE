AmmoCrate = {
	type = "AmmoCrate",
	
	Properties = {
		--object_BoxModel = "objects/weapons/ammo/ammo_box/ammo_box.cgf",
		object_BoxModel = "objects/weapons/ammo/ammo_crate/ammo_crate_small_open.cgf",
		object_Ammo4Recharges = "objects/weapons/ammo/ammo_crate/magazine_rifle_x4.cgf",
		object_Ammo3Recharges = "objects/weapons/ammo/ammo_crate/magazine_rifle_x3.cgf",
		object_Ammo2Recharges = "objects/weapons/ammo/ammo_crate/magazine_rifle_x2.cgf",
		object_Ammo1Recharges = "objects/weapons/ammo/ammo_crate/magazine_rifle_x1.cgf",
		object_Ammo0Recharges = "objects/weapons/ammo/ammo_crate/ammo_crate_small_open.cgf",
		numberOfRecharges = 0,
		
		Ammo = 
		{
			AmmoCategory = "Regular",
			FragGrenades = 2,
		},
	},
	
	Editor={
		Icon="item.bmp",
		IconOnTop=1,
	},
}


function AmmoCrate:OnInit()
	self.ammoRechargesModels = {};
	self:OnReset();
	Game.AddTacticalEntity(self.id, eTacticalEntity_Ammo);
	
	local givesFragGrenades = self.Properties.Ammo.FragGrenades > 0;
	Game.OnAmmoCrateSpawned(givesFragGrenades); 
end

function AmmoCrate:OnPropertyChange()
	self:OnReset();
end

function AmmoCrate:OnReset()
	self:ResetMainBoxModel();
	self:ResetAmmoAttachmentModels();
	
	self.remaingRecharges = math.min(4 , math.max(0 , self.Properties.numberOfRecharges));
	self.infiniteRecharges = (self.remaingRecharges == 0);
	self.bUsable = self.Properties.bUsable;
	
	self:SetUpAmmoAttachmentsVisibility();
end

function AmmoCrate:ResetMainBoxModel()
	if (self.currentModel ~= self.Properties.object_BoxModel and not EmptyString(self.Properties.object_BoxModel)) then
		self:LoadObject( 0,self.Properties.object_BoxModel );
		self:Physicalize(0,PE_STATIC,{mass = 0, density = 0});
	end
	self.currentModel = self.Properties.object_BoxModel;
end

function AmmoCrate:ResetAmmoAttachmentModels()
	if (self.ammoRechargesModels[1] ~= self.Properties.object_Ammo0Recharges and not EmptyString(self.Properties.object_Ammo0Recharges)) then
		self:LoadObject(1, self.Properties.object_Ammo0Recharges );
	end
	self:DrawSlot(1, 0);
	self.ammoRechargesModels[1] = self.Properties.object_Ammo0Recharges;
	
	if (self.ammoRechargesModels[2] ~= self.Properties.object_Ammo1Recharges and not EmptyString(self.Properties.object_Ammo1Recharges)) then
		self:LoadObject(2, self.Properties.object_Ammo1Recharges );
	end
	self:DrawSlot(2, 0);
	self.ammoRechargesModels[2] = self.Properties.object_Ammo1Recharges;
	
	if (self.ammoRechargesModels[3] ~= self.Properties.object_Ammo2Recharges and not EmptyString(self.Properties.object_Ammo2Recharges)) then
		self:LoadObject(3, self.Properties.object_Ammo2Recharges );
	end
	self:DrawSlot(3, 0);
	self.ammoRechargesModels[3] = self.Properties.object_Ammo2Recharges;
	
	if (self.ammoRechargesModels[4] ~= self.Properties.object_Ammo3Recharges and not EmptyString(self.Properties.object_Ammo3Recharges)) then
		self:LoadObject(4, self.Properties.object_Ammo3Recharges );
	end
	self:DrawSlot(4, 0);
	self.ammoRechargesModels[4] = self.Properties.object_Ammo3Recharges;
	
	if (self.ammoRechargesModels[5] ~= self.Properties.object_Ammo4Recharges and not EmptyString(self.Properties.object_Ammo4Recharges)) then
		self:LoadObject(5, self.Properties.object_Ammo4Recharges );
	end
	self:DrawSlot(5, 0);
	self.ammoRechargesModels[5] = self.Properties.object_Ammo4Recharges;
end

function AmmoCrate:SetUpAmmoAttachmentsVisibility()
	if (self.infiniteRecharges) then
		self:DrawSlot(5, 1);
	else
		self:DrawSlot(self.remaingRecharges + 1, 1);
	end
end

function AmmoCrate:OnSave(tbl)
	tbl.bUsable = self.bUsable;
	tbl.infiniteRecharges = self.infiniteRecharges
	tbl.remaingRecharges = self.remaingRecharges;
end

function AmmoCrate:OnLoad(tbl)
	self.bUsable = tbl.bUsable;
	self.infiniteRecharges = tbl.infiniteRecharges
	self.remaingRecharges = tbl.remaingRecharges;
	self:ResetAmmoAttachmentModels();
	self:SetUpAmmoAttachmentsVisibility();
end

function AmmoCrate:OnShutDown()
	Game.RemoveTacticalEntity(self.id, eTacticalEntity_Ammo);
end


function AmmoCrate:HasRecharges()
	return (self.infiniteRecharges or (self.remaingRecharges > 0));
end


function AmmoCrate:DecreaseRecharges()
	if (not self.infiniteRecharges) then
		self:DrawSlot(self.remaingRecharges + 1, 0);
		self.remaingRecharges = self.remaingRecharges - 1;
		self:DrawSlot(self.remaingRecharges + 1, 1);
		if ( not self:HasRecharges() ) then
			Game.RemoveTacticalEntity(self.id, eTacticalEntity_Ammo);
		end
	end
end



function AmmoCrate:Event_Hide()
	Game.RemoveTacticalEntity(self.id, eTacticalEntity_Ammo);

	self:Hide(1);
	self:ActivateOutput("Hide",true );
end;

function AmmoCrate:Event_UnHide()
	self:Hide(0);
	self:ActivateOutput("UnHide",true );
	
	Game.AddTacticalEntity(self.id, eTacticalEntity_Ammo);
end;


MakeUsable(AmmoCrate);


AmmoCrate.FlowEvents =
{
	Inputs =
	{
		Hide = { AmmoCrate.Event_Hide, "bool" },
		UnHide = { AmmoCrate.Event_UnHide, "bool" },
		Used = { AmmoCrate.Event_Used, "bool" },
		EnableUsable = { AmmoCrate.Event_EnableUsable, "bool" },
		DisableUsable = { AmmoCrate.Event_DisableUsable, "bool" },
	},
	Outputs =
	{
		Hide = "bool",
		UnHide = "bool",
		Used = "bool",
		EnableUsable = "bool",
		DisableUsable = "bool",
	},
}


function AmmoCrate:StopUse(user, idx)
end;

function AmmoCrate:OnUsed(user, idx)
	local ammoRefilled = user.actor:SvRefillAllAmmo(self.Properties.Ammo.AmmoCategory, false, self.Properties.Ammo.FragGrenades, false);
	if (ammoRefilled ~= 0) then
		self:DecreaseRecharges();
	end

	user.actor:ClRefillAmmoResult(ammoRefilled);
	self:ActivateOutput("Used",true );
end;

function AmmoCrate:IsUsable(user)
	local canUserUse = user.actor:IsLocalClient() and not user.actor:IsCurrentItemHeavy();
	local isUsable = self.bUsable and canUserUse and self:HasRecharges();

	if (isUsable) then
		return 1;
	end
	
	return 0;
end
