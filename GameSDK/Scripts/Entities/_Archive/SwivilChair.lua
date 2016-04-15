-- chair for AI to sit on
-- after creating lua need to place path in Editor\EntityRegistry.xml (in Artwork SS)
SwivilChair = {
	name = "SwivilChair",
	Properties = {
		objModel = "Objects/Indoor/furniture/chairs/swivel.cgf",
	},
}
------------------------------------------------------------------------------------------------------
function SwivilChair:OnInit()
	self:Activate(0);
	if (self.ModelName ~= self.Properties.objModel) then
		self.ModelName = self.Properties.objModel;
		self:LoadObject( self.ModelName, 0,1 );
	end
	-- see ScriptObjectEntity 
	self:CreateStaticEntity(200,0); -- param fMass, nSurfaceID
	self:EnablePhysics( 1 );
	self:DrawObject(0,1); --param nPos(slot number), nMode(0 = Don't draw, 1 = Draw normally, 3 = Draw near)
--	self:RegisterWithAI(AIAnchor.AIOBJECT_SWIVIL_CHAIR);
	AI:RegisterWithAI(self.id, AIAnchor.AIOBJECT_SWIVIL_CHAIR);
end
------------------------------------------------------------------------------------------------------
function SwivilChair:OnEvent ( id, params)
	
end
------------------------------------------------------------------------------------------------------
function SwivilChair:OnShutDown()

end
------------------------------------------------------------------------------------------------------
