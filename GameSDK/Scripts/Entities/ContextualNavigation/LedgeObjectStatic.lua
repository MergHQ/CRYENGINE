--------------------------------------------------------------------------
--	Crytek Source File.
-- 	Copyright (C), Crytek Studios, 2001-2009.
--------------------------------------------------------------------------
--	$Id$
--	$DateTime$
--	Description: Interactive Static Ledge (for editor placement)
--
--------------------------------------------------------------------------
--  History:
--  - 29/02/2012    : Created by Benito Gangoso Rodriguez
--
--------------------------------------------------------------------------
LedgeObjectStatic =
{
  Properties =
  {
    bIsThin			= 0,
	  bIsWindow		= 0,
	  bLedgeDoubleSide = 0,
	  bLedgeFlipped   = 0,
		fCornerMaxAngle = 130.0,
		fCornerEndAdjustAmount = 0.5,

	  MainSide =
	  {
		  bEndCrouched = 0,
		  bEndFalling = 0,	
			bUsableByMarines = 1,
		  esLedgeType = "Normal",
	  },

	  OppositeSide = 
	  {
		  bEndCrouched = 0,
		  bEndFalling = 0,	
			bUsableByMarines=1,
		  esLedgeType = "Normal",
	  },

  },

  Client={},
  Server={},

  Editor=
  {
    Icon="ledge.bmp",
    ShowBounds = 1,
  },
}

function LedgeObjectStatic:OnPropertyChange()

end

function LedgeObjectStatic:IsShapeOnly()
	return 1;
end

function LedgeObjectStatic:IsClosed()
  return 0;
end

function LedgeObjectStatic:ShapeMinPoints()
	return 2;
end

function LedgeObjectStatic:ShapeMaxPoints()
	return 33;
end

function LedgeObjectStatic:ExportToGame()
  return 0;
end

function LedgeObjectStatic:OnSpawn()
	self:SetFlags(ENTITY_FLAG_CLIENT_ONLY, 0);
	self:SetFlags(ENTITY_FLAG_NO_PROXIMITY, 0);
end


