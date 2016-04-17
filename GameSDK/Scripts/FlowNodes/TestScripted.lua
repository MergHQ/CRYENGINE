TestScripted = {
	Category = "wip",
	Inputs = {"input"},
	Outputs = {"output"},
}

function TestScripted:OnActivate_input()
	Log( "TestScripted got: "..tostring(self.input) )
	self:Activate_output( self.input )
end

