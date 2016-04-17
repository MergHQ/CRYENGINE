MineField =
{
	type = "MineField",

	Properties =
	{
		TacticalInfo =
		{
			bScannable = 1,
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

MakeUsable( MineField );

function MineField:OnSave(tbl)
	tbl.bUsable=self.bUsable;
end;

function MineField:OnLoad(tbl)
	self:OnReset();
	self.bUsable=tbl.bUsable;
end;

function MineField.Client:OnInit()
	self:OnReset();
end;

function MineField:OnReset()
	self.bUsable = self.Properties.bUsable;
end;

function MineField:OnUsed( user, idx )
end

function MineField:IsUsable(user)
	if(self.bUsable==1)then
		return 2;
	end
		
	return 0;	
end;

function MineField:OnPropertyChange()

end

-- Scanning callbacks

function MineField:HasBeenScanned()
	Game.SendEventToGameObject( self.id, "OnScanned" );
	self:ActivateOutput( "Scanned", true );
end

-- Flownode related

function MineField:Event_SetScanned()
	self:HasBeenScanned();
end;

function MineField:Event_Destroy()
	Game.SendEventToGameObject( self.id, "OnDestroy" );
	self:ActivateOutput( "Destroyed", true );
end;

MineField.FlowEvents =
{
	Inputs =
	{
		SetScanned = { MineField.Event_SetScanned, "bool" },
		SetDestroyed = { MineField.Event_Destroy, "bool" },
	},
	Outputs =
	{
		Scanned = "bool",
		Destroyed = "bool",
	},
}