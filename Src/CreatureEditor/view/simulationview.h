//
// 2020-01-31, jjuiddong
// Simulation View
//
#pragma once


class cSimulationView : public framework::cDockWindow
{
public:
	cSimulationView(const StrId &name);
	virtual ~cSimulationView();

	virtual void OnUpdate(const float deltaSeconds) override;
	virtual void OnRender(const float deltaSeconds) override;
	virtual void OnEventProc(const sf::Event &evt) override;


protected:
	void Play();
	void RecoverySavedPose();
	void SavePoseCurrentSelection();
	void SaveKinematicStateCurrentSelection();
	void RenderSavePoseList();
	void RenderUnlockActorList();


public:
	struct sPose
	{
		int id; // sync id
		bool kinematic;
		Transform transform; // rigidbody transform
	};
	vector<sPose> m_poseSaved;
	vector<int> m_unLockIds; // sync id
};
