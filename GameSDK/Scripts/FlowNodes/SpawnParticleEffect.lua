--------------------------------------------------------------------------
--	Crytek Source File.
-- 	Copyright (C), Crytek Studios, 2001-2005.
--------------------------------------------------------------------------
--	$Id$
--	$DateTime$
--	Description: FlowGraph node which spawns a particle effect
--  
--------------------------------------------------------------------------
--  History:
--  - 30/09/2005   : Created by Sascha Gundlach
--
--------------------------------------------------------------------------

SpawnParticleEffect = {
	Category = "approved",
	Inputs = {{"t_Activate","bool"},{"Position","vec3"}, {"Effect","string"}},
	Outputs = {{"Done","bool"}},
	Implementation=function(bActivate,vPosition,sEffect)
		Particle.SpawnEffect(sEffect,vPosition,g_Vectors.v001,1.0);
		return;
	end;
} 


