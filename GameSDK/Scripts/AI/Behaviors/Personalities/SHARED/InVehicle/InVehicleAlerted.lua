local Behavior = CreateAIBehavior("InVehicleAlerted", "InVehicle",	{	Alertness = 2,	Exclusive = 1,

	Destructor = function( self, entity )	
		entity:SelectPipe(0,"do_nothing");
		entity:InsertSubpipe(AIGOALPIPE_NOTDUPLICATE,"clear_all");
	end,
})