Script.ReloadScript( "Scripts/Entities/Physics/RigidBodyEx.lua");

DangerousRigidBody = {
	Properties = {
		bCurrentlyDealingDamage = 0,
		bDoesFriendlyFireDamage = 0,
		fDamageToDeal = 10000.0,
		fTimeBetweenHits = 0.5,
	},
}

MakeDerivedEntity( DangerousRigidBody, RigidBodyEx )

-------------------------------------------------------
--					Flow Graph						 --
-------------------------------------------------------

function DangerousRigidBody:Event_MakeDangerous(sender, params)
	Game.SetDangerousRigidBodyDangerStatus(self.id, true, params.id);
	self:ActivateOutput( "IsDangerous", true );
end

function DangerousRigidBody:Event_MakeNotDangerous()
	Game.SetDangerousRigidBodyDangerStatus(self.id, false, 0);
	self:ActivateOutput( "IsDangerous", false );
end

DangerousRigidBody.FlowEvents = 
{
	Inputs = 
	{
		MakeDangerous = { DangerousRigidBody.Event_MakeDangerous, "entity" },
		MakeNotDangerous = { DangerousRigidBody.Event_MakeNotDangerous, "any" },
	},
	Outputs = 
	{
		IsDangerous = "bool"
	},
}

mergef(DangerousRigidBody.FlowEvents, RigidBodyEx.FlowEvents, 1);