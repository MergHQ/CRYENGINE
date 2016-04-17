----------------------------------------------------------------------------
CI_NEWTABLE=1
CI_ENDTABLE=2
CI_STRING=3
CI_NUMBER=4


function WriteIndex(stm,idx)
	if(type(idx)=="number")then
		stm:WriteBool(1);
		stm:WriteInt(idx);
	elseif(type(idx)=="string")then
		stm:WriteBool(nil);
		stm:WriteString(idx);
	else
		System.Log("Unrecognized idx type");
	end
end

function ReadIndex(stm)
	local bNumber=stm:ReadBool();
	if(bNumber)then
		return stm:ReadInt();
	else
		return stm:ReadString();
	end
end

function WriteToStream(stm,t,name)
	if(name==nil)then
		name="__root__"
	end
	if(type(t)=="table")then
		stm:WriteByte(CI_NEWTABLE);
		WriteIndex(stm,name);
		for i,val in pairs(t) do
			WriteToStream(stm,val,i);
		end
		stm:WriteByte(CI_ENDTABLE);
	else
		local tt=type(t);
		if(tt=="string")then
			stm:WriteByte(CI_STRING);
			WriteIndex(stm,name);
			stm:WriteString(t);
			--System.Log("STRING "..name.." "..t);
		elseif(tt=="number")then
			stm:WriteByte(CI_NUMBER);
			WriteIndex(stm,name);
			stm:WriteFloat(t);
			--System.Log("NUMBER "..name.." "..t);
		end
	end
end

function ReadFromStream(stm,parent)
	local chunkid=stm:ReadByte();
	local idx=nil;
	local val;
	if(chunkid~=CI_ENDTABLE)then
		idx=ReadIndex(stm);
		if(chunkid==CI_NEWTABLE)then
		------------------------------
			val={}
			while(ReadFromStream(stm,val)~=CI_ENDTABLE)do end
			if(parent)then
				parent[idx]=val;
			end
		------------------------------
		elseif (chunkid==CI_STRING)then
			parent[idx]=stm:ReadString();
		elseif (chunkid==CI_NUMBER)then
			parent[idx]=stm:ReadFloat();
		end
	end

	if(parent==nil)then
		return val;
	else
		return chunkid;
	end
end

--WriteToStream(ProximityTrigger);
--table=ReadFromStream(stm)

----------------------------------------------------------------------------


StringStream = {
	buffer = "",

	New = function()
		local newStream = {};
		for i,v in pairs(StringStream) do
			-- Copy across functions (and buffer)
			newStream[i] = v;
		end
		-- Create empty buffer
		newStream["buffer"] = "";
		return newStream;
	end,

	Write = function( self, str )
		self.buffer = self.buffer..str;
	end,

	WriteLine = function( self, str )
		self.buffer = self.buffer..str.."\n";
	end,

	WriteValue = function( self, v )
		local t = type(v);
		if (t == "number") then
			self.buffer = self.buffer..v;
		elseif (t == "string") then
			self.buffer = self.buffer.."\""..v.."\"";
		elseif (t == "boolean") then
			self.buffer = self.buffer..(v and "true" or "false");
		elseif (t == "table" or t == "function") then
			self.buffer = self.buffer.."["..tostring(v).."]";
		else
			self.buffer = self.buffer.."Unrecognised type "..t;
		end
	end,

	WriteIndex = function( self, v )
		local t = type(v);
		if (t == "number") then
			self.buffer = self.buffer.."["..v.."]";
		elseif (t == "string") then
			self.buffer = self.buffer.."[\""..v.."\"]";
		else
			self.buffer = self.buffer.."Unrecognised type "..t;
		end
	end,

	Reset = function( self )
		self.buffer = "";
	end,

}

----------------------------------------------------------------------------

LuaDumpRecursionTable = Set.New();

-- myTable - table to dump
-- tableName - name of that table
-- stream - stream to write to
-- recuse - boolean flag defining table handling - true recurses, false ignores them
-- indent - current level of indentation (nil -> 0)

-- Now makes a shallow attempt at sorting - ie sorts entries alphabetically and puts tables above other entries
function DumpTableAsLua( myTable, tableName, stream, recurse, indent)

	if (not indent) then
		indent = 0;
	end
	local iS = string.rep("  ", indent);
	stream:WriteLine(iS.."--Automatically dumped LUA table");
	stream:WriteLine(iS..tableName.." = {");

	local newIndent = indent + 2;
	local iiS = string.rep("  ", newIndent);

	local sortTable = {};
	local tableN = 1;
	local localStream = StringStream:New();

	for i,v in pairs(myTable) do
		-- If: value is a table, recursion is allowed, and we haven't already recursed into this table...
		if (type(v) == "table" and recurse and Set.Add(LuaDumpRecursionTable,v)) then
			if (type(i) == "table") then
		 		i = "[Table]";
		 	end
			DumpTableAsLua( v, i, stream, recurse, newIndent) ;
			--Set.Remove(LuaDumpRecursionTable, v);
		else
			localStream:Write(iiS);
			localStream:WriteIndex(i);
			localStream:Write(" = ");
			localStream:WriteValue(v);
			localStream:Write(",");
			sortTable[ tableN ] = localStream.buffer;
			tableN = tableN + 1;
			localStream:Reset();
		end
	end

	table.sort( sortTable );

	for i,v in pairs(sortTable) do
		stream:WriteLine(v);
	end


	if (indent == 0) then
		stream:WriteLine(iS.."}");
	else
		stream:WriteLine(iS.."},");
	end
end

----------------------------------------------------------------------------

function DumpTableAsLuaString( myTable, tableName, recurse )
	StringStream:Reset();
	Set.RemoveAll(LuaDumpRecursionTable);
	DumpTableAsLua( myTable, tableName, StringStream, recurse );
	Set.RemoveAll(LuaDumpRecursionTable);
	return StringStream.buffer;
end

----------------------------------------------------------------------------

-- Table difference
-- For each entry, if there is a difference (in value or existence) between A and B,
-- copy the entry from A into C
function TableDifference( A, B, C )
	table.foreach( A, function(i,v)
		local foo = B[i];
		local bar = v;
		if ( foo ~= bar and tostring(foo) ~= tostring(bar)) then
			-- We use that string comparison as a very general catchall
			-- coping with string/number conversion and f.p. precision variation
			C[i] = v;
		end
	end)
end


----------------------------------------------------------------------------


-- Table difference
-- For each entry, if there is a difference (in value or existence) between A and B,
-- copy the entry from A into C
-- This version recurses into tables
function TableDifferenceRecursive( A, B, C )
	for i,v in pairs(A) do
		if (type(v) == "table") then
			C[i] = {};
			if (B[i]) then
				-- Recurse into both tables
				TableDifferenceRecursive( v, B[i], C[i] );
				-- If that table comes back empty, remove it
				local first = pairs(C[i])(C[i]);
				if (not first) then
					C[i] = nil;
				end
			else
				-- Copy whole table from A to C
				table.foreach( v, function(ii,vv)
					C[i][ii] = vv;
				end);
			end
		else
			local foo = B[i];
			local bar = v;
			if ( foo ~= bar and tostring(foo) ~= tostring(bar)) then
				-- We use that string comparison as a very general catchall
				-- coping with string/number conversion and f.p. precision variation
				C[i] = v;
			end
		end
	end
end

----------------------------------------------------------------------------

-- Table add
-- For each entry, if it exists in A but not in B, copy from A into B
function TableAdd( A, B )
	for i,v in pairs(A) do
		if (B[i]==nil) then
			B[i] = v;
		end
	end
end

----------------------------------------------------------------------------

-- Table add
-- For each entry, if it exists in A but not in B, copy from A into B
-- This version recurses into tables
function TableAddRecursive( A, B )
	for i,v in pairs(A) do
	  if (B[i]==nil) then
			if (type(v) == "table") then
		  	B[i] = {};
		  	TableAddRecursive( v, B[i] );
		  else
				B[i] = v;
			end
		elseif (type(v) == "table" and type(B[i])=="table") then
			TableAddRecursive( v, B[i] );
		end
	end
end

----------------------------------------------------------------------------

-- Table intersect on keys
-- For each entry, if it does not exist in A but does in B, remove from B
function TableIntersectKeys( A, B )
	-- Is removal from a table like this really well-defined?
	for i,v in pairs(B) do
		if (A[i]==nil) then
			B[i] = nil;
		end
	end
end

----------------------------------------------------------------------------

-- Table intersect on keys
-- For each entry, if it does not exist in A but does in B, remove from B
-- This version is recursive
function TableIntersectKeysRecursive( A, B )
	-- Is removal from a table like this really well-defined?
	for i,v in pairs(B) do
		if (A[i]==nil) then
			B[i] = nil;
		else
			if (type(A[i]) == "table" and type(v)=="table") then
			  TableIntersectKeysRecursive( A[i], v );
			end
		end
	end
end

----------------------------------------------------------------------------


-- Table add
-- For each entry, if it exists in A but not in B, copy from A into B
-- This version recurses into tables
function TableAddRecursive( A, B )
	for i,v in pairs(A) do
	  if (not B[i]) then
			if (type(v) == "table") then
		  	B[i] = {};
		  	TableAddRecursive( v, B[i] );
		  else
				B[i] = v;
			end
		elseif (type(v) == "table" and type(B[i])=="table") then
			TableAddRecursive( v, B[i] );
		end
	end
end


----------------------------------------------------------------------------

-- Assumes subtables have unique names

function FlattenTree(tree, flat, rootName)
	if (not rootName) then
		rootName = "Root";
	end
	flat[rootName] = {};
	FlattenSubTree( tree, flat, flat[rootName] );
end

----------------------------------------------------------------------------

function FlattenSubTree(subTree, flat, subTable)
	table.foreach( subTree, function(i,v)
		if (type(v) == "table") then
			flat[i] = {};
			FlattenSubTree( v, flat, flat[i] );
		else
			subTable[i] = v;
		end
	end);
end


----------------------------------------------------------------------------
-- Split up huge strings into smaller ones to allow dumping to log, etc

function BreakUpHugeString( input, minSize, maxSize )
	minSize = minSize or 128;
	maxSize = maxSize or 256;
	
	local result = {};
	local i=1;
	local inputLen = string.len(input);
	local resultNum = 1;

	local start = i;	
	while (i < inputLen) do
		local endLine = string.find( input, "\n", i, true); 
		if (not endLine) then
			-- No newlines before end of string
			endLine = inputLen;
		end

		local chunkLen = endLine - start;
		if (chunkLen > maxSize) then
			-- Just take maxSize characters if too long
			endLine = start + maxSize;
		end
		
		if (chunkLen > minSize or endLine == inputLen) then
			-- If big enough, or end of input, take this chunk
			result[resultNum] = string.sub(input,start,endLine); 
			resultNum = resultNum + 1;		
			-- Carry on after that chunk
			start = endLine + 1;
		end
		
		i = endLine + 1;
	end

	return result;
end




---------------------------------------------------------------------------

function LogTable( table, tableName, recurse )
	tableName = tableName or "DumpedTable";
	if (recurse == nil) then
		recurse = true;
	end
	
	local input = DumpTableAsLuaString( table, tableName, recurse );
	local chunks = BreakUpHugeString( input );
	for i,v in ipairs(chunks) do
		System.Log( v );
	end
end


----------------------------------------------------------------------------

-- Take a path to a value and return it
function GetValueRecursive( path )
	-- Easiest way is using loadstring
	local f, err = loadstring("return "..path);
	if (f) then
		local status, result = pcall(f);
		if (status) then return result; end
	end
	return nil;
end


----------------------------------------------------------------------------

-- Take a path to a value and set that value
-- Should be changed to create path if required
-- Currently won't handle functions
function SetValueRecursive( path, value )
	-- Easiest way is using loadstring
	local f,err = loadstring(path.." = "..value);
	if (f) then
		return f();
	end
	return nil;
end



----------------------------------------------------------------------------

-- Print out a value or values, whatever they are, simulating print
function out( ... ) 
	-- Are we running in the engine?
	if ( System and System.Log) then
		-- How many arguments are there really?
		local argc = 0;
		for i = 1, 10 do
			if (arg[i]~=nil) then
				argc = i;
			end
		end
		
		local s = "";
		local i = 0;
		while ( i< argc ) do
			i = i + 1;
			s = s..tostring(arg[i]).."\t";		
		end
		System.Log(s);
	else
		if (print) then
			print( arg );
		end
	end

end


----------------------------------------------------------------------------
-- Testing
----------------------------------------------------------------------------

--Automatically dumped LUA table
local testTable = {
		--Automatically dumped LUA table
		AnotherSubTable = {
				[57] = 5,
				[32.7] = 2,
				--Automatically dumped LUA table
				SubSubTable = {
						["arf"] = "noodle",
				},
		},
		--Automatically dumped LUA table
		SubTable = {
				["black"] = 5,
				["bar"] = 2,
		},
		["cao"] = true,
		["boo"] = false,
		["aoo"] = 1,
		["coo"] = 3,
}

--print (DumpTableAsLuaString(testTable, "testTable"));
--print (DumpTableAsLuaString(testTable, "testTable", true));
--local flat = {};
--FlattenTree( testTable, flat, "Rooty");
--local testStr = "foo\nbar\nhowisyourfather\n";
--print (DumpTableAsLuaString(BreakUpHugeString(testStr, 5, 10), "testStr", true));
-- Should yield:
--[[
testStr = {
		[1] = "foo
bar
",
		[2] = "howisyourfa",
		[3] = "ther
",
}   ]]--

