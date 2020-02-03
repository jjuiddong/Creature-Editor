
#include "stdafx.h"
#include "simulationview.h"

using namespace graphic;


cSimulationView::cSimulationView(const StrId &name)
	: framework::cDockWindow(name)
{
}

cSimulationView::~cSimulationView()
{
}


void cSimulationView::OnUpdate(const float deltaSeconds)
{
}


void cSimulationView::OnRender(const float deltaSeconds)
{
	if (ImGui::Button("Play (Unlock)"))
	{
		Play();
	}

	ImGui::SameLine(150);
	if (ImGui::Button("Recovery"))
	{
		RecoverySavedPose();
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	if (ImGui::Button("Save Pose"))
	{
		SavePoseCurrentSelection();
	}

	ImGui::SameLine(100);
	if (ImGui::Button("Save State"))
	{
		SaveKinematicStateCurrentSelection();
	}

	RenderSavePoseList();
	RenderUnlockActorList();
}


void cSimulationView::Play()
{
	for (int id : m_unLockIds)
	{
		if (phys::cRigidActor *actor = g_pheno->FindRigidActorFromSyncId(id))
			actor->SetKinematic(false);
	}
}


// recovery saved pose
// 1. remove joint
// 2. update saved pose
// 3. connect joint
void cSimulationView::RecoverySavedPose()
{
	if (m_poseSaved.empty())
		return;

	// set global pose to rigidactor
	for (auto &pose : m_poseSaved)
	{
		phys::sSyncInfo *sync = g_pheno->FindSyncInfo(pose.id);
		if (!sync)
			continue;

		sync->node->m_transform = pose.transform;
		if (sync->actor)
		{
			sync->actor->SetKinematic(true);
			sync->actor->SetGlobalPose(pose.transform);
			sync->actor->SetLinearVelocity(Vector3::Zeroes);
			sync->actor->SetAngularVelocity(Vector3::Zeroes);
			sync->actor->ClearForce();
		}
	}

	// reconnect joint if broken
	set<phys::cJoint*> joints;
	for (auto &pose : m_poseSaved)
	{
		phys::sSyncInfo *sync = g_pheno->FindSyncInfo(pose.id);
		if (!sync || !sync->actor)
			continue;
		for (auto *j : sync->actor->m_joints)
			joints.insert(j);
	}

	// move to local pose and then connect joint
	for (auto &j : joints)
	{
		j->m_actor0->SetGlobalPose(j->m_actorLocal0);
		j->m_actor1->SetGlobalPose(j->m_actorLocal1);
		j->ReconnectBreakJoint(*g_pheno->m_physics);
	}

	// move to global pose
	for (auto &pose : m_poseSaved)
	{
		phys::sSyncInfo *sync = g_pheno->FindSyncInfo(pose.id);
		if (!sync)
			continue;

		sync->node->m_transform = pose.transform;
		if (!sync->actor->m_joints.empty())
			sync->actor->SetGlobalPose(pose.transform);
	}
}


void cSimulationView::SavePoseCurrentSelection()
{
	m_poseSaved.clear();

	for (int id : g_pheno->m_selects)
	{
		phys::sSyncInfo *sync = g_pheno->FindSyncInfo(id);
		if (!sync)
			continue;

		sPose pose;
		pose.id = id;
		pose.transform = sync->node->m_transform;
		pose.kinematic = (sync->actor) ? sync->actor->IsKinematic() : false;
		m_poseSaved.push_back(pose);
	}
}


void cSimulationView::SaveKinematicStateCurrentSelection()
{
	m_unLockIds = g_pheno->m_selects;
}


void cSimulationView::RenderSavePoseList()
{
	ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
	if (ImGui::TreeNode("Save Pose List"))
	{
		for (auto &pose : m_poseSaved)
		{
			Str128 text;
			text.Format("%d", pose.id);
			ImGui::Selectable(text.c_str());
		}
		ImGui::TreePop();
	}
}


void cSimulationView::RenderUnlockActorList()
{
	ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
	if (ImGui::TreeNode("Unlock Actor List"))
	{
		for (int id : m_unLockIds)
		{
			Str128 text;
			text.Format("%d", id);
			ImGui::Selectable(text.c_str());
		}
		ImGui::TreePop();
	}
}
