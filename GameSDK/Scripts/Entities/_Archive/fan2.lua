Fan = {
	Properties = {
	},
}

-------------------------------------------------------
function Fan:OnInit()
	self:SetName( "Fan" );
	self:LoadCharacter("Objects/Fan/Fan.cid", 0 );
	self:DrawCharacter( 0, 1 );
	self:StartAnimation(0,"FanLoop");	
end

-------------------------------------------------------
function Fan:OnContact( player )
end

-------------------------------------------------------
function Fan:OnUpdate(dt)
	
end

-------------------------------------------------------
function Fan:OnShutDown()
end

-------------------------------------------------------
function Fan:OnActivate()

end