----------------------------------------------------------------------------
----------------------------------------------------------------------------
-- ZeroG common code
----------------------------------------------------------------------------
----------------------------------------------------------------------------

GravityGlobals = {
	gravityNodes={},
}

function GravityGlobals:RemoveGravityArea(node)

	if (not node) then
		return;
	end
	
	local asize = table.getn(self.gravityNodes);
	
	for i=1,asize do
	--for i,node in pairs(self.gravityNodes) do
		
		--if (self.gravityNodes[i]==node) then
		if (self.gravityNodes[i] and self.gravityNodes[i].id == node.id) then	
			
			if (node.GetName) then
				
				System.Log(node:GetName().."removed from the gravity array!");
			end
			
			self.gravityNodes[i] = nil;
		
			return;
		end
	end
end

function GravityGlobals:AddGravityArea(node)

	if (not node) then
		return;
	end

	local asize = table.getn(self.gravityNodes);
		
	--check if the node is already in
	for i=1,asize do
	
		--if (self.gravityNodes[i]==node) then		
		if (self.gravityNodes[i] and self.gravityNodes[i].id == node.id) then	
				
--			System.Log(node:GetName().." already inside the array!");
			return;	
		end
	end	
	
	--now search for a free spot inside the array
	for i=1,asize do
			
		if (self.gravityNodes[i]==nil) then
			
--			System.Log("found empty slot! "..node:GetName().." added to the array ("..i..")");
			self.gravityNodes[i] = node;
			return;
		end
	end	
		
	--no free spots, add a new element to the array
--	System.Log(node:GetName().." added to the array! (new array size:"..(asize+1)..")");
	self.gravityNodes[asize+1]=node;
end

function GravityGlobals:GetPosGravity(pos)

	local asize = table.getn(self.gravityNodes);
	local node = nil;
	local candidateGravity = nil;
	
	for i=1,asize do
	--for i,node in pairs(self.gravityNodes) do
		node = self.gravityNodes[i];
		
		if (node and node.GetPos and node.GetPosGravity) then
		
			local gravity = node:GetPosGravity(pos);
			
			--unfortunately its not possible to return immediately the first gravity we found because there could be
			--different gravity areas in the same place, and its necessary to get the strongest one.
			if (gravity and (not candidateGravity or gravity > candidateGravity)) then 
				
				candidateGravity = gravity;
				
				--thats should be to make the code faster, ignore for now
				--if (candidateGravity > -0.1) then
				--	return candidateGravity;
				--end
				
				--return gravity; 
			end
			
		elseif (node) then
			
--			System.Log("deleting a gravity node from list!");
			self.gravityNodes[i] = nil;
		end
	end
	
	return candidateGravity;
	--return nil;
end

function EntityUpdateGravity(ent)

	if (GravityGlobals) then
		
		local UpImpulse = 0.0;
		local pos = g_Vectors.temp_v1;
		CopyVector(pos,ent:GetPos());
		
		local newGravity = nil;
		local tempg = GravityGlobals:GetPosGravity(pos);
						
		if (tempg) then
			
			if (ent.SaveLastGravity==nil) then
				
				local physicalStats = ent:GetPhysicalStats();
				
				ent:AwakePhysics(1);
				
				ent.SaveLastGravity = physicalStats.gravity;
				
				--first time entity enters a gravity sphere, add an upward impulse
				if (math.abs(tempg)<9.81) then
					UpImpulse = physicalStats.mass;
					
					if (UpImpulse) then
						UpImpulse = UpImpulse * 1.0;
					else
						UpImpulse = 0.0;
					end
				end
			end
						
			--System.Log(ent:GetName().." "..tempg);
			
			if (ent.TempPhysicsParams==nil) then
				ent.TempPhysicsParams = 
				{
					gravityz = -9.81,
					freefall_gravityz = -9.81,
					lying_gravityz = -9.81,
					zeroG = 0,
					air_control = 0,
				};
			end
			
			newGravity = tempg;
		else
			
			if (ent.SaveLastGravity) then
				
				newGravity = ent.SaveLastGravity;	
				ent.SaveLastGravity = nil;
				
--				System.Log("restored to "..newGravity);
			end
		end
		
		if (newGravity) then
			
			--make gravity proportional to the original entity gravity. Using 9.81 as default gravity.
			if (ent.SaveLastGravity) then
				newGravity = newGravity * (ent.SaveLastGravity/-9.81);
			end
			
			local pparams = ent.TempPhysicsParams;
				
			pparams.gravityz = newGravity;
			pparams.freefall_gravityz = newGravity;
			pparams.lying_gravityz = newGravity;
			
			if (ent.type == "Player" and ent.isDedbody~=1) then
				
				pparams.gravity = -newGravity;
				
--				if (math.abs(newGravity)<9.81) then
--					--TODO:make the zerog flag a range between 0 and 1, 1 max thruster power - 0 no thruster power.
--					pparams.zeroG = 1;
--					pparams.air_control = 0;
--				else
--					pparams.zeroG = 0;
--					pparams.air_control = BasicPlayer.DynProp.air_control;
--				end
--				
--				
--				ent.cnt:SetDynamicsProperties( pparams );

				ent:SetPhysicParams( PHYSICPARAM_PLAYERDYN, pparams );		
			elseif (ent.IsProjectile) then
				
				if (not pparams.gravity) then
					pparams.gravity = {x=0,y=0,z=newGravity};
				else
					pparams.gravity.z = newGravity;
				end
				
				ent:SetPhysicParams( PHYSICPARAM_PARTICLE, pparams );
			else
				ent:SetPhysicParams(PHYSICPARAM_SIMULATION, pparams );
			end
		end
		
		if (UpImpulse>0.0) then
										
			--local vup = g_Vectors.temp_v2;
			--CopyVector(vup,ent:GetDirectionVector(2));	
			--ent:AddImpulse( -1, pos, vup, UpImpulse );
			ent:AddImpulse( -1, pos, g_Vectors.v001, UpImpulse );
			
--			System.Log("adding a "..UpImpulse.." impulse to "..ent:GetName());
		end
	end		
end
