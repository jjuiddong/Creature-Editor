//
// 2020-01-20, jjuiddong
// joint renderer
//		- connect actor - actor
//		- show joint pivot
//		- control joint direction, pivot position
//		- revolute direction
//
#pragma once


class cJointRenderer : public graphic::cNode
{
public:
	cJointRenderer();
	virtual ~cJointRenderer();

	bool Create(phys::cJoint *joint);

	virtual bool Update(graphic::cRenderer &renderer, const float deltaSeconds) override;
	virtual bool Render(graphic::cRenderer &renderer
		, const XMMATRIX &parentTm = graphic::XMIdentity
		, const int flags = 1) override;

	virtual graphic::cNode* Picking(const Ray &ray, const graphic::eNodeType::Enum type
		, const bool isSpherePicking = true
		, OUT float *dist = NULL) override;

	void SetPivotPos(const int actorIndex, const Vector3 &pos);
	Transform GetPivotWorldTransform(const int actorIndex);
	void SetRevoluteAxisPos(const Vector3 &pos);
	bool ApplyPivot();
	void Clear();


protected:
	Transform GetJointTransform();
	Vector3 GetPivotPos(const int actorIndex);


public:
	phys::cJoint *m_joint; // reference
	phys::sSyncInfo *m_sync0; // reference
	phys::sSyncInfo *m_sync1; // reference

	struct sPivot {
		Vector3 dir; // pivot direction(local), actor -> pivot
		float len; // pivot length
	};
	sPivot m_pivots[2]; // actor0,1
	//float m_axisLen;
	Vector3 m_revoluteRelativePos;
};
