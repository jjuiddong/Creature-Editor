
#include "stdafx.h"
#include "editorview.h"

using namespace graphic;

cEditorView::cEditorView(const StrId &name)
	: framework::cDockWindow(name)
	, m_transform(Vector3(0, 3, 0), Vector3(0.5f,0.5f,0.5f))
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
	ImGui::SameLine();
	ImGui::InputInt("DebugType", &renderer.m_dbgRenderStyle);

	//pause/play
	const bool isPause = g_global->m_physics.m_timerScale == 0.f;
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.6f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.1f, 0.9f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.4f, 0, 1));
	if (isPause)
	{
		if (ImGui::Button("Play Simulation"))
			g_global->m_physics.Play();
	}
	else
	{
		if (ImGui::Button("Pause Simulation"))
			g_global->m_physics.Pause();
	}
	ImGui::PopStyleColor(3);

	ImGui::Spacing();
	ImGui::Spacing();

	RenderSpawnTransform();
	RenderSelectionInfo();
	RenderJointInfo();
}


void cEditorView::RenderSpawnTransform()
{
	cRenderer &renderer = g_global->GetRenderer();
	phys::cPhysicsSync *physSync = g_global->m_physSync;

	ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
	if (ImGui::CollapsingHeader("Create RigidBody"))
	{
		//static bool isKinematic = true;
		ImGui::TextUnformatted("Lock ( Kinematic )  ");
		ImGui::SameLine();
		ImGui::Checkbox("##kinematic", &g_global->m_isSpawnLock);

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

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.6f, 0, 1));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.9f, 0, 1));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.4f, 0, 1));

		ImGui::Spacing();
		int syncId = -1;
		if (ImGui::Button("Box"))
		{
			syncId = physSync->SpawnBox(renderer, m_transform, m_density);
		}

		ImGui::SameLine();
		if (ImGui::Button("Sphere"))
		{
			syncId = physSync->SpawnSphere(renderer, m_transform, m_radius, m_density);
		}

		ImGui::SameLine();
		if (ImGui::Button("Capsule"))
		{
			Transform tfm(m_transform.pos, m_transform.rot);
			syncId = physSync->SpawnCapsule(renderer, tfm, m_radius, m_halfHeight, m_density);
		}

		ImGui::PopStyleColor(3);

		if (syncId >= 0)// creation?
		{
			phys::sSyncInfo *sync = physSync->FindSyncInfo(syncId);
			if (sync && sync->actor)
			{
				sync->actor->SetKinematic(g_global->m_isSpawnLock);

				// select spawn rigidactor
				g_global->m_state = eEditState::Normal;
				g_global->ClearSelection();
				g_global->SelectObject(syncId);
				g_global->m_gizmo.SetControlNode(sync->node);
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
	int selSyncId = (g_global->m_selects.size() == 1) ? *g_global->m_selects.begin() : -1;

	ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
	if (ImGui::CollapsingHeader("Select RigidBody"))
	{
		phys::sSyncInfo *sync = physSync->FindSyncInfo(selSyncId);
		if (!sync)
			return;
		if (!sync->actor)
			return;

		phys::cRigidActor *actor = sync->actor;
		graphic::cNode *node = sync->node;
		Transform tfm = node->m_transform;
		float mass = actor->GetMass();
		float linearDamping = actor->GetLinearDamping();
		float angularDamping = actor->GetAngularDamping();
		float maxAngularVelocity = actor->GetMaxAngularVelocity();

		using namespace physx;

		if (actor->m_type == phys::eRigidType::Dynamic)
		{
			bool isKinematic = actor->IsKinematic();
			ImGui::TextUnformatted("Lock ( Kinematic ) ");
			ImGui::SameLine();
			if (ImGui::Checkbox("##Lock ( Kinematic )", &isKinematic))
			{
				if (isKinematic)
				{
					actor->SetKinematic(true);
				}
				else
				{
					// update actor dimension?
					g_global->UpdateActorDimension(actor, isKinematic);
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
		case phys::eShapeType::Box:
			ImGui::TextUnformatted("Dim           ");
			ImGui::SameLine();
			edit1 = ImGui::DragFloat3("##scale2", (float*)&tfm.scale, 0.001f, 0.01f, 1000.f);
			break;
		case phys::eShapeType::Sphere:
			ImGui::TextUnformatted("Radius       ");
			ImGui::SameLine();
			edit1 = ImGui::DragFloat("##scale2", &tfm.scale.x, 0.001f, 0.01f, 1000.f);
			break;
		case phys::eShapeType::Capsule:
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

		ImGui::TextUnformatted("Mass         ");
		ImGui::SameLine();
		const bool edit4 = ImGui::DragFloat("##Mass", &mass, 0.001f, 0.f, 1000.f);

		ImGui::PushItemWidth(140);
		ImGui::TextUnformatted("Linear Damping          ");
		ImGui::SameLine();
		const bool edit5 = ImGui::DragFloat("##Linear Damping", &linearDamping, 0.001f, 0.f, 1000.f);

		ImGui::TextUnformatted("Angular Damping       ");
		ImGui::SameLine();
		const bool edit6 = ImGui::DragFloat("##Angular Damping", &angularDamping, 0.001f, 0.f, 1000.f);

		ImGui::TextUnformatted("Max Angular Velocity  ");
		ImGui::SameLine();
		const bool edit7 = ImGui::DragFloat("##Max Angular Velocity", &maxAngularVelocity, 0.001f, 0.f, 1000.f);
		ImGui::PopItemWidth();

		if (edit0) // position edit
		{
			node->m_transform.pos = tfm.pos;

			PxTransform tm(*(PxVec3*)&tfm.pos, *(PxQuat*)&tfm.rot);
			actor->SetGlobalPose(tm);
		}

		if (edit1 || edit1_1) // scale edit
		{
			actor->SetKinematic(true);

			// RigidActor Scale change was delayed, change when wakeup time
			// store change value
			// change 3d model dimension
			switch (actor->m_shape)
			{
			case phys::eShapeType::Box:
			{
				Transform tm(node->m_transform.pos, tfm.scale, node->m_transform.rot);
				((cCube*)node)->SetCube(tm);
				g_global->ModifyRigidActorTransform(selSyncId, tfm.scale);
			}
			break;
			case phys::eShapeType::Sphere:
			{
				((cSphere*)node)->SetRadius(tfm.scale.x);
				g_global->ModifyRigidActorTransform(selSyncId, tfm.scale);
			}
			break;
			case phys::eShapeType::Capsule:
			{
				((cCapsule*)node)->SetDimension(radius, halfHeight);
				g_global->ModifyRigidActorTransform(selSyncId, Vector3(halfHeight, radius, radius));
			}
			break;
			}
		}

		if (edit2) // rotation edit
		{
			//selectModel->m_transform.rot.Euler2(ryp);
		}

		if (edit4) // mass edit
			actor->SetMass(mass);
		if (edit5) // linear damping edit
			actor->SetLinearDamping(linearDamping);
		if (edit6) // angular damping edit
			actor->SetAngularDamping(angularDamping);
		if (edit7) // max  angular velocity edit
			actor->SetMaxAngularVelocity(maxAngularVelocity);

		ImGui::Separator();
		RenderSelectActorJointInfo(selSyncId);

		ImGui::Spacing();
		ImGui::Spacing();
	}//~CollapsingHeader
}


// show select actor joint information
void cEditorView::RenderSelectActorJointInfo(const int syncId)
{
	using namespace graphic;
	cRenderer &renderer = g_global->GetRenderer();

	phys::cPhysicsSync *physSync = g_global->m_physSync;
	phys::sSyncInfo *sync = physSync->FindSyncInfo(syncId);
	if (!sync)
		return;
	if (!sync->actor)
		return;

	phys::cRigidActor *actor = sync->actor;
	graphic::cNode *node = sync->node;

	set<phys::cJoint*> rmJoints; // remove joints

	// show all joint information contained rigid actor
	ImGui::TextUnformatted("Actor Joint Information");
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
				cJointRenderer *jointRenderer = g_global->FindJointRenderer(joint);
				if (jointRenderer)
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
		//g_global->m_gizmo.SetControlNode(nullptr);
		g_global->m_selJoint = nullptr;
		//g_global->ClearSelection();
	}
	for (auto &joint : rmJoints)
		physSync->RemoveSyncInfo(joint);
}


void cEditorView::RenderJointInfo()
{
	ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
	if (ImGui::CollapsingHeader("Create Joint"))
	{
		CheckCancelUIJoint();

		if (g_global->m_selects.size() == 2)
		{
			// check already connection
			{
				auto it = g_global->m_selects.begin();
				const int syncId0 = *it++;
				const int syncId1 = *it++;

				phys::cPhysicsSync *physSync = g_global->m_physSync;
				phys::sSyncInfo *sync0 = physSync->FindSyncInfo(syncId0);
				phys::sSyncInfo *sync1 = physSync->FindSyncInfo(syncId1);
				if (!sync0 || !sync1)
					return;
				if (!sync0->actor || !sync1->actor)
					return;
				for (auto &j1 : sync0->actor->m_joints)
					for (auto &j2 : sync1->actor->m_joints)
						if (j1 == j2)
							return; // already connection joint
			}

			const char *jointType = "Fixed\0Spherical\0Revolute\0Prismatic\0Distance\0D6\0\0";
			static int idx = 0;
			ImGui::TextUnformatted("Joint");
			ImGui::SameLine();
			ImGui::Combo("##joint type", &idx, jointType);
			ImGui::Separator();

			switch ((phys::eJointType::Enum)idx)
			{
			case phys::eJointType::Fixed: RenderFixedJoint(); break;
			case phys::eJointType::Spherical: RenderSphericalJoint(); break;
			case phys::eJointType::Revolute: RenderRevoluteJoint(); break;
			case phys::eJointType::Prismatic: RenderPrismaticJoint(); break;
			case phys::eJointType::Distance: RenderDistanceJoint(); break;
			case phys::eJointType::D6: RenderD6Joint(); break;
			}
		}
	}
}


void cEditorView::RenderFixedJoint()
{
	auto it = g_global->m_selects.begin();
	const int syncId0 = *it++;
	const int syncId1 = *it++;

	phys::cPhysicsSync *physSync = g_global->m_physSync;
	phys::sSyncInfo *sync0 = physSync->FindSyncInfo(syncId0);
	phys::sSyncInfo *sync1 = physSync->FindSyncInfo(syncId1);
	if (!sync0 || !sync1)
		return;

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.4f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.8f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0, 1));
	if (ImGui::Button("Create Fixed Joint"))
	{
		g_global->UpdateActorDimension(sync0->actor, true);
		g_global->UpdateActorDimension(sync1->actor, true);

		phys::cJoint *joint = new phys::cJoint();
		joint->CreateFixed(g_global->m_physics
			, sync0->actor, sync0->node->m_transform
			, sync1->actor, sync1->node->m_transform);

		cJointRenderer *jointRenderer = new cJointRenderer();
		jointRenderer->Create(joint);
		physSync->AddJoint(joint, jointRenderer);
	}
	ImGui::PopStyleColor(3);
}


void cEditorView::RenderSphericalJoint()
{
	using namespace physx;

	auto it = g_global->m_selects.begin();
	const int syncId0 = *it++;
	const int syncId1 = *it++;

	phys::cPhysicsSync *physSync = g_global->m_physSync;
	phys::sSyncInfo *sync0 = physSync->FindSyncInfo(syncId0);
	phys::sSyncInfo *sync1 = physSync->FindSyncInfo(syncId1);
	if (!sync0 || !sync1)
		return;

	static Vector2 limit(PxPi / 2.f, PxPi / 6.f);
	static bool isLimit;
	ImGui::Text("Limit Cone (Radian)");
	//ImGui::TextUnformatted("Limit");
	ImGui::SameLine();
	ImGui::Checkbox("##Limit", &isLimit);
	ImGui::DragFloat("Y Limit Angle", &limit.x, 0.001f);
	ImGui::DragFloat("Z Limit Angle", &limit.y, 0.001f);	

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.4f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.8f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0, 1));
	if (ImGui::Button("Create Spherical Joint"))
	{
		g_global->UpdateActorDimension(sync0->actor, true);
		g_global->UpdateActorDimension(sync1->actor, true);

		phys::cJoint *joint = new phys::cJoint();
		joint->CreateSpherical(g_global->m_physics
			, sync0->actor, sync0->node->m_transform
			, sync1->actor, sync1->node->m_transform);

		if (isLimit)
		{
			joint->SetLimitCone(PxJointLimitCone(limit.x, limit.y, 0.01f));
			joint->SetSphericalJointFlag(PxSphericalJointFlag::eLIMIT_ENABLED, true);
		}

		cJointRenderer *jointRenderer = new cJointRenderer();
		jointRenderer->Create(joint);
		physSync->AddJoint(joint, jointRenderer);
	}
	ImGui::PopStyleColor(3);
}


void cEditorView::RenderRevoluteJoint()
{
	using namespace physx;

	auto it = g_global->m_selects.begin();
	const int syncId0 = *it++;
	const int syncId1 = *it++;

	phys::cPhysicsSync *physSync = g_global->m_physSync;
	phys::sSyncInfo *sync0 = physSync->FindSyncInfo(syncId0);
	phys::sSyncInfo *sync1 = physSync->FindSyncInfo(syncId1);
	if (!sync0 || !sync1)
		return;

	static Vector2 limit(-PxPi / 4.f, PxPi / 4.f);
	static bool isDrive = false;
	static float velocity = 1.f;
	static bool isLimit = false;

	ImGui::Text("Angular Limit (Radian)");
	ImGui::SameLine();
	ImGui::Checkbox("##Limit", &isLimit);
	ImGui::DragFloat("Lower Limit Angle", &limit.x, 0.001f);
	ImGui::DragFloat("Upper Limit Angle", &limit.y, 0.001f);

	ImGui::TextUnformatted("Drive");
	ImGui::SameLine();
	ImGui::Checkbox("##Drive", &isDrive);
	ImGui::DragFloat("Velocity", &velocity, 0.001f);

	const char *axisStr = "X\0Y\0Z\0\0";
	const static Vector3 axis[3] = { Vector3(1,0,0), Vector3(0,1,0), Vector3(0,0,1) };
	static int axisIdx = 0;
	const bool editAxis = ImGui::Combo("Revolute Axis", &axisIdx, axisStr);

	if (ImGui::Button("Pivot Setting"))
	{
		g_global->m_state = eEditState::Pivot0;
		g_global->m_selJoint = &g_global->m_uiJoint;
		g_global->m_gizmo.m_type = graphic::eGizmoEditType::None;
	}

	// update ui joint (once process)
	if (!g_global->m_showUIJoint || editAxis)
	{
		if (!g_global->FindJointRenderer(&g_global->m_uiJoint))
			physSync->AddJoint(&g_global->m_uiJoint, &g_global->m_uiJointRenderer, false);

		const bool isPrevSelection = (((g_global->m_uiJoint.m_actor0 == sync0->actor)
				&& (g_global->m_uiJoint.m_actor1 == sync1->actor))
			|| ((g_global->m_uiJoint.m_actor1 == sync0->actor)
				&& (g_global->m_uiJoint.m_actor0 == sync1->actor)));

		g_global->m_showUIJoint = true;

		if (!isPrevSelection || editAxis) // new selection?
		{
			g_global->m_uiJoint.m_type = phys::eJointType::Revolute;
			g_global->m_uiJoint.m_actor0 = sync0->actor;
			g_global->m_uiJoint.m_actor1 = sync1->actor;
			g_global->m_uiJoint.m_actorLocal0 = sync0->node->m_transform;
			g_global->m_uiJoint.m_actorLocal1 = sync1->node->m_transform;
			g_global->m_uiJoint.m_revoluteAxis = axis[axisIdx];
			g_global->m_uiJoint.m_pivots[0].dir = Vector3::Zeroes;
			g_global->m_uiJoint.m_pivots[0].len = 0;
			g_global->m_uiJoint.m_pivots[1].dir = Vector3::Zeroes;
			g_global->m_uiJoint.m_pivots[1].len = 0;

			g_global->m_uiJointRenderer.m_joint = &g_global->m_uiJoint;
			g_global->m_uiJointRenderer.m_sync0 = sync0;
			g_global->m_uiJointRenderer.m_sync1 = sync1;
			const Vector3 jointPos = (g_global->m_uiJointRenderer.GetPivotWorldTransform(0).pos +
				g_global->m_uiJointRenderer.GetPivotWorldTransform(1).pos) / 2.f;
			g_global->m_uiJointRenderer.SetRevoluteAxis(axis[axisIdx], jointPos);
		}
	}
	//~

	ImGui::Spacing();
	ImGui::Spacing();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.4f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.8f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0, 1));
	if (ImGui::Button("Create Revolute Joint"))
	{
		g_global->UpdateActorDimension(sync0->actor, true);
		g_global->UpdateActorDimension(sync1->actor, true);

		const Transform pivot0 = g_global->m_uiJointRenderer.GetPivotWorldTransform(0);
		const Transform pivot1 = g_global->m_uiJointRenderer.GetPivotWorldTransform(1);

		phys::cJoint *joint = new phys::cJoint();
		joint->CreateRevolute(g_global->m_physics
			, sync0->actor, sync0->node->m_transform, pivot0.pos
			, sync1->actor, sync1->node->m_transform, pivot1.pos
			, axis[axisIdx]);

		if (isLimit)
		{
			joint->SetLimitAngular(PxJointAngularLimitPair(limit.x, limit.y, 0.01f));
			joint->SetRevoluteJointFlag(PxRevoluteJointFlag::eLIMIT_ENABLED, true);
		}

		if (isDrive)
		{
			joint->SetDriveVelocity(velocity);
			joint->SetRevoluteJointFlag(PxRevoluteJointFlag::eDRIVE_ENABLED, isDrive);
		}

		cJointRenderer *jointRenderer = new cJointRenderer();
		jointRenderer->Create(joint);
		physSync->AddJoint(joint, jointRenderer);

		g_global->m_state = eEditState::Normal;
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
	static bool isLimit = false;

	ImGui::Text("Angular Limit (Radian)");
	ImGui::SameLine();
	ImGui::Checkbox("##Limit", &isLimit);
	ImGui::DragFloat("Lower Limit Angle", &limit.x, 0.001f);
	ImGui::DragFloat("Upper Limit Angle", &limit.y, 0.001f);
	ImGui::TextUnformatted("Drive");
	ImGui::SameLine();
	ImGui::Checkbox("##Drive", &isDrive);
	ImGui::DragFloat("Velocity", &velocity, 0.001f);

	const char *axisStr = "X\0Y\0Z\0\0";
	const static Vector3 axis[3] = { Vector3(1,0,0), Vector3(0,1,0), Vector3(0,0,1) };
	static int axisIdx = 0;
	ImGui::Combo("Revolute Axis", &axisIdx, axisStr);

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.4f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.8f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0, 1));
	if (ImGui::Button("Apply Option"))
	{
		joint->SetRevoluteJointFlag(PxRevoluteJointFlag::eLIMIT_ENABLED, isLimit);
		if (isLimit)
			joint->SetLimitAngular(PxJointAngularLimitPair(limit.x, limit.y, 0.01f));

		joint->SetRevoluteJointFlag(PxRevoluteJointFlag::eDRIVE_ENABLED, isDrive);
		if (isDrive)
			joint->SetDriveVelocity(velocity);

		joint->m_actor0->WakeUp();
		joint->m_actor1->WakeUp();
	}
	ImGui::PopStyleColor(3);
}


// check cancel show ui joint
void cEditorView::CheckCancelUIJoint()
{
	if (!g_global->m_showUIJoint)
		return;

	bool isCancelUIJoint = false;

	if ((g_global->m_selects.size() != 1)
		&& (g_global->m_selects.size() != 2)
		&& (g_global->m_selects.size() != 3))
	{
		goto $cancel;
	}
	else if (g_global->m_selects.size() == 1)
	{
		phys::sSyncInfo *sync0 = g_global->FindSyncInfo(g_global->m_selects[0]);
		if (!sync0->joint)
			goto $cancel;
	}
	else
	{
		// check rigidactor x 2
		if (g_global->m_selects.size() == 2)
		{
			phys::sSyncInfo *sync0 = g_global->FindSyncInfo(g_global->m_selects[0]);
			phys::sSyncInfo *sync1 = g_global->FindSyncInfo(g_global->m_selects[1]);
			if (!sync0 || !sync1)
				goto $cancel;
			if (sync0->joint || sync1->joint)
				goto $cancel;
		}
		else if (g_global->m_selects.size() == 3)
		{
			// check joint link rigid actor x 2
			phys::sSyncInfo *syncs[3] = { nullptr, nullptr, nullptr };
			syncs[0] = g_global->FindSyncInfo(g_global->m_selects[0]);
			syncs[1] = g_global->FindSyncInfo(g_global->m_selects[1]);
			syncs[2] = g_global->FindSyncInfo(g_global->m_selects[2]);
			if (!syncs[0] || !syncs[1] || !syncs[2])
				goto $cancel;

			int jointIdx = -1;
			for (int i = 0; i < 3; ++i)
				if (syncs[i]->joint)
					jointIdx = i;
				
			if (jointIdx < 0)
				goto $cancel;

			int i0 = (jointIdx + 1) % 3;
			int i1 = (jointIdx + 2) % 3;
			if (((syncs[jointIdx]->joint->m_actor0 == syncs[i0]->actor)
				&& (syncs[jointIdx]->joint->m_actor1 == syncs[i1]->actor))
				|| ((syncs[jointIdx]->joint->m_actor1 == syncs[i0]->actor)
					&& (syncs[jointIdx]->joint->m_actor0 == syncs[i1]->actor)))
			{
				// ok
			}
			else
			{
				goto $cancel;
			}
		}
	}

	return;

$cancel:
	g_global->m_showUIJoint = false;
	g_global->m_physSync->RemoveSyncInfo(&g_global->m_uiJoint);
	return;
}
