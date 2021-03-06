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

	bool Create(phys::cPhysicsSync &sync, phys::cJoint *joint);

	virtual bool Update(graphic::cRenderer &renderer, const float deltaSeconds) override;
	virtual bool Render(graphic::cRenderer &renderer
		, const XMMATRIX &parentTm = graphic::XMIdentity
		, const int flags = 1) override;

	virtual graphic::cNode* Picking(const Ray &ray, const graphic::eNodeType::Enum type
		, const bool isSpherePicking = true
		, OUT float *dist = NULL) override;

	void SetPivotPos(const int actorIndex, const Vector3 &pos);
	Transform GetPivotWorldTransform(const int actorIndex);
	void SetPivotPosByRevoluteAxis(const Vector3 &revoluteAxis, const Vector3 &axisPos);
	void SetPivotPosByRevolutePos(const Vector3 &pos);
	bool GetRevoluteAxis(OUT Vector3 &out0, OUT Vector3 &out1
		, const Vector3 &axisPos=Vector3::Zeroes);
	bool ApplyPivot(phys::cPhysicsEngine &physics);
	void UpdateLimit();
	void Clear();


protected:
	Vector3 GetPivotPos(const int actorIndex);


public:
	phys::cJoint *m_joint; // reference
	phys::sSyncInfo *m_sync0; // reference
	phys::sSyncInfo *m_sync1; // reference
	bool m_highlightRevoluteJoint;

	// cone limit
	union sLimit {
		struct {
			bool isLimit;
			float r;
			float h;
			float ry;
			float rz;
		} cone;

		struct {
			bool isLimit;
			float rtm0[4][4]; // inverse actor0 local tm
			float rtm1[4][4]; // inverse actor1 local tm
		} angular;

		struct {
			bool isLimit;
			float lower;
			float upper;
			float distance; // original actor0,1 distance
		} linear;
	};
	sLimit m_limit;
};
