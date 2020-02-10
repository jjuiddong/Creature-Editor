//
// 2020-02-06, jjuiddong
//	- genotype data definition
//
#pragma once


namespace evc
{

	// joint cone limit info
	struct sConeLimit
	{
		bool isLimit;
		float yAngle;
		float zAngle;
	};

	// joint angular limit info
	struct sAngularLimit
	{
		bool isLimit;
		float lower;
		float upper;
	};

	// joint linear limit infor
	struct sLinearLimit
	{
		bool isLimit;
		bool isSpring;
		float value;
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
		bool isLimit;
		float minDistance;
		float maxDistance;
	};

	struct sDriveParam
	{
		bool isDrive;
		float stiffness;
		float damping;
		float forceLimit;
		bool accel;
	};

	// joint d6 limit info
	struct sD6Limit
	{
		int motion[6]; // PxD6Motion::Enum
		sDriveParam drive[6];
		sLinearLimit linear;
		sAngularLimit twist;
		sConeLimit swing;
		float linearVelocity[3]; // compile error when declare Vector3, tricky code
		float angularVelocity[3]; // compile error when declare Vector3, tricky code
	};

	// joint drive info
	struct sDriveInfo
	{
		bool isDrive;
		float velocity;
		bool isCycle;
		float period;
		float driveAccel;
		float maxVelocity;
	};

	// pivot information
	struct sPivot
	{
		Vector3 dir; // pivot direction(local space), gnode -> pivot
		float len; // pivot length
	};


	struct sGenotypeNode
	{
		int id; // unique id
		StrId name;
		phys::eShapeType::Enum shape;
		graphic::cColor color;
		float density;
		float mass;
		float linearDamping;
		float angularDamping;
		int iteration; // iterate genotype node id
		bool generation; // internal used, is generation node?
		Transform transform; // dimension
							 //           box : scale
							 //           sphere : radius = scale.x
							 //			  capsule : radius = scale.y
							 //			            halfheight = scale.x - scale.y
							 //			  cylinder: radius = scale.y
							 //			            height = scale.x	
	};


	struct sGenotypeLink
	{
		phys::eJointType::Enum type;
		sGenotypeNode *gnode0; // parent reference
		sGenotypeNode *gnode1; // child reference

		// joint property
		Vector3 revoluteAxis; // local space
		Vector3 origPos; // joint origin pos (local space)
		Quaternion rotRevolute; // X-axis -> revoluteAxis rotation (local space)
							    // revoluteAxis = normal(pivot1 - pivot0)
		Transform nodeLocal0; // gnode0 local transform (local space)
		Transform nodeLocal1; // gnode1 local transform (local space)

		union sLimit {
			sConeLimit cone;
			sAngularLimit angular;
			sLinearLimit linear;
			sDistanceLimit distance;
			sD6Limit d6;
		};
		sLimit limit;
		sDriveInfo drive; // revolute joint
		sPivot pivots[2]; // gnode0,1
	};

}
