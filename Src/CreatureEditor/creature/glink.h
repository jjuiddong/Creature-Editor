//
// 2020-02-03, jjuiddong
// GenoType Node Link
//		- between genotype node connection link
//
#pragma once


namespace evc
{
	class cGNode;

	class cGLink : public graphic::cNode
	{
	public:
		cGLink();
		virtual ~cGLink();

		bool Create(const sGenotypeLink &glink, cGNode *gnode0, cGNode *gnode1);

		bool CreateFixed(cGNode *gnode0, const Vector3 &pivot0
			, cGNode *gnode1, const Vector3 &pivot1);

		bool CreateSpherical(cGNode *gnode0, const Vector3 &pivot0
			, cGNode *gnode1, const Vector3 &pivot1);

		bool CreateRevolute(cGNode *gnode0, const Vector3 &pivot0
			, cGNode *gnode1, const Vector3 &pivot1, const Vector3 &revoluteAxis);

		bool CreatePrismatic(cGNode *gnode0, const Vector3 &pivot0
			, cGNode *gnode1, const Vector3 &pivot1, const Vector3 &revoluteAxis);

		bool CreateDistance(cGNode *gnode0, const Vector3 &pivot0
			, cGNode *gnode1, const Vector3 &pivot1);

		bool CreateD6(cGNode *gnode0, const Vector3 &pivot0
			, cGNode *gnode1, const Vector3 &pivot1);

		virtual bool Render(graphic::cRenderer &renderer
			, const XMMATRIX &parentTm = graphic::XMIdentity
			, const int flags = 1) override;

		virtual graphic::cNode* Picking(const Ray &ray, const graphic::eNodeType::Enum type
			, const bool isSpherePicking = true
			, OUT float *dist = NULL) override;

		void SetPivotPos(const int nodeIndex, const Vector3 &pos);
		Transform GetPivotWorldTransform(const int nodeIndex);
		Vector3 GetPivotPos(const int nodeIndex);
		void SetRevoluteAxis(const Vector3 &revoluteAxis);
		void SetPivotPosByRevoluteAxis(const Vector3 &revoluteAxis, const Vector3 &axisPos);
		void SetPivotPosByRevolutePos(const Vector3 &pos);
		bool GetRevoluteAxis(OUT Vector3 &out0, OUT Vector3 &out1
			, const Vector3 &axisPos = Vector3::Zeroes);

		void Clear();


	public:
		bool m_autoDelete; // default: true
		phys::eJointType::Enum m_type;
		cGNode *m_gnode0;
		cGNode *m_gnode1;
		bool m_highlightRevoluteAxis;

		// joint property
		const float m_breakForce;
		float m_revoluteAxisLen; // revolute, prismatic joint axis
		Vector3 m_revoluteAxis; // local space
		Vector3 m_origPos; // joint origin pos (local space)
		Quaternion m_rotRevolute; // Xaxis -> revoluteAxis rotation (local space)
								  // revoluteAxis = normal(pivot1 - pivot0)
		Transform m_nodeLocal0; // gnode0 local transform (local space)
		Transform m_nodeLocal1; // gnode1 local transform (local space)

		sPivot m_pivots[2]; // gnode0,1
		sDriveInfo m_drive; // revolute joint

		union sLimit {
			sConeLimit cone;
			sAngularLimit angular;
			sLinearLimit linear;
			sDistanceLimit distance;
			sD6Limit d6;
		};
		sLimit m_limit;
	};

}
