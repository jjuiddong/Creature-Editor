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
	void RenderSphericalJointSetting(phys::cJoint *joint, INOUT evc::sGenotypeLink &info);
	void RenderRevoluteJointSetting(phys::cJoint *joint, INOUT evc::sGenotypeLink &info);
	void RenderPrismaticJointSetting(phys::cJoint *joint, INOUT evc::sGenotypeLink &info);
	void RenderDistanceJointSetting(phys::cJoint *joint, INOUT evc::sGenotypeLink &info);
	void RenderD6JointSetting(phys::cJoint *joint, INOUT evc::sGenotypeLink &info);
	bool CheckCancelUIJoint();
	void CheckChangeSelection();
	void UpdateUIJoint(phys::sSyncInfo *sync0
		, phys::sSyncInfo *sync1, const bool editAxis
		, const Vector3 &revoluteAxis);
	void UpdateJointInfo(const int syncId);


public:
	float m_radius;
	float m_halfHeight;
	float m_density;
	Vector3 m_eulerAngle; // roll,pitch,yaw (angle)
	bool m_isChangeSelection; // ui information update?
	vector<int> m_oldSelects;
	vector<evc::sGenotypeLink> m_jointInfos; // temporal store joint
};
