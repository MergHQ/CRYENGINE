HUD_Object = {
	type = "HUD_Object",
	Properties = {
		fileModel 					= "objects/hud/hud_bg_center.cgf"
	},
	Editor={
		Icon = "Comment.bmp",
		IconOnTop=1,
	},
}

function HUD_Object:OnInit()
	self:OnReset();
end

function HUD_Object:OnPropertyChange()
	self:OnReset();
end

function HUD_Object:OnReset()
	self:LoadObjectWithFlags(0,self.Properties.fileModel, 2);
end

function HUD_Object:SetObjectModel(model)
	self.Properties.fileModel = model;
	self:LoadObjectWithFlags(0,model, 2);
end

function HUD_Object:OnSave(tbl)
	tbl.fileModel = self.Properties.fileModel;
end

function HUD_Object:OnLoad(tbl)
	self:SetObjectModel(tbl.fileModel);
end
 
function HUD_Object:OnPostLoad()
	self:Hide(0);
end

function HUD_Object:OnShutDown()
end

function HUD_Object:Event_TargetReached( sender )
end

function HUD_Object:Event_Enable( sender )
end

function HUD_Object:Event_Disable( sender )
end

HUD_Object.FlowEvents =
{
	Inputs =
	{
	},
	Outputs =
	{
	},
}
