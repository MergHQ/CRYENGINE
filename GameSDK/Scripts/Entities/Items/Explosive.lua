function CreateExplosive(name)
	if (not _G[name]) then
		local explosive={};
		
		function explosive:CanDisarm(playerId)
			return true;
		end

		_G[name]=explosive;
	end
end


CreateExplosive("c4explosive");
