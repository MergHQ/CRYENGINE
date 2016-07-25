The MonoRuntime of CRYENGINE is able to convert .NET debug symbol files (namely pdb files) to debug symbols files that are understood by CRYENGINE's scripting engine (namely .dll.mdb files) when importing both the .dll and the .pdb in the AddIn's folder.

If you prefer to handle the conversion yourself you need to call a tool named pdb2mdb on the .dll associated with the .pdb:

```
pdb2mdb MyLibrary.dll
```

Will produce a MyLibrary.dll.mdb usable on CRYENGINE if MyLibrary.pdb is present.

License Information:

* pdb2mdb is licensed under the Microsoft Public License (Ms-PL).
* Mono.Cecil is licensed under the MIT/X11.
