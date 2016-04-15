VicinityDependentObjectMover =
{
	type = "VicinityDependentObjectMover",

	Properties =
	{
		objModel = "Objects/architecture/00_tutorial/columns/nanogon.cgf",
		fMoveToDistance = 2.0,
		fMoveToSpeed = 30.0,
		fMoveBackSpeed = 30.0,
		fAreaTriggerRange = 10.0,
		fBackAreaTriggerRange = 10.0,
		fForceMoveCompleteDistance = 2.0,
		bUseAreaTrigger = 0,
		bDisableAreaTriggerOnMoveComplete = 0,
	},

	Editor =
	{
		Icon="mine.bmp",
	},

	Client = {},
	Server = {},
}

function VicinityDependentObjectMover:OnPropertyChange()
	Game.SendEventToGameObject( self.id, "UpdateFromProperties" );
end

-- Flownode related

function VicinityDependentObjectMover:Event_EnableAreaTrigger()
	Game.SendEventToGameObject( self.id, "EnableAreaTrigger" );
end;

function VicinityDependentObjectMover:Event_DisableAreaTrigger()
	Game.SendEventToGameObject( self.id, "DisableAreaTrigger" );
end;

function VicinityDependentObjectMover:Event_MoveTo()
	Game.SendEventToGameObject( self.id, "MoveTo" );
end;

function VicinityDependentObjectMover:Event_MoveBack()
	Game.SendEventToGameObject( self.id, "MoveBack" );
end;

function VicinityDependentObjectMover:Event_Reset()
	Game.SendEventToGameObject( self.id, "Reset" );
end;

function VicinityDependentObjectMover:Event_ForceToTargetPos()
	Game.SendEventToGameObject( self.id, "ForceToTargetPos" );
end;

function VicinityDependentObjectMover:Event_ForceReverseMoveToStartPos()
	Game.SendEventToGameObject( self.id, "ForceReverseMoveToStartPos" );
end;

function VicinityDependentObjectMover:Event_Hide()
	self:Hide(1);
end;

function VicinityDependentObjectMover:Event_Unhide()
	self:Hide(0)
end;

VicinityDependentObjectMover.FlowEvents =
{
	Inputs =
	{
		EnableAreaTrigger = { VicinityDependentObjectMover.Event_EnableAreaTrigger, "bool" },
		DisableAreaTrigger = { VicinityDependentObjectMover.Event_DisableAreaTrigger, "bool" },
		MoveTo = { VicinityDependentObjectMover.Event_MoveTo, "bool" },
		MoveBack = { VicinityDependentObjectMover.Event_MoveBack, "bool" },
		Reset = { VicinityDependentObjectMover.Event_Reset, "bool" },
		ForceToTargetPos = { VicinityDependentObjectMover.Event_ForceToTargetPos, "bool" },
		ForceReverseMoveToStartPos = { VicinityDependentObjectMover.Event_ForceReverseMoveToStartPos, "bool" },
		Hide = { VicinityDependentObjectMover.Event_Hide, "bool" },
		Unhide = { VicinityDependentObjectMover.Event_Unhide, "bool" },
	},
	Outputs =
	{
		MoveToStart = "bool",
		MoveToComplete = "bool",
		MoveBackStart = "bool",
		MoveBackComplete = "bool",
		OnReset = "bool",
		OnForceToTargetPos = "bool",
		OnHide = "bool",
		OnUnhide = "bool",
	},
}