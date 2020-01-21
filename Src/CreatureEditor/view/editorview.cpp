
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

	//initialize ui
	g_global->m_showUIJoint = false;
	//~

	RenderSpawnTransform();
	RenderSelectionInfo();
	RenderJointInfo();
}


void cEditorView::RenderSpawnTransform()
{
	cRenderer &renderer = g_global->GetRenderer();
	phys::cPhysicsSync *physSync = g_global->m_physSync;

	ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
	if (ImGui::CollapsingHeader("Spawn RigidBody"))
	{
		static bool isKinematic = true;
		ImGui::TextUnformatted("Lock ( Kinematic )  ");
		ImGui::SameLine();
		ImGui::Checkbox("##kinematic", &isKinematic);

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
			phys::sActorInfo *info = physSync->FindActorInfo(id);
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
	if (ImGui::CollapsingHeader("Selection RigidBody"))
	{
		phys::sActorInfo *info = physSync->FindActorInfo(selId);
		if (!info)
			return;

		phys::cRigidActor *actor = info->actor;
		graphic::cNode *node = info->node;
		Transform tfm = node->m_transform;

		using namespace physx;

		if (actor->m_dynamic)
		{
			bool isKinematic = actor->IsKinematic();
			ImGui::TextUnformatted("Lock ( Kinematic ) ");
			ImGui::SameLine();
			if (ImGui::Checkbox("##Lock ( Kinematic )", &isKinematic))
			{
				actor->SetKinematic(isKinematic);
				if (!isKinematic)
				{
					// is change dimension?
					// apply modify dimension
					Vector3 dim;
					if (g_global->GetModifyRigidActorTransform(selId, dim))
					{
						// apply physics shape
						actor->ChangeDimension(g_global->m_physics, dim);
						g_global->RemoveModifyRigidActorTransform(selId);
					}
					actor->m_dynamic->wakeUp();
				}
			}
		}

		// edit position
		ImGui::TextUnformatted("Pos            ");
		ImGui::SameLine();
		const bool edit0 = ImGui::DragFloat3("##position2", (float*)&tfm.pos, 0.001f);

		// edit scale
		bool edit1 = false;
		bool edit1_1 = false;
		float radius = 0.f, halfHeight = 0.f; // capsule info
		switch (actor->m_shape)
		{
		case phys::cRigidActor::eShape::Box:
			ImGui::TextUnformatted("Dim           ");
			ImGui::SameLine();
			edit1 = ImGui::DragFloat3("##scale2", (float*)&tfm.scale, 0.001f, 0.01f, 1000.f);
			break;
		case phys::cRigidActor::eShape::Sphere:
			ImGui::TextUnformatted("Radius       ");
			ImGui::SameLine();
			edit1 = ImGui::DragFloat("##scale2", &tfm.scale.x, 0.001f, 0.01f, 1000.f);
			break;
		case phys::cRigidActor::eShape::Capsule:
			radius = ((cCapsule*)node)->m_radius;
			halfHeight = ((cCapsule*)node)->m_halfHeight;
			ImGui::TextUnformatted("Radius       ");
			ImGui::SameLine();
			edit1 = ImGui::DragFloat("##scale2", &radius, 0.001f, 0.01f, 1000.f);
			ImGui::TextUnformatted("HalfHeight");
			ImGui::SameLine();
			edit1_1 = ImGui::DragFloat("##halfheight2", &halfHeight, 0.001f, 0.01f, 1000.f);
			break;
		}

		// edit rotation
		ImGui::TextUnformatted("Rot            ");
		ImGui::SameLine();
		static Vector3 ryp;//roll yaw pitch
		const bool edit2 = ImGui::DragFloat3("##rotation2", (float*)&ryp, 0.001f);

		if (edit0) // position edit
		{
			node->m_transform.pos = tfm.pos;

			PxTransform tm(*(PxVec3*)&tfm.pos, *(PxQuat*)&tfm.rot);
			actor->m_dynamic->setGlobalPose(tm);
		}

		if (edit1 || edit1_1) // scale edit
		{
			actor->SetKinematic(true);

			// RigidActor Scale change was delayed, change when wakeup time
			// store change value
			// change 3d model dimension
			switch (actor->m_shape)
			{
			case phys::cRigidActor::eShape::Box:
			{
				Transform tm(node->m_transform.pos, tfm.scale, node->m_transform.rot);
				((cCube*)node)->SetCube(tm);
				g_global->ModifyRigidActorTransform(selId, tfm.scale);
			}
			break;
			case phys::cRigidActor::eShape::Sphere:
			{
				((cSphere*)node)->SetRadius(tfm.scale.x);
				g_global->ModifyRigidActorTransform(selId, tfm.scale);
			}
			break;
			case phys::cRigidActor::eShape::Capsule:
			{
				((cCapsule*)node)->SetDimension(radius, halfHeight);
				g_global->ModifyRigidActorTransform(selId, Vector3(halfHeight, radius, radius));
			}
			break;
			}
		}

		if (edit2) // rotation edit
		{
			//selectModel->m_transform.rot.Euler2(ryp);
		}

		ImGui::Separator();
		RenderSelectActorJointInfo(selId);

		ImGui::Spacing();
		ImGui::Spacing();
	}//~CollapsingHeader
}


// show select actor joint information
void cEditorView::RenderSelectActorJointInfo(const int actorId)
{
	using namespace graphic;
	cRenderer &renderer = g_global->GetRenderer();

	phys::cPhysicsSync *physSync = g_global->m_physSync;
	phys::sActorInfo *info = physSync->FindActorInfo(actorId);
	if (!info)
		return;

	phys::cRigidActor *actor = info->actor;
	graphic::cNode *node = info->node;

	set<phys::cJoint*> rmJoints; // remove joints

	// show all joint information contained rigid actor
	ImGui::TextUnformatted("Actor RigidBody Joint Information");
	for (uint i=0; i < actor->m_joints.size(); ++i)
	{
		phys::cJoint *joint = actor->m_joints[i];

		const ImGuiTreeNodeFlags node_flags = 0;
		ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
		if (ImGui::TreeNodeEx((void*)((int)actor + i), node_flags, "Joint-%d", i + 1))
		{
			const char *jointStr[] = { "Fixed", "Spherical", "Revolute", "Prismatic", "Distance", "D6" };
			ImGui::Text("%s JointType", jointStr[(int)joint->m_type]);

			if (ImGui::Button("Pivot Setting"))
			{
				g_global->m_state = eEditState::Pivot0;
				g_global->m_selJoint = joint;
				g_global->m_gizmo.m_type = graphic::eGizmoEditType::None;
			}

			ImGui::SameLine(150);
			if (ImGui::Button("Apply Pivot"))
			{
				if (cJointRenderer *jointRenderer = g_global->FindJointRenderer(joint))
					jointRenderer->ApplyPivot();
				g_global->m_state = eEditState::Normal;
			}

			ImGui::Separator();
			RenderRevoluteJointSetting(joint);

			ImGui::Spacing();
			ImGui::Spacing();

			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.1f, 0, 1));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.1f, 0, 1));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.1f, 0, 1));
			StrId text;
			text.Format("Remove Joint-%d", i + 1);
			if (ImGui::Button(text.c_str()))
			{
				rmJoints.insert(joint);
			}
			ImGui::PopStyleColor(3);

			ImGui::TreePop();
		}
	}

	// remove joint?
	if (!rmJoints.empty())
	{
		// clear selection
		g_global->m_state = eEditState::Normal;
		g_global->m_gizmo.SetControlNode(nullptr);
		g_global->m_selJoint = nullptr;
		g_global->ClearSelection();
	}
	for (auto &joint : rmJoints)
		g_global->RemoveJoint(joint);
}


void cEditorView::RenderJointInfo()
{
	ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
	if (ImGui::CollapsingHeader("Joint Creation"))
	{
		if (g_global->m_selects.size() == 2)
		{
			// check already connection
			{
				auto it = g_global->m_selects.begin();
				const int actorId0 = *it++;
				const int actorId1 = *it++;

				phys::cPhysicsSync *physSync = g_global->m_physSync;
				phys::sActorInfo *info0 = physSync->FindActorInfo(actorId0);
				phys::sActorInfo *info1 = physSync->FindActorInfo(actorId1);
				if (!info0 || !info1)
					return;

				for (auto &joint : physSync->m_joints)
				{
					if (((joint->m_actor0 == info0->actor) && (joint->m_actor1 == info1->actor))
						|| ((joint->m_actor1 == info0->actor) && (joint->m_actor0 == info1->actor))
						)
						return; // already connection joint
				}
			}

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
	phys::sActorInfo *info0 = physSync->FindActorInfo(actorId0);
	phys::sActorInfo *info1 = physSync->FindActorInfo(actorId1);
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
		g_global->AddJoint(joint);
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
	phys::sActorInfo *info0 = physSync->FindActorInfo(actorId0);
	phys::sActorInfo *info1 = physSync->FindActorInfo(actorId1);
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

		g_global->AddJoint(joint);
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
	phys::sActorInfo *info0 = physSync->FindActorInfo(actorId0);
	phys::sActorInfo *info1 = physSync->FindActorInfo(actorId1);
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

	if (ImGui::Button("Pivot Setting"))
	{
		g_global->m_state = eEditState::Pivot0;
		g_global->m_selJoint = &g_global->m_uiJoint;
		g_global->m_gizmo.m_type = graphic::eGizmoEditType::None;
	}

	// update ui joint
	g_global->m_showUIJoint = true;
	g_global->m_uiJoint.m_type = phys::cJoint::eType::Revolute;
	g_global->m_uiJoint.m_actor0 = info0->actor;
	g_global->m_uiJoint.m_actor1 = info1->actor;
	g_global->m_uiJoint.m_revoluteAxis = axis[axisIdx];
	g_global->m_uiJointRenderer.m_joint = &g_global->m_uiJoint;
	g_global->m_uiJointRenderer.m_info0 = info0;
	g_global->m_uiJointRenderer.m_info1 = info1;
	//~

	ImGui::Spacing();
	ImGui::Spacing();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.4f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.8f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0, 1));
	if (ImGui::Button("Create Spherical Joint"))
	{
		const Transform pivot0 = g_global->m_uiJointRenderer.GetPivotWorldTransform(0);
		const Transform pivot1 = g_global->m_uiJointRenderer.GetPivotWorldTransform(1);

		phys::cJoint *joint = new phys::cJoint();
		joint->CreateRevolute(g_global->m_physics
			, info0->actor, info0->node->m_transform, pivot0.pos
			, info1->actor, info1->node->m_transform, pivot1.pos
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

		g_global->AddJoint(joint);
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


void cEditorView::RenderRevoluteJointSetting(phys::cJoint *joint)
{
	using namespace physx;

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

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.4f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.8f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0, 1));
	if (ImGui::Button("Apply Option"))
	{
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
	}
	ImGui::PopStyleColor(3);
}
