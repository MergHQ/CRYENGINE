----------------------------------------------------------------------------------------------------
--  Crytek Source File.
--  Copyright (C), Crytek Studios, 2001-2004.
----------------------------------------------------------------------------------------------------
--  $Id$
--  $DateTime$
--  Description: Math Utility Functions Used by Item System
--
----------------------------------------------------------------------------------------------------
--  History:
--  - 22:11:2004   12:13 : Created by Márcio Martins
--
----------------------------------------------------------------------------------------------------
----------------------------------------------------------------------------------------------------
-- return squared distance of point to line, and closest point in line
function PointToLineDistanceSq(point, lineStart, lineEnd)
	local v = vecSub(lineEnd, lineStart);
	local lineLenSq = vecLenSq(v);

	if (lineLenSq > 0.001) then
		local u = (((point.x-lineStart.x) * (lineEnd.x-lineStart.x))+
							((point.y-lineStart.y) * (lineEnd.y-lineStart.y))+
							((point.z-lineStart.z) * (lineEnd.z-lineStart.z)))/lineLenSq;
							
		if (u > 0 and u < 1) then
			local i = g_Vectors.temp_v2;
			i.x = lineStart.x+u*(lineEnd.x-lineStart.x);
			i.y = lineStart.y+u*(lineEnd.y-lineStart.y);
			i.z = lineStart.z+u*(lineEnd.z-lineStart.z);
			
			return vecLenSq(vecSub(point, i)), i;
		end
	end
end


----------------------------------------------------------------------------------------------------
function PointInPolygon(p, polygon)
	local n = table.getn(polygon);
	local r = false;
	local j = 1;
	
	for i=1,n do
		j=j+1;

		if (j > n) then
			j = 1;
		end

		if ((polygon[i].y<p.y and polygon[j]>=p.y) or(polygon[j].y<p.y and polygon[i].y>=p.y)) then
			if (polygon[i].x+(p.y-polygon[i].y)/(polygon[j].y-polygon[i].y)*(polygon[j].x-polygon[i].x) < x) then
				r = not r;
			end
		end
	end

	return r;
end


----------------------------------------------------------------------------------------------------
function PointInCircle(x, y, center, r)
	local dx = x-center.x;
	local dy = y-center.y;

	if (dx*dx+dy*dy<r*r) then
		return true;
	end
	
	return false;
end

----------------------------------------------------------------------------------------------------
----------------------------------------------------------------------------------------------------
-- compute a right vector from a front vector
-- up is optional
function vecFrontToRight(front, up)
	if (not up) then
		up = g_Vectors.v001;
	end
	
	return vecNormalize(vecCross(front, up));
end


----------------------------------------------------------------------------------------------------
-- compute an up vector from a front vector
-- up is optional
function vecFrontToUp(front, up)
	if (not up) then
		up = g_Vectors.v001;
	end
	
	local right = vecCopy(g_Vectors.temp_v1, vecNormalize(vecCross(front, up)));

	return vecNormalize(vecCross(right, front));
end


----------------------------------------------------------------------------------------------------
function vecToAngles(vec)
	local x,y = vec.x, vec.y;
	local l = math.sqrt(x*x+y*y);
	local rcpl = 1.0;	

	if (l >= 0.99) then
		local rcpl = 1.0/l;
	end
	
	-- convert to angles
	local ax = math.atan2(-x*rcpl, y*rcpl);
	local ay = math.atan2(vec.z, l);
	
	local v = g_Vectors.vecMathTemp2;
	v.x=ax;
	v.y=ay;
	v.z=0;
	
	return v;
end


----------------------------------------------------------------------------------------------------
function anglesToVec(angles)
	local v = g_Vectors.vecMathTemp2;
	
	local cz = math.cos(angles.x);
	local sz = math.sin(angles.x);
	local cx = math.cos(angles.y);
	local sx = math.sin(angles.y);
	
	v.x=-sz*cx;
	v.y=cz*cx;
	v.z=sx;
	
	return v;
end


----------------------------------------------------------------------------------------------------
function vecNew(v)
	if (not v) then
		return {x=0, y=0, z=0};
	else
		return vecCopy({}, v);
	end
end


----------------------------------------------------------------------------------------------------
function vecSet(v, x, y, z)
	v.x = x;
	v.y = y;
	v.z = z;
	
	return v;
end


----------------------------------------------------------------------------------------------------
function vecCopy(va, vb)
	va.x = vb.x;
	va.y = vb.y;
	va.z = vb.z;
	
	return va;
end


----------------------------------------------------------------------------------------------------
function vecDistanceSq(v, t)
	local tx = v.x-t.x;
	local ty = v.y-t.y;
	local tz = v.z-t.z;
	
	return tx*tx+ty*ty+tz*tz;
end


----------------------------------------------------------------------------------------------------
function vec2DDistanceSq(v, t)
	local tx = v.x-t.x;
	local ty = v.y-t.y;
	
	return tx*tx+ty*ty;
end


----------------------------------------------------------------------------------------------------
function vecScale(v, t)
	local va = g_Vectors.vecMathTemp1;
	
	va.x=v.x*t;
	va.y=v.y*t;
	va.z=v.z*t;
	
	return va;
end


----------------------------------------------------------------------------------------------------
function vecSLerp(va, vb, t, angle)
	local radians;
	
	if (angle) then
		radians = angle;
	else
		radians = math.acos(vecDot(va, vb));
	end

	local s2 = math.sin(radians);
	
	if (math.abs(s2) > 0.01) then
		local s0 = math.sin((1-t)*radians);
		local s1 = math.sin(t*radians);
		local sf = 1/s2;
		
		local v = g_Vectors.vecMathTemp1;
		
		v.x=sf*(va.x*s0+vb.x*s1);
		v.y=sf*(va.y*s0+vb.y*s1);
		v.z=sf*(va.z*s0+vb.z*s1);

		return v;
	else
		return vecLerp(va, vb, t);
	end
end


----------------------------------------------------------------------------------------------------
function vecLerp(va, vb, t)
	local v = g_Vectors.vecMathTemp1;
	local ot = 1-t;
	
	v.x=va.x*ot+vb.x*t;
	v.y=va.y*ot+vb.y*t;
	v.z=va.z*ot+vb.z*t;

	return v;
end


----------------------------------------------------------------------------------------------------
function vecSub(va, vb)
	local v = g_Vectors.vecMathTemp1;
	
	v.x=va.x-vb.x;
	v.y=va.y-vb.y;
	v.z=va.z-vb.z;
	
	return v;
end


----------------------------------------------------------------------------------------------------
function vecAdd(va, vb)
	local v = g_Vectors.vecMathTemp1;
	
	v.x=va.x+vb.x;
	v.y=va.y+vb.y;
	v.z=va.z+vb.z;
	
	return v;
end


----------------------------------------------------------------------------------------------------
function vecLenSq(v)
	return v.x*v.x+v.y*v.y+v.z*v.z;
end


----------------------------------------------------------------------------------------------------
function vecLen(v)
	return math.sqrt(v.x*v.x+v.y*v.y+v.z*v.z);
end


----------------------------------------------------------------------------------------------------
function vecMul(va, vb)
	local v = g_Vectors.vecMathTemp1;
	
	v.x = va.x*vb.x;
	v.y = va.y*vb.y;
	v.z = va.z*vb.z;
	
	return v;
end


----------------------------------------------------------------------------------------------------
function vecNormalize(va)
	local l = math.sqrt(va.x*va.x+va.y*va.y+va.z*va.z);
	if (l>0) then
		local t = 1/l;
		local v = g_Vectors.vecMathTemp1;
		
		v.x = va.x*t;
		v.y = va.y*t;
		v.z = va.z*t;
		
		return v, l;
	end
	return v,0;
end


----------------------------------------------------------------------------------------------------
function vecDot(va, vb)
	return va.x*vb.x+va.y*vb.y+va.z*vb.z;
end


----------------------------------------------------------------------------------------------------
function vecCross(va, vb)
	local v = g_Vectors.vecMathTemp1;
	
	v.x=va.y*vb.z-va.z*vb.y;
	v.y=va.z*vb.x-va.x*vb.z;
	v.z=va.x*vb.y-va.y*vb.x;
	
	return v;
end


----------------------------------------------------------------------------------------------------
function retDef(val, def)
	if (val) then
		return val;
	end

	return def;
end