//
// 2020-02-05, jjuiddong
// joint & link limit and drive definition
//
#pragma once


namespace evc
{

	// joint cone limit info
	struct sConeLimit
	{
		float yAngle;
		float zAngle;
	};

	// joint angular limit info
	struct sAngularLimit
	{
		float lower;
		float upper;
	};

	// joint linear limit infor
	struct sLinearLimit
	{
		float lower;
		float upper;
		float stiffness;
		float damping;
		float contactDistance;
		float bounceThreshold;
	};

	// joint distance limit info
	struct sDistanceLimit
	{
		float minDistance;
		float maxDistance;
	};

	// joint drive info
	struct sDriveInfo
	{
		float velocity;
		float period;
		float driveAccel;
	};

}

