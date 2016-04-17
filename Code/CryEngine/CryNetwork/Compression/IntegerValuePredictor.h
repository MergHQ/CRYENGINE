// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

/*************************************************************************
   -------------------------------------------------------------------------
   $Id$
   $DateTime$
   CIntegerValuePredictor
   Description:  Predicts the range in which the next integer in a continuous
   queue could be positioned based on curve gradient analysis.
   This class is meant to be used as a Memento with the Float Serialization
   Policy and thus is simplyfied and shrinked.
   It is limited to 16 Bit differences and is vulnerable for error values.
   It can't work on frame-differences larger than 16 Bit.
   -------------------------------------------------------------------------
   History:
   - 14/11/2005   18:45 : Created by Jan Müller
*************************************************************************/

#ifndef INTEGER_VALUE_PREDICTOR
#define INTEGER_VALUE_PREDICTOR

#pragma once
#include "Config.h"

#if USE_MEMENTO_PREDICTORS

	#include "Streams/ByteStream.h"

class CIntegerValuePredictor
{

public:

	CIntegerValuePredictor(const int32 startValue = 0);

	/************************************************************************/
	/* Updates the predictor with the real value. (after prediction)        */
	/************************************************************************/
	void Update(int32 value, const int32 prediction, int32 mementoAge);

	/************************************************************************/
	/* Predicts a range in which the next value in a continuous integer
	   queue could probably be positioned.
	   @param	delta is the return of the update() after the last prediction */
	/************************************************************************/
	int32        Predict(uint32& left, uint32& right, uint32 avgHeight, int32 minRange, int32 maxQuantizedValue, int32 maxDifference = 0, int32 mementoAge = 1);

	ILINE uint32 GetOff() const
	{
		return (uint32)m_off;
	}

	ILINE int32 GetDelta() const
	{
		return (int32)m_delta;
	}

	ILINE int32 GetLastValue() const
	{
		return m_lastValue;
	}

	//don't call this after first prediction ...!
	ILINE void Clear(int32 startValue)
	{
		m_lastValue = startValue;
		m_delta = 0;
		m_off = 0;
		m_belowMinimum = 0;
	}

	//this function writes the memento to the stream
	void Serialize(class CByteOutputStream& stm);

	//this function reads a memento from the stream
	void Deserialize(class CByteInputStream& stm);

private:
	//this are the necessary member variables for this predictor/memento

	//this is the delta - the predicted "gradient" for the next value
	int16  m_delta;
	//this is the saved differences between predicted and real values
	uint16 m_off;
	//this is the last value
	int32  m_lastValue;
	//this counter keeps track of very small offs to reduce the minimum range
	uint8  m_belowMinimum;
};

#endif

#endif
