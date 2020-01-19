
#include "stdafx.h"
#include "editorview.h"

using namespace graphic;

cEditorView::cEditorView(const StrId &name)
	: framework::cDockWindow(name)
	, m_transform(Vector3(0, 10, 0), Vector3(0.5f,0.5f,0.5f))
	, m_radius(0.5f)
	, m_halfHeight(1.f)
	, m_density(1.f)
{
}

cEditorView::~cEditorView()
{
}


void cEditorView::OnUpdate(const float deltaSeconds)
{
}


void cEditorView::OnRender(const float deltaSeconds)
{
	cRenderer &renderer = g_global->GetRenderer();
	phys::cPhysicsSync *physSync = g_global->m_physSync;

	ImGui::Checkbox("Debug", &renderer.m_isDbgRender);
	ImGui::InputInt("DebugType", &renderer.m_dbgRenderStyle);

	RenderSpawnTransform();
	RenderSelectionInfo();
}


void cEditorView::RenderSpawnTransform()
{
	cRenderer &renderer = g_global->GetRenderer();
	phys::cPhysicsSync *physSync = g_global->m_physSync;

	ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
	if (ImGui::CollapsingHeader("Spawn Transform"))
	{
		ImGui::TextUnformatted("Pos  ");
		ImGui::SameLine();
		ImGui::DragFloat3("##position", (float*)&m_transform.pos, 0.001f);

		ImGui::TextUnformatted("Dim ");
		ImGui::SameLine();
		ImGui::DragFloat3("##scale", (float*)&m_transform.scale, 0.001f, 0.01f, 1000.f);

		ImGui::TextUnformatted("Rot  ");
		ImGui::SameLine();
		static Vector3 rot;
		ImGui::DragFloat3("##rotation", (float*)&rot, 0.001f);

		ImGui::TextUnformatted("Radius       ");
		ImGui::SameLine();
		ImGui::DragFloat("##radius", &m_radius, 0.001f);

		ImGui::TextUnformatted("HalfHeight");
		ImGui::SameLine();
		ImGui::DragFloat("##halfheight", &m_halfHeight, 0.001f);

		ImGui::TextUnformatted("Density      ");
		ImGui::SameLine();
		ImGui::DragFloat("##density", &m_density, 0.001f, 0.0f, 1000.f);

		static bool isKinematic = true;
		ImGui::TextUnformatted("Kinematic  ");
		ImGui::SameLine();
		ImGui::Checkbox("##kinematic", &isKinematic);

		ImGui::Spacing();
		int id = -1;
		if (ImGui::Button("Box"))
		{
			id = physSync->SpawnBox(renderer, m_transform, m_density);
		}

		ImGui::SameLine();
		if (ImGui::Button("Sphere"))
		{
			id = physSync->SpawnSphere(renderer, m_transform.pos, m_radius, m_density);
		}

		ImGui::SameLine();
		if (ImGui::Button("Capsule"))
		{
			Transform tfm(m_transform.pos);
			id = physSync->SpawnCapsule(renderer, tfm, m_radius, m_halfHeight, m_density);
		}

		if (id >= 0)// creation?
		{
			phys::cPhysicsSync::sActorInfo *info = physSync->FindActorInfo(id);
			if (info && info->actor->m_dynamic)
			{
				info->actor->m_dynamic->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, isKinematic);
			}
		}

		ImGui::Spacing();
		ImGui::Spacing();
	}
}


void cEditorView::RenderSelectionInfo()
{
	using namespace graphic;
	cRenderer &renderer = g_global->GetRenderer();
	phys::cPhysicsSync *physSync = g_global->m_physSync;
	const int selId = g_global->m_selectActorId;

	ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
	if (ImGui::CollapsingHeader("Selection"))
	{
		phys::cPhysicsSync::sActorInfo *info = physSync->FindActorInfo(selId);
		if (!info)
			return;

		Transform tfm = info->node->m_transform;

		ImGui::TextUnformatted("Pos   ");
		ImGui::SameLine();
		const bool edit0 = ImGui::DragFloat3("##position2", (float*)&tfm.pos, 0.001f);

		ImGui::TextUnformatted("Scale");
		ImGui::SameLine();
		const bool edit1 = ImGui::DragFloat3("##scale2", (float*)&tfm.scale, 0.001f, 0.01f, 1000.f);

		ImGui::TextUnformatted("Rot   ");
		ImGui::SameLine();
		static Vector3 ryp;//roll yaw pitch
		const bool edit2 = ImGui::DragFloat3("##rotation2", (float*)&ryp, 0.001f);

		using namespace physx;

		if (info->actor->m_dynamic)
		{
			const PxRigidBodyFlags flags = info->actor->m_dynamic->getRigidBodyFlags();
			bool isKinematic = flags.isSet(PxRigidBodyFlag::eKINEMATIC);
			if (ImGui::Checkbox("Kinematic", &isKinematic))
			{
				info->actor->m_dynamic->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, isKinematic);
				info->actor->m_dynamic->wakeUp();
			}
		}

		if (edit0)
		{
			info->node->m_transform.pos = tfm.pos;

			PxTransform tm(*(PxVec3*)&tfm.pos, *(PxQuat*)&tfm.rot);
			info->actor->m_dynamic->setGlobalPose(tm);
		}

		if (edit1)
		{
			info->node->m_transform.scale = tfm.scale;
		}

		if (edit2)
		{
			//selectModel->m_transform.rot.Euler2(ryp);
		}
	}

}


