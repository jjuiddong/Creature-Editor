//
// 2020-01-19, jjuiddong
// PhenoType Editor View
//
#pragma once

namespace phys {
	struct sSyncInfo;
}

class cPhenoEditorView : public framework::cDockWindow
{
public:
	cPhenoEditorView(const StrId &name);
	virtual ~cPhenoEditorView();

	virtual void OnUpdate(const float deltaSeconds) override;
	virtual void OnRender(const float deltaSeconds) override;


protected:
	void RenderSpawnTransform();
	void RenderSelectionInfo();
	void RenderJointInfo();
	void RenderFixedJoint();
	void RenderSphericalJoint();
	void RenderRevoluteJoint();
	void RenderPrismaticJoint();
	void RenderDistanceJoint();
	void RenderD6Joint();
	void RenderSelectActorJointInfo(const int syncId);
	void RenderSphericalJointSetting(phys::cJoint *joint);
	void RenderRevoluteJointSetting(phys::cJoint *joint);
	void RenderPrismaticJointSetting(phys::cJoint *joint);
	void RenderDistanceJointSetting(phys::cJoint *joint);
	void RenderD6JointSetting(phys::cJoint *joint);
	bool CheckCancelUIJoint();
	void CheckChangeSelection();
	void UpdateUIJoint(phys::sSyncInfo *sync0
		, phys::sSyncInfo *sync1, const bool editAxis
		, const Vector3 &revoluteAxis);


public:
	float m_radius;
	float m_halfHeight;
	float m_density;
	Vector3 m_eulerAngle; // roll,pitch,yaw (angle)
	bool m_isChangeSelection; // ui information update?
	vector<int> m_oldSelects;
};
