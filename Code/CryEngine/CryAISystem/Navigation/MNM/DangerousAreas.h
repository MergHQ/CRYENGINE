// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

namespace MNM
{

// Heuristics for dangers
enum EWeightCalculationType
{
	eWCT_None = 0,
	eWCT_Range,             // It returns 1 as a weight for the cost if the location to evaluate
							//  is in the specific range from the threat, 0 otherwise
	eWCT_InverseDistance,   // It returns the inverse of the distance as a weight for the cost
	eWCT_Direction,         // It returns a value between [0,1] as a weight for the cost in relation of
							// how the location to evaluate is in the threat direction. The range is not taken into
							// account with this weight calculation type
	eWCT_Last,
};

template<EWeightCalculationType WeightType>
struct DangerWeightCalculation
{
	MNM::real_t CalculateWeight(const Vec3& locationToEval, const Vec3& startingLocation, const Vec3& dangerPosition, const float rangeSq) const
	{
		return MNM::real_t(.0f);
	}
};

template<>
struct DangerWeightCalculation<eWCT_InverseDistance>
{
	MNM::real_t CalculateWeight(const Vec3& locationToEval, const Vec3& startingLocation, const Vec3& dangerPosition, const float rangeSq) const
	{
		// TODO: This is currently not used, but if we see we need it, then we should think to change
		// the euclidean distance calculation with a faster approximation (Like the one used in the FindWay function)
		const float distance = (dangerPosition - locationToEval).len();
		bool isInRange = rangeSq > .0f ? sqr(distance) > rangeSq : true;
		return isInRange ? MNM::real_t(1 / distance) : MNM::real_t(.0f);
	}
};

template<>
struct DangerWeightCalculation<eWCT_Range>
{
	MNM::real_t CalculateWeight(const Vec3& locationToEval, const Vec3& startingLocation, const Vec3& dangerPosition, const float rangeSq) const
	{
		const Vec3 dangerToLocationDir = locationToEval - dangerPosition;
		const float weight = static_cast<float>(__fsel(dangerToLocationDir.len2() - rangeSq, 0.0f, 1.0f));
		return MNM::real_t(weight);
	}
};

template<>
struct DangerWeightCalculation<eWCT_Direction>
{
	real_t CalculateWeight(const Vec3& locationToEval, const Vec3& startingLocation, const Vec3& dangerPosition, const float rangeSq) const
	{
		Vec3 startLocationToNewLocation = (locationToEval - startingLocation);
		startLocationToNewLocation.NormalizeSafe();
		Vec3 startLocationToDangerPosition = (dangerPosition - startingLocation);
		startLocationToDangerPosition.NormalizeSafe();
		float dotProduct = startLocationToNewLocation.dot(startLocationToDangerPosition);
		return max(real_t(.0f), real_t(dotProduct));
	}
};

struct DangerArea
{
	virtual ~DangerArea() {}
	virtual real_t      GetDangerHeuristicCost(const Vec3& locationToEval, const Vec3& startingLocation) const = 0;
	virtual const Vec3& GetLocation() const = 0;
};
DECLARE_SHARED_POINTERS(DangerArea)

template<EWeightCalculationType WeightType>
struct DangerAreaT : public DangerArea
{
	DangerAreaT()
		: location(ZERO)
		, effectRangeSq(.0f)
		, cost(0)
		, weightCalculator()
	{}

	DangerAreaT(const Vec3& _location, const float _effectRange, const uint8 _cost)
		: location(_location)
		, effectRangeSq(sqr(_effectRange))
		, cost(_cost)
		, weightCalculator()
	{}

	virtual real_t GetDangerHeuristicCost(const Vec3& locationToEval, const Vec3& startingLocation) const
	{
		return real_t(cost) * weightCalculator.CalculateWeight(locationToEval, startingLocation, location, effectRangeSq);
	}

	virtual const Vec3& GetLocation() const { return location; }

private:
	const Vec3                                location;      // Position of the danger
	const float                               effectRangeSq; // If zero the effect is on the whole level
	const unsigned int                        cost;          // Absolute cost associated with the Danger represented by the DangerAreaT
	const DangerWeightCalculation<WeightType> weightCalculator;
};

enum { max_danger_amount = 5, };

typedef CryFixedArray<MNM::DangerAreaConstPtr, max_danger_amount> DangerousAreasList;

} // namespace MNM