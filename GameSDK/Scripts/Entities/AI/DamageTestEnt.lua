Script.ReloadScript( "Scripts/Entities/actor/player.lua");
-----------------------------------------------------------------------------------------------------

DamageTestEnt = new(Player);

DamageTestEnt.totalDamage = 0.0;
DamageTestEnt.lastHitTime = 0.0;
DamageTestEnt.firstHitTime = 0.0;
DamageTestEnt.numHits = 0;

DamageTestEnt.isActive = false;
DamageTestEnt.watchText = "";

DamageTestEnt.watchTextDead = "";
DamageTestEnt.isDead = false;

DamageTestEnt.Properties.fileHitDeathReactionsParamsDataFile = "Libs/HitDeathReactionsData/HitDeathReactions_MP.xml";
DamageTestEnt.Properties.Damage.fileBodyDamage = "Libs/BodyDamage/BodyDamage_MP.xml";
DamageTestEnt.Properties.Damage.fileBodyDamageParts = "Libs/BodyDamage/BodyParts_HumanMaleShared.xml";
DamageTestEnt.Properties.Damage.fileBodyDestructibility = "Libs/BodyDamage/BodyDestructibility_Default.xml";

function DamageTestEnt.Server:OnHit(hit)
	local damage = hit.damage;

	local targetPos = hit.target:GetPos();
	local shooterPos = hit.shooter:GetPos();

	local distance = DistanceVectors(targetPos, shooterPos);

	local dps = 0.0;
	local timeInSeconds = 0.0;

	local currentTime = g_gameRules.game:GetServerTime();
	if (self.isActive == false) then
		self.totalDamage = damage;
		self.firstHitTime = currentTime;
		self.lastHitTime = currentTime;
		self.isActive = true;
		self.numHits = 1;
	else
		self.totalDamage = self.totalDamage + damage;
		self.lastHitTime = currentTime;

		timeInSeconds = (self.lastHitTime - self.firstHitTime) * 0.001;

		dps = self.totalDamage / timeInSeconds;
		self.numHits = self.numHits + 1;
	end

	local partName = hit.partId;
	local extraHitInfo = self.actor:GetExtraHitLocationInfo(0, hit.partId);
	if (extraHitInfo) then
		if (extraHitInfo.boneName) then
			partName = extraHitInfo.boneName;
		elseif (extraHitInfo.attachmentName) then
			partName = extraHitInfo.attachmentName;
		end
	end

	local weaponName;
	if (hit.weapon) then
		weaponName = hit.weapon.class;
	else
		weaponName = EntityName(hit.weaponId) .. " (" .. hit.type .. ")";
	end

	self.watchText = EntityName(self) .. " (" .. partName .. ") : " .. weaponName .. ": " .. damage .. " damage at " .. distance .. "m away, (total=" .. self.totalDamage .. ", time=" .. timeInSeconds .. ", dps=" .. dps .. ", numHits=" .. self.numHits .. ")";
	Log(self.watchText);

	if (self.isDead == false) then
		if (self.totalDamage > 120) then		-- Unfortunately we can't check what a players health should be since the IsMultiplayer check will fail (player initialised before flag is set)
			self.watchTextDead = "Dead at time=" .. timeInSeconds .. ", numHits=" .. self.numHits;
			self.isDead = true;
			Log(self.watchTextDead);
		end
	end
end

function DamageTestEnt.Server:OnUpdate(frameTime)
	self.actor:PostPhysicalize();	-- HACK to make the entity actually stand up!

	if (self.isActive == true) then
		if (g_gameRules.game:GetServerTime() - self.lastHitTime < 10000.0) then
			g_gameRules.game:Watch(self.watchText);
			if (self.isDead == true) then
				g_gameRules.game:Watch(self.watchTextDead);
			end
		else
			self.isActive = false;
			self.isDead = false;
		end
	end
end

CreateActor(DamageTestEnt);
DamageTestEnt:Expose();

