#pragma once

// Represents an application domain: https://msdn.microsoft.com/en-us/library/2bh4z9hs(v=vs.110).aspx
// Each application domain can load assemblies in a "Sandbox" fashion.
// Assemblies can only be unloaded by unloading the domain that they reside in, with the exception of assemblies in the root domain that is only unloaded on shutdown.
struct IMonoDomain
{
	virtual ~IMonoDomain() {}

	// Whether or not this domain is the root one
	virtual bool IsRoot() = 0;

	// Call to make this the currently active domain
	virtual bool Activate(bool bForce = false) = 0;
	// Used to check if this domain is currently active
	virtual bool IsActive() const = 0;

	// Called to unload an app domain and then reload it afterwards, useful to use newly compiled assemblies without restarting
	virtual bool Reload() = 0;

	virtual void* GetHandle() const = 0;

	virtual void* CreateManagedString(const char* str) = 0;
};