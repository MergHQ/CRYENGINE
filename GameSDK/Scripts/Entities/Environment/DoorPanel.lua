DoorPanel =
{
	type = "DoorPanel",

	Properties =
	{
		objModel = "Objects/props/cell_panel/cell_panel.cgf",
		objModelDestroyed = "Objects/props/cell_panel/cell_panel_broken.cgf",
		bDestroyable = 1,
		DestroyedExplosionType = "SmartMineExplosive",
		esDoorPanelState="Idle",
		bUsableOnlyOnce = 0,
		VisibleFlashDistance = 50,
		ScanSuccessTimeDelay = 0.5,
		ScanFailureTimeDelay = 1.5,

		TacticalInfo =
		{
			bScannable = 0,
			LookupName = "",
		},
	},

	Editor =
	{
		Icon="mine.bmp",
	},

	Client = {},
	Server = {},
}

MakeUsable( DoorPanel );

function DoorPanel:OnSave(tbl)
	tbl.bUsable=self.bUsable;
end;

function DoorPanel:OnLoad(tbl)
	self:OnReset();
	self.bUsable=tbl.bUsable;
end;

function DoorPanel.Client:OnInit()
	self:OnReset();
end;

function DoorPanel:OnReset()
	self.bUsable = self.Properties.bUsable;
end;

function DoorPanel:OnUsed( user, idx )
end

function DoorPanel:IsUsable(user)
	if(self.bUsable==1)then
		return 2;
	end
		
	return 0;	
end;

function DoorPanel:OnPropertyChange()

end

function DoorPanel.Server:OnHit(hit)
	if (self.Properties.bDestroyable==1) then
		if ((hit.type ~= "collision") and (hit.damage > 0)) then
			Game.SendEventToGameObject( self.id, "Destroyed" );
		end
	end
end

function DoorPanel:OnStateChange(stateName)
	self:ActivateOutput( stateName, true );
end;

-- Flownode related

function DoorPanel:Event_EnableUsable()
	self.bUsable=1;
end;

function DoorPanel:Event_DisableUsable()
	self.bUsable=0;
end;

function DoorPanel:Event_Idle()
	Game.SendEventToGameObject( self.id, "Idle" );
end;

function DoorPanel:Event_Scanning()
	self:OnCustomActionStart();
end;

function DoorPanel:Event_ScanSuccess()
	self:OnCustomActionSucceed();
end;

function DoorPanel:Event_ScanComplete()
	if (self.Properties.bUsableOnlyOnce==1) then
		self.bUsable = 0;
	end
	Game.SendEventToGameObject( self.id, "ScanComplete" );
end;

function DoorPanel:Event_ScanFailure()
	Game.SendEventToGameObject( self.id, "ScanFailure" );
end;

function DoorPanel:Event_Destroyed()
	if (self.Properties.bDestroyable==1) then
		Game.SendEventToGameObject( self.id, "Destroyed" );
	end
end;

function DoorPanel:Event_Hide()
	self:Hide(1);
end;

function DoorPanel:Event_Unhide()
	self:Hide(0)
end;

DoorPanel.FlowEvents =
{
	Inputs =
	{
		Idle = { DoorPanel.Event_Idle, "bool" },
		Scanning = { DoorPanel.Event_Scanning, "bool" },
		ScanSuccess = { DoorPanel.Event_ScanSuccess, "bool" },
		ScanComplete = { DoorPanel.Event_ScanComplete, "bool" },
		ScanFailure = { DoorPanel.Event_ScanFailure, "bool" },
		Destroyed = { DoorPanel.Event_Destroyed, "bool" },

		Hide = { DoorPanel.Event_Hide, "bool" },
		Unhide = { DoorPanel.Event_Unhide, "bool" },
	},
	Outputs =
	{
		Idle = "bool",
		Scanning = "bool",
		ScanSuccess = "bool",
		ScanComplete = "bool",
		ScanFailure = "bool",
		Destroyed = "bool",
	},
}