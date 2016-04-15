----------------------------------------------------------------------------------------------------
--  Crytek Source File.
--  Copyright (C), Crytek Studios, 2012.
----------------------------------------------------------------------------------------------------
--  Description: Stub for data patch run once script for post-launch fixing etc.
--
----------------------------------------------------------------------------------------------------
--  History:
--  - 17:01:12 Created by Andrew Blackwell
--
----------------------------------------------------------------------------------------------------

RunOnce = 
{

}

function RunOnce:RunVersion1(inVersion)
	Log("RunOnce:RunVersion1()")

	if (inVersion == 0) then
		Log("RunOnce:RunVersion1() inVersion is < 1 actually running")

		currentTrackingVersion = 0 + 1
		Log("RunOnce:RunVersion1() writing RunOnceTrackingVersion of " .. tostring(currentTrackingVersion))
		setTable = { value = currentTrackingVersion } 
		success = ProtectedBinds.SetPersistantStat("EIPS_RunOnceTrackingVersion", setTable)

		inVersion = inVersion + 1
	end

	return inVersion
end

function RunOnce:RunVersion2(inVersion)
	Log("RunOnce:RunVersion2()")

	if (inVersion == 1) then 
		Log("RunOnce:RunVersion2() inVersion is < 2 actually running")

		currentTrackingVersion = tonumber(ProtectedBinds.GetPersistantStat("EIPS_RunOnceTrackingVersion"))

		if (currentTrackingVersion == nil) then
			Log("RunOnce:RunVersion2() failed to get currentTrackingVersion from profile. This should NOT happen. Version1 patch should have set it!")
			currentTrackingVersion=2000;
		end

		currentTrackingVersion = currentTrackingVersion + 2
		Log("RunOnce:RunVersion2() writing RunOnceTrackingVersion of " .. tostring(currentTrackingVersion))
		setTable = { value = currentTrackingVersion } 
		success = ProtectedBinds.SetPersistantStat("EIPS_RunOnceTrackingVersion", setTable)

		inVersion = inVersion + 1
	end

	return inVersion
end

function RunOnce:RunVersion3(inVersion)
	Log("RunOnce:RunVersion3()")

	if (inVersion == 2) then 
		Log("RunOnce:RunVersion3() inVersion is < 3 actually running")

		currentTrackingVersion = tonumber(ProtectedBinds.GetPersistantStat("EIPS_RunOnceTrackingVersion"))

		if (currentTrackingVersion == nil) then
			Log("RunOnce:RunVersion3() failed to get RunOnceTrackingVersion from profile. This should NOT happen. Version1 patch should have set it!")
			currentTrackingVersion=3000
		end

		if (currentTrackingVersion == 3) then
			Log("RunOnce:RunVersion3() everything is fine, currentTrackingVersion is 3 as expected")
		else
			Log("RunOnce:RunVersion3() its all gone wrong. currentTrackingVersion is " .. tostring(currentTrackingVersion) .. " instead of 3 as expected")
		end

		currentTrackingVersion = currentTrackingVersion + 3
		Log("RunOnce:RunVersion3() writing RunOnceTrackingVersion of " .. tostring(currentTrackingVersion))
		setTable = { value = currentTrackingVersion } 
		success = ProtectedBinds.SetPersistantStat("EIPS_RunOnceTrackingVersion", setTable)

		inVersion = inVersion + 1

	end

	return inVersion
end

function RunOnce:Execute()
	currentVersion = ProtectedBinds.GetPersistantStat("EIPS_RunOnceVersion")

	if (currentVersion == nil) then
		currentVersion = 0
		Log("RunOnce:Execute() failed to get RunOnceVersion from profile. Creating our version set to 0");
	end

	currentVersion = tonumber(currentVersion)
	startingVersion = currentVersion

	currentTrackingVersion = ProtectedBinds.GetPersistantStat("EIPS_RunOnceTrackingVersion")
	if (currentTrackingVersion == nil) then
		currentTrackingVersion = 0
		Log("RunOnce:Execute() failed to get RunOnceTrackingVersion from profile. Just logging it at 0");
	end
	currentTrackingVersion = tonumber(currentTrackingVersion)


	Log("RunOnce:Execute() currentVersion is " .. tostring(currentVersion))
	Log("RunOnce:Execute() currentTrackingVersion is " .. tostring(currentTrackingVersion))
	
	Log("RunOnce:Execute() is unpatched")
	-- apply patches if required
	--currentVersion = self:RunVersion1(currentVersion)
	--currentVersion = self:RunVersion2(currentVersion)
	--currentVersion = self:RunVersion3(currentVersion)
	-- etc

	if( currentVersion ~= startingVersion ) then
		Log("RunOnce:Execute() writing currentVersion of " .. tostring(currentVersion))

		setTable = { value = currentVersion }
		success = ProtectedBinds.SetPersistantStat("EIPS_RunOnceVersion", setTable)

		if (success) then
			Log("RunOnce:Execute() saving stats to blaze")
			ProtectedBinds.SavePersistantStatsToBlaze()
		end
	end
end

function RunOnce:ExecuteOld()
	--set and check some profile attribute
	setTable = { value = 5 }
	ProtectedBinds.SetProfileAttribute("awesomeness", setTable )
	
	newAwsomeness = ProtectedBinds.GetProfileAttribute("awesomeness")
	
	if( newAwsomeness == 5 ) then
		Log("RunOnce: We changed awesomeness as expected")
	else
		Log("RunOnce: ERROR, Didn't change awesomeness")
	end
end

function RunOnce:Test()
	--attempt to set and read some profile attribute, hopefully fail
	
	setTable = { value = 99 }
	ProtectedBinds.SetProfileAttribute("awesomeness", setTable )
	
	newAwsomeness = ProtectedBinds.GetProfileAttribute("awesomeness")
	
	if( newAwsomeness == 99 ) then
		Log("RunOnce: ERROR, We changed awesomeness when we shouldn't have been able to ")
	else
		Log("RunOnce: Didn't change awesomeness as expected")
	end
end