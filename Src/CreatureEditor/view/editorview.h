//
// 2020-01-19, jjuiddong
// Editor View
//
#pragma once


class cEditorView : public framework::cDockWindow
{
public:
	cEditorView(const StrId &name);
	virtual ~cEditorView();

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
	void CheckCancelUIJoint();
	void CheckChangeSelection();


public:
	float m_radius;
	float m_halfHeight;
	float m_density;
	bool m_isChangeSelection; // ui information update?
	vector<int> m_oldSelects;
};
