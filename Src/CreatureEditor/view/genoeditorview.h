//
// 2020-02-03, jjuiddong
// GenoType Editor View
//
#pragma once


class cGenoEditorView : public framework::cDockWindow
{
public:
	cGenoEditorView(const StrId &name);
	virtual ~cGenoEditorView();

	virtual void OnUpdate(const float deltaSeconds) override;
	virtual void OnRender(const float deltaSeconds) override;

	
protected:
	void RenderSpawnTransform();
	void RenderSelectionInfo();
	void RenderLinkInfo();
	void RenderFixedJoint();
	void RenderSphericalJoint();
	void RenderRevoluteJoint();
	void RenderPrismaticJoint();
	void RenderDistanceJoint();
	void RenderD6Joint();
	void RenderSelectNodeLinkInfo(const int id);
	void RenderSphericalJointSetting(phys::cJoint *joint);
	void RenderRevoluteJointSetting(phys::cJoint *joint);
	void RenderPrismaticJointSetting(phys::cJoint *joint);
	void RenderDistanceJointSetting(phys::cJoint *joint);
	void RenderD6JointSetting(phys::cJoint *joint);
	bool CheckCancelUIJoint();
	void CheckChangeSelection();
	void UpdateUIJoint(evc::cGNode *gnode0, evc::cGNode *gnode1
		, const bool editAxis, const Vector3 &revoluteAxis);


public:
	float m_radius;
	float m_halfHeight;
	float m_density;
	Vector3 m_eulerAngle; // roll,pitch,yaw (angle)
	bool m_isChangeSelection; // ui information update?
	vector<int> m_oldSelects;
};
