-- This entity is designed to be used with projectile weapons such as Bow/Arrow where ammo can be retrieved by the player after being fired.
-- See also PickUpPickableAmmo scriptbinds for Flowgraph implementation.

PickableAmmo =
{
	type = "PickableAmmo",

	Client = {},
	Server = {},

	Properties =
	{
		bUsable = 1,
		bAutoPickable = 0,
		AmmoType = "Bullet",
		SoundSignal = "Player_GrabAmmo",
		UseMessage = "",
	},

	Editor =
	{
		Icon="item.bmp",
		IconOnTop=1,
	},

	bUsed = false,
	bInitialized = false,
	fAliveTime = 0.0,
	bDestroyOnUse = 0
}

MakeUsable(PickableAmmo);


function PickableAmmo.Server:OnInit()
	if(not self.bInitialized)then
		self:OnReset();
		self.bInitialized = true;
	end;
end;


function PickableAmmo.Client:OnInit()
	if(not self.bInitialized)then
		self:OnReset();
		self.bInitialized = true;
	end;
end;


function PickableAmmo:OnReset(user, idx)
	local physics =
	{
		bPhysicalize = 1,
		bPushableByPlayers = 0,
		Density = -1,
		Mass = -1,
	};
	EntityCommon.PhysicalizeRigid(self, 0, physics, 1);
	self.bUsed = false;
end;


function PickableAmmo:OnUsed(user, idx)
	self:PickAmmo(user);
end;


function PickableAmmo:IsUsable(user)
	bIsUsable = (self.Properties.bUsable==1) and not self.bUsed and not(self.Properties.bAutoPickable==1);
	bCanPickupAmmo = self:CanPickUpAmmo(user);
	if (bIsUsable and bCanPickupAmmo) then
		return 1;
	else
		return 0;
	end;
end


function PickableAmmo:CanPickUpAmmo(user)
	local capacity = user.inventory:GetAmmoCapacity(self.Properties.AmmoType);
	local currentCount = user.inventory:GetAmmoCount(self.Properties.AmmoType);
	return currentCount < capacity;
end;


function PickableAmmo:OnUnhidden()
	CryAction.ForceGameObjectUpdate(self.id, true);
end;


function PickableAmmo:PickAmmo(user)
	bCanPickupAmmo = self:CanPickUpAmmo(user);
	if (bCanPickupAmmo) then
		user.actor:PickUpPickableAmmo(self.Properties.AmmoType, 1);

		audio = GameAudio.GetSignal(self.Properties.SoundSignal);
		GameAudio.JustPlayEntitySignal(audio, user.id);
	end;

	self.bUsed = true;

	if (self.bDestroyOnUse == 1) then
		System.RemoveEntity(self.id);
	end;

	self:Activate(0);
end;