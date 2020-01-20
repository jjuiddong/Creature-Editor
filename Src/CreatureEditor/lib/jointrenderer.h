//
// 2020-01-20, jjuiddong
// joint renderer
//		- connect actor - actor
//		- show joint pivot
//		- control joint direction, pivot position
//		- revolute direction
//
#pragma once


class cJointRenderer
{
public:
	cJointRenderer();
	virtual ~cJointRenderer();

	bool Create(phys::cJoint *joint);
	bool Update(const float deltaSeconds);
	bool Render(graphic::cRenderer &renderer, const XMMATRIX &tm = graphic::XMIdentity);
	void SetPivotPos(const int actorIndex, const Vector3 &pos);
	void Clear();


public:
	phys::cJoint *m_joint; // reference
	phys::sActorInfo *m_info0; // reference
	phys::sActorInfo *m_info1; // reference

	struct sPivot {
		Vector3 dir; // pivot direction from center
		float len; // pivot length from center
	};
	sPivot m_pivots[2]; // actor0,1

	Vector3 m_pivot0; // actor0 pivot position
	Vector3 m_pivot1; // actor1 pivot position
	Vector3 m_pivotDir0; // actor0 pivot direction from center
	Vector3 m_pivotDir1; // actor0 pivot direction from center
};
