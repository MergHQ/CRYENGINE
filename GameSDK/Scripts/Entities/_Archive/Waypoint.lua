Waypoint = {

	type = "Trigger",
	triggered = nil,
	time = 0,
	closed = nil,
	nTextureId = -1,
		
	Properties = {
		Radius = 1.5,
		TextTrigger = "Waypoint*TxtTrg",
		TextInstruction	= "Waypoint*TxtInst",
		NeededItem = "",
		RepeatTime = 5,
		Objective = "Waypoint*Reached",
		NextPoint = "Waypoint*",
		ActiveByDefault = 1,
		LabelSize = 2,
		LabelText = "Waypoint*LblTxt",
		LabelImage = "Textures/Hud/Icons/Waypoint.tga",
		CutsceneName = "Cutscenes/",
		TextTriggerTimeout=15,
		Map = "0",
	},
	DialogId=-1,
}

function Waypoint:OnPropertyChange()
	self:OnReset();
end

function Waypoint:OnReset()
	self:SetRadius(self.Properties.Radius);	
end

function Waypoint:OnInit()
	self:OnReset();	
	self:LoadObject("Objects/Editor/HUD/W.cgf",0,1);
	self:DrawObject( 0, 1 );	
end

function Waypoint:OnContact( player )

	if (self.closed ~= nil) then do return end; end

	if (_localplayer ~= player) then do return end; end
	
	if (self.triggered == nil)
	then
		self.triggered = 1;
		self.time = 0;

		if (self.Properties.NeededItem ~= "")
		then
		
			if (player.MyInventory.HasItem(player.MyInventory,self.Properties.NeededItem))
			then
				self.DialogId=Game:ShowIngameDialog(-1, "", "", 12, Language[self.Properties.TextTrigger]);
				Game:SetTimer(self, self.Properties.TextTriggerTimeout*1000);
				self.closed = 1;
				self:DeActivate();

				-- if need to activate another waypoint do so now
				--local other;
				--other = System.GetEntityByName(self.Properties.NextPoint);
				--if (other~=nil)
				--then
--					System.LogToConsole("Activating "..self.Properties.NextPoint);
				--	other.Activate(other);
				--end
				--self.OnEvent( ScriptEvent_Activate );
				self.OnEvent( self,ScriptEvent_DropItem );
				--StdBroadcastEvent(self,ScriptEvent_DropItem );
				
				System.LogToConsole("Objective= "..self.Properties.Objective);
				
				-- Mission table global has to be defined
				if (Mission[self.Properties.Objective] ~= nil)
				then
					Mission[self.Properties.Objective] = 1;
					Mission.OnUpdate(Mission);
					System.LogToConsole("mission objective set OK!!");
				else
					System.LogToConsole("Tried to set to true unexistant mission objective");
				end
			else
				System.LogToConsole(Language[self.Properties.TextInstruction]);
			end
		else
			self.DialogId=Game:ShowIngameDialog(-1, "", "", 12, Language[self.Properties.TextTrigger]);
			Game:SetTimer(self, self.Properties.TextTriggerTimeout*1000);
			self.closed = 1;
			self:DeActivate();

			-- if need to activate another waypoint do so now
			local other;
			other = System.GetEntityByName(self.Properties.NextPoint);
			if (other~=nil)
			then
				System.LogToConsole("Activating "..self.Properties.NextPoint);
				other.Activate(other);
			end
			
			System.LogToConsole("Objective= "..self.Properties.Objective);
	
			-- Mission table global has to be defined
			if (Mission[self.Properties.Objective] ~= nil)
			then
				Mission[self.Properties.Objective] = 1;
				Mission.OnUpdate(Mission);
				System.LogToConsole("mission objective set OK!!");
			else
				System.LogToConsole("Tried to set to true unexistant mission objective");
			end
		end

	end
	
end

function Waypoint:OnUpdate( dt )
	
--	System.LogToConsole(Language[self.Properties.TextInstruction]);

	if (self.closed ~= nil) then do return end; end

	local pos = self:GetPos();
	pos.z = pos.z + 2;
	Game:DrawLabel(pos,self.Properties.LabelSize,Language[self.Properties.LabelText]);
	if (self.nTextureId~=nil) then	
		System.DrawLabelImage(pos,self.Properties.LabelSize,self.nTextureId);
	end

local angles = self:GetAngles();
	angles.z = angles.z + 8.0;
	self:SetAngles(angles);

	if (self.triggered~=nil)
	then
		self.time = self.time + System.GetFrameTime();
		if (self.time > self.Properties.RepeatTime) then
			self.triggered = nil;
		end
	end
	
end

function Waypoint:OnEvent( eid, params)
 	if (eid == ScriptEvent_Reset) then
		self:SetRadius(self.Properties.Radius);
		self.nTextureId=System.LoadTexture(self.Properties.LabelImage);
		if (self.Properties.ActiveByDefault == 1)
		then
			self:Activate();
		else
			self:DeActivate();
		end
	elseif (eid == ScriptEvent_Activate) then
		self:Activate();
	elseif (eid == ScriptEvent_Timer) then
		Game:HideIngameDialog(self.DialogId);
	end
	--StdBroadcastEvent(self,params);
end

function Waypoint:OnShutDown()
end

function Waypoint:Activate()
	System.LogToConsole("Activating "..self:GetName());
	self:Activate(1);
	self.closed = nil;
	self:DrawObject(0,1);
end

function Waypoint:DeActivate()
	System.LogToConsole("DEActivating "..self:GetName());
	self:Activate(nil);
	self.closed = 1;
	self:DrawObject(0,0);
end

function Waypoint:OnSave(stm)
	stm:WriteBool(self.triggered);
	stm:WriteBool(self.closed);
--properties
	local prop=self.Properties;
	stm:WriteFloat(prop.Radius);
	stm:WriteString(prop.TextTrigger);
	stm:WriteString(prop.TextInstruction);
	stm:WriteString(prop.NeededItem);
	stm:WriteInt(prop.RepeatTime);
	stm:WriteString(prop.Objective);
	stm:WriteString(prop.NextPoint);
	stm:WriteInt(prop.ActiveByDefault);
	stm:WriteInt(prop.LabelSize);
	stm:WriteString(prop.LabelText);
	stm:WriteString(prop.LabelImage);
	stm:WriteString(prop.CutsceneName);
	stm:WriteInt(prop.TextTriggerTimeout);
	stm:WriteString(prop.Map);
end

function Waypoint:OnLoad(stm)
	self.triggered=stm:ReadBool();
	self.closed=stm:ReadBool();
	if(self.closed~=nil) then
		self:DeActivate();
	end
--properties
	local prop=self.Properties;
	prop.Radius=stm:ReadFloat();
	prop.TextTrigger=stm:ReadString();
	prop.TextInstruction=stm:ReadString();
	prop.NeededItem=stm:ReadString();
	prop.RepeatTime=stm:ReadInt();
	prop.Objective=stm:ReadString();
	prop.NextPoint=stm:ReadString();
	prop.ActiveByDefault=stm:ReadInt();
	prop.LabelSize=stm:ReadInt();
	prop.LabelText=stm:ReadString();
	prop.LabelImage=stm:ReadString();
	if(prop.LabelImage)then
		self.nTextureId=System.LoadImage(self.Properties.LabelImage);
	end
	prop.CutsceneName=stm:ReadString();
	prop.TextTriggerTimeout=stm:ReadInt();
	prop.Map=stm:ReadString();
end