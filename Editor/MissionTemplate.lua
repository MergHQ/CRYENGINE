Mission = {
};


function Mission:OnInit()
	-- you may want to load a string-table etc. here...
end

function Mission:OnUpdate()
	local i;
	local bFinished=1;
	for i,objective in Mission do
		if (type(objective)~="function") then
			if (objective==0) then 
				bFinished=0;				
			else
			end
		end	
	end
	if (bFinished==1) then	
		self.Finish();
	end
end


function Mission:Finish()
	-- go to next mission...
end