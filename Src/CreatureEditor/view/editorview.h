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
	void RenderSelectActorJointInfo(const int actorId);


public:
	Transform m_transform;
	float m_radius;
	float m_halfHeight;
	float m_density;
	map<int, Vector3> m_chDimensions; // key:actorid, value:dimension
};
