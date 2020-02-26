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

		bool UpdateLinkInfo();

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
		sGenotypeLink m_prop; // joint property
		cGNode *m_gnode0;
		cGNode *m_gnode1;
		bool m_highlightRevoluteAxis;
	};

}
