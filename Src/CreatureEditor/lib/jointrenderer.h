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
	void SetRevoluteAxis(const Vector3 &revoluteAxis, const Vector3 &axisPos);
	void SetRevoluteAxisPos(const Vector3 &pos);
	bool GetRevoluteAxis(OUT Vector3 &out0, OUT Vector3 &out1
		, const Vector3 &axisPos=Vector3::Zeroes);
	bool ApplyPivot();
	void Clear();


protected:
	Vector3 GetPivotPos(const int actorIndex);


public:
	phys::cJoint *m_joint; // reference
	phys::sSyncInfo *m_sync0; // reference
	phys::sSyncInfo *m_sync1; // reference
};
