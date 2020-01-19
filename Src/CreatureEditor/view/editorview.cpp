
#include "stdafx.h"
#include "editorview.h"

using namespace graphic;

cEditorView::cEditorView(const StrId &name)
	: framework::cDockWindow(name)
	, m_transform(Vector3(0, 2, 0), Vector3(0.5f,0.5f,0.5f))
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
	RenderJointInfo();
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
	int selId = (g_global->m_selects.size() == 1) ? *g_global->m_selects.begin() : -1;

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
				if (!isKinematic)
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


void cEditorView::RenderJointInfo()
{
	ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
	if (ImGui::CollapsingHeader("Joint"))
	{
		if (g_global->m_selects.size() == 2)
		{
			const char *jointType = "Fixed\0Spherical\0Revolute\0Prismatic\0Distance\0D6\0\0";
			static int idx = 0;
			ImGui::TextUnformatted("Joint");
			ImGui::SameLine();
			ImGui::Combo("##joint type", &idx, jointType);
			ImGui::Separator();

			switch ((phys::cJoint::eType)idx)
			{
			case phys::cJoint::eType::Fixed: RenderFixedJoint(); break;
			case phys::cJoint::eType::Spherical: RenderSphericalJoint(); break;
			case phys::cJoint::eType::Revolute: RenderRevoluteJoint(); break;
			case phys::cJoint::eType::Prismatic: RenderPrismaticJoint(); break;
			case phys::cJoint::eType::Distance: RenderDistanceJoint(); break;
			case phys::cJoint::eType::D6: RenderD6Joint(); break;
			}
		}
	}
}


void cEditorView::RenderFixedJoint()
{
	auto it = g_global->m_selects.begin();
	const int actorId0 = *it++;
	const int actorId1 = *it++;

	phys::cPhysicsSync *physSync = g_global->m_physSync;
	phys::cPhysicsSync::sActorInfo *info0 = physSync->FindActorInfo(actorId0);
	phys::cPhysicsSync::sActorInfo *info1 = physSync->FindActorInfo(actorId1);
	if (!info0 || !info1)
		return;

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.4f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.8f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0, 1));
	if (ImGui::Button("Create Fixed Joint"))
	{
		phys::cJoint *joint = new phys::cJoint();
		joint->CreateFixed(g_global->m_physics
			, info0->actor, info0->node->m_transform
			, info1->actor, info1->node->m_transform);
		physSync->AddJoint(joint);
	}
	ImGui::PopStyleColor(3);
}


void cEditorView::RenderSphericalJoint()
{
	using namespace physx;

	auto it = g_global->m_selects.begin();
	const int actorId0 = *it++;
	const int actorId1 = *it++;

	phys::cPhysicsSync *physSync = g_global->m_physSync;
	phys::cPhysicsSync::sActorInfo *info0 = physSync->FindActorInfo(actorId0);
	phys::cPhysicsSync::sActorInfo *info1 = physSync->FindActorInfo(actorId1);
	if (!info0 || !info1)
		return;

	static Vector2 limit(PxPi / 2.f, PxPi / 6.f);
	ImGui::Text("Limit Cone (Radian)");
	ImGui::DragFloat("Y Limit Angle", &limit.x, 0.001f);
	ImGui::DragFloat("Z Limit Angle", &limit.y, 0.001f);
	
	static bool isLimit;
	ImGui::TextUnformatted("Limit");
	ImGui::SameLine();
	ImGui::Checkbox("##Limit", &isLimit);

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.4f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.8f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0, 1));
	if (ImGui::Button("Create Spherical Joint"))
	{
		phys::cJoint *joint = new phys::cJoint();
		joint->CreateSpherical(g_global->m_physics
			, info0->actor, info0->node->m_transform
			, info1->actor, info1->node->m_transform);

		if (isLimit)
		{
			joint->m_sphericalJoint->setLimitCone(PxJointLimitCone(limit.x, limit.y, 0.01f));
			joint->m_sphericalJoint->setSphericalJointFlag(PxSphericalJointFlag::eLIMIT_ENABLED, true);
		}

		physSync->AddJoint(joint);
	}
	ImGui::PopStyleColor(3);
}


void cEditorView::RenderRevoluteJoint()
{
	using namespace physx;

	auto it = g_global->m_selects.begin();
	const int actorId0 = *it++;
	const int actorId1 = *it++;

	phys::cPhysicsSync *physSync = g_global->m_physSync;
	phys::cPhysicsSync::sActorInfo *info0 = physSync->FindActorInfo(actorId0);
	phys::cPhysicsSync::sActorInfo *info1 = physSync->FindActorInfo(actorId1);
	if (!info0 || !info1)
		return;

	static Vector2 limit(-PxPi / 4.f, PxPi / 4.f);
	static bool isDrive = false;
	static float velocity = 1.f;
	ImGui::Text("Angular Limit (Radian)");
	ImGui::DragFloat("Lower Limit Angle", &limit.x, 0.001f);
	ImGui::DragFloat("Upper Limit Angle", &limit.y, 0.001f);
	ImGui::Checkbox("Drive", &isDrive);
	ImGui::DragFloat("Velocity", &velocity, 0.001f);

	const char *axisStr = "X\0Y\0Z\0\0";
	const static Vector3 axis[3] = { Vector3(1,0,0), Vector3(0,1,0), Vector3(0,0,1) };
	static int axisIdx = 0;
	ImGui::Combo("Revolute Axis", &axisIdx, axisStr);

	static bool isLimit = false;
	ImGui::TextUnformatted("Limit");
	ImGui::SameLine();
	ImGui::Checkbox("##Limit", &isLimit);

	//static bool isDrive = false;
	//ImGui::TextUnformatted("Drive");
	//ImGui::SameLine();
	//ImGui::Checkbox("##Drive", &isDrive);

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.4f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.8f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0, 1));
	if (ImGui::Button("Create Spherical Joint"))
	{
		phys::cJoint *joint = new phys::cJoint();
		joint->CreateRevolute(g_global->m_physics
			, info0->actor, info0->node->m_transform
			, info1->actor, info1->node->m_transform
			, axis[axisIdx]);

		if (isLimit)
		{
			joint->m_revoluteJoint->setLimit(PxJointAngularLimitPair(limit.x, limit.y, 0.01f));
			joint->m_revoluteJoint->setRevoluteJointFlag(PxRevoluteJointFlag::eLIMIT_ENABLED, true);
		}

		if (isDrive)
		{
			joint->m_revoluteJoint->setDriveVelocity(velocity);
			joint->m_revoluteJoint->setRevoluteJointFlag(PxRevoluteJointFlag::eDRIVE_ENABLED, isDrive);
		}

		physSync->AddJoint(joint);
	}
	ImGui::PopStyleColor(3);
}


void cEditorView::RenderPrismaticJoint()
{

}


void cEditorView::RenderDistanceJoint()
{

}


void cEditorView::RenderD6Joint()
{

}
