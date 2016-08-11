#pragma once

#include <CryMath/Cry_Color.h>
#include <CryString/CryString.h>

#include <CryMath/Cry_Math.h>

inline Vec3 Vec3FromString(string sVector)
{
	size_t yPos = sVector.find(",");
	size_t zPos = sVector.find(",", yPos + 1);

	Vec3 vector;

	vector.x = (float)atof(sVector.substr(0, yPos));
	vector.y = (float)atof(sVector.substr(yPos + 1, (zPos - yPos) - 1));
	vector.z = (float)atof(sVector.substr(zPos + 1));

	return vector;
}

inline string Vec3ToString(Vec3 vector)
{
	string sVector;
	sVector.Format("%f,%f,%f", vector.x, vector.y, vector.z);

	return sVector;
}

inline Quat QuatFromString(string sQuat)
{
	size_t xPos = sQuat.find(",");
	size_t yPos = sQuat.find(",", xPos + 1);
	size_t zPos = sQuat.find(",", yPos + 1);

	Quat quaternion;

	quaternion.w = (float)atof(sQuat.substr(0, xPos));;
	quaternion.v.x = (float)atof(sQuat.substr(xPos + 1, (yPos - xPos) - 1));
	quaternion.v.y = (float)atof(sQuat.substr(yPos + 1, (zPos - yPos) - 1));
	quaternion.v.z = (float)atof(sQuat.substr(zPos + 1));

	return quaternion;
}

inline string QuatToString(Quat quaternion)
{
	string sQuaternion;
	sQuaternion.Format("%f,%f,%f,%f", quaternion.w, quaternion.v.x, quaternion.v.y, quaternion.v.z);

	return sQuaternion;
}

inline ColorF StringToColor(const char *sColor, bool adjustGamma)
{
	ColorF color(1.f);
	string colorString = sColor;

	for (int i = 0; i < 4; i++)
	{
		size_t pos = colorString.find_first_of(",");
		if (pos == string::npos)
			pos = colorString.size();

		string sToken = colorString.substr(0, pos);

		float fToken = (float)atof(sToken);

		// Convert to linear space
		if (adjustGamma)
			color[i] = powf(fToken / 255, 2.2f);
		else
			color[i] = fToken;

		if (pos == colorString.size())
			break;
		else
			colorString.erase(0, pos + 1);
	}

	return color;
}

inline Vec3 StringToVec3(const char *sVector)
{
	Vec3 v(ZERO);
	string vecString = sVector;

	for (int i = 0; i < 3; i++)
	{
		size_t pos = vecString.find_first_of(",");
		if (pos == string::npos)
			pos = vecString.size();

		string sToken = vecString.substr(0, pos);

		float fToken = (float)atof(sToken);

		v[i] = fToken;

		if (pos == vecString.size())
			break;
		else
			vecString.erase(0, pos + 1);
	}

	return v;
}

inline Quat StringToQuat(const char *sQuat)
{
	Quat q(ZERO);
	string quatString = sQuat;

	for (int i = 0; i < 4; i++)
	{
		size_t pos = quatString.find_first_of(",");
		if (pos == string::npos)
			pos = quatString.size();

		string sToken = quatString.substr(0, pos);

		float fToken = (float)atof(sToken);

		switch (i)
		{
		case 0:
			q.w = fToken;
			break;
		case 1:
			q.v.x = fToken;
			break;
		case 2:
			q.v.y = fToken;
			break;
		case 3:
			q.v.z = fToken;
			break;
		}

		if (pos == quatString.size())
			break;
		else
			quatString.erase(0, pos + 1);
	}

	return q;
}

inline uint64 StringToMs(string time)
{
	size_t secondsPos = time.find(":");
	size_t msPos = time.find(":", secondsPos + 1);

	string sMinutes = time.substr(0, secondsPos);
	string sSeconds = time.substr(secondsPos + 1, (msPos - secondsPos) - 1);
	string sMS = time.substr(msPos + 1);

	uint64 ms = atoi(sMS.c_str()) + (atoi(sSeconds.c_str()) * 1000) + (atoi(sMinutes.c_str()) * 60000);

	return ms;
}