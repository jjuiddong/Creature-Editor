
#include "stdafx.h"
#include "phenoeditorview.h"
#include "../creature/creature.h"

using namespace graphic;

cPhenoEditorView::cPhenoEditorView(const StrId &name)
	: framework::cDockWindow(name)
	, m_radius(0.5f)
	, m_halfHeight(1.f)
	, m_density(1.f)
	, m_isChangeSelection(false)
{
}

cPhenoEditorView::~cPhenoEditorView()
{
}


void cPhenoEditorView::OnUpdate(const float deltaSeconds)
{
}


void cPhenoEditorView::OnRender(const float deltaSeconds)
{
	cRenderer &renderer = g_pheno->GetRenderer();
	phys::cPhysicsSync *physSync = g_pheno->m_physSync;

	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 0, 1));
	ImGui::TextUnformatted("PhenoType Editor");
	ImGui::PopStyleColor();
	ImGui::Separator();

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
			g_pheno->m_physics->Play();
	}
	else
	{
		if (ImGui::Button("Pause Simulation"))
			g_pheno->m_physics->Pause();
	}
	ImGui::PopStyleColor(3);

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.1f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.1f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.1f, 0, 1));
	ImGui::SameLine(250);
	if (ImGui::Button("Clear"))
	{
		g_nn->SetCurrentCreature(nullptr);
		g_pheno->ClearCreature();
	}
	ImGui::PopStyleColor(3);

	ImGui::Spacing();
	ImGui::Spacing();

	CheckChangeSelection();

	RenderSpawnTransform();
	RenderSelectionInfo();
	RenderJointInfo();

	// final process, recovery
	m_isChangeSelection = false;
}


void cPhenoEditorView::RenderSpawnTransform()
{
	cRenderer &renderer = g_pheno->GetRenderer();
	phys::cPhysicsSync *physSync = g_pheno->m_physSync;

	ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
	if (ImGui::CollapsingHeader("Create RigidBody"))
	{
		ImGui::TextUnformatted("Lock ( Kinematic )  ");
		ImGui::SameLine();
		ImGui::Checkbox("##kinematic", &g_pheno->m_isSpawnLock);
		ImGui::SameLine(235);
		if (ImGui::Button("SpawnPos"))
			g_pheno->ChangeEditMode(ePhenoEditMode::SpawnLocation);

		ImGui::TextUnformatted("Pos  ");
		ImGui::SameLine();
		ImGui::DragFloat3("##position", (float*)&g_pheno->m_spawnTransform.pos, 0.001f);

		ImGui::TextUnformatted("Dim ");
		ImGui::SameLine();
		ImGui::DragFloat3("##scale", (float*)&g_pheno->m_spawnTransform.scale, 0.001f, 0.01f, 1000.f);

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

		ImGui::TextUnformatted("Generation");
		ImGui::SameLine();
		ImGui::InputInt("##generation", &g_pheno->m_generationCnt, 1, 1);

		ImGui::PushItemWidth(180);
		ImGui::TextUnformatted("NeuralNet Layer");
		ImGui::SameLine();
		ImGui::InputInt("##NeuralNet Layer", &g_pheno->m_nnLayerCnt, 1, 1);
		ImGui::PopItemWidth();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.6f, 0, 1));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.9f, 0, 1));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.4f, 0, 1));

		ImGui::Spacing();
		int syncId = -1;
		if (ImGui::Button("Box"))
		{
			syncId = physSync->SpawnBox(renderer, g_pheno->m_spawnTransform, m_density);
		}

		ImGui::SameLine();
		if (ImGui::Button("Sphere"))
		{
			syncId = physSync->SpawnSphere(renderer, g_pheno->m_spawnTransform, m_radius, m_density);
		}

		ImGui::SameLine();
		if (ImGui::Button("Capsule"))
		{
			Transform tfm(g_pheno->m_spawnTransform.pos, g_pheno->m_spawnTransform.rot);
			syncId = physSync->SpawnCapsule(renderer, tfm, m_radius, m_halfHeight, m_density);
		}

		ImGui::SameLine();
		if (ImGui::Button("Cylinder"))
		{
			Transform tfm(g_pheno->m_spawnTransform.pos, g_pheno->m_spawnTransform.rot);
			syncId = physSync->SpawnCylinder(renderer, tfm, m_radius, m_halfHeight, m_density);
		}

		ImGui::PopStyleColor(3);

		if (syncId >= 0)// creation?
			g_pheno->SetSelectionAndKinematic(syncId);

		ImGui::Spacing();
		ImGui::Spacing();
	}
}


void cPhenoEditorView::RenderSelectionInfo()
{
	using namespace graphic;
	cRenderer &renderer = g_pheno->GetRenderer();
	phys::cPhysicsSync *physSync = g_pheno->m_physSync;
	const int selSyncId = (g_pheno->m_selects.size() == 1) ? *g_pheno->m_selects.begin() : -1;

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

		if (m_isChangeSelection)
		{
			const Vector3 rpy = tfm.rot.Euler();
			m_eulerAngle = Vector3(RAD2ANGLE(rpy.x), RAD2ANGLE(rpy.y), RAD2ANGLE(rpy.z));
		}

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
					g_pheno->UpdateActorDimension(actor, isKinematic);
				}
			}
		}

		if (evc::cCreature *creature = g_pheno->FindCreatureContainNode(selSyncId))
		{
			ImGui::TextUnformatted("Neural Network     ");
			ImGui::SameLine();
			ImGui::Checkbox("##Neural Network", &creature->m_isNN);
		}

		// edit position
		ImGui::TextUnformatted("Pos            ");
		ImGui::SameLine();
		const bool edit0 = ImGui::DragFloat3("##position2", (float*)&tfm.pos, 0.001f);

		// edit scale
		bool edit1 = false;
		bool edit1_1 = false;
		float radius = 0.f, halfHeight = 0.f, height = 0.f; // capsule,cylinder info
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
		case phys::eShapeType::Cylinder:
			radius = ((cCylinder*)node)->m_radius;
			height = ((cCylinder*)node)->m_height;
			ImGui::TextUnformatted("Radius       ");
			ImGui::SameLine();
			edit1 = ImGui::DragFloat("##scale2", &radius, 0.001f, 0.01f, 1000.f);
			ImGui::TextUnformatted("Height       ");
			ImGui::SameLine();
			edit1_1 = ImGui::DragFloat("##height2", &height, 0.001f, 0.01f, 1000.f);
			break;
		}

		// edit rotation
		ImGui::TextUnformatted("Rot            ");
		ImGui::SameLine();
		const bool edit2 = ImGui::DragFloat3("##rotation2", (float*)&m_eulerAngle, 0.1f);

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
			tfm.pos.y = max(0.f, tfm.pos.y);
			node->m_transform.pos = tfm.pos;
			actor->SetGlobalPose(tfm);
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
				g_pheno->ModifyRigidActorTransform(selSyncId, tfm.scale);
			}
			break;
			case phys::eShapeType::Sphere:
			{
				((cSphere*)node)->SetRadius(tfm.scale.x);
				g_pheno->ModifyRigidActorTransform(selSyncId, tfm.scale);
			}
			break;
			case phys::eShapeType::Capsule:
			{
				((cCapsule*)node)->SetDimension(radius, halfHeight);
				g_pheno->ModifyRigidActorTransform(selSyncId, Vector3(halfHeight, radius, radius));
			}
			break;
			case phys::eShapeType::Cylinder:
			{
				((cCylinder*)node)->SetDimension(radius, height);
				g_pheno->ModifyRigidActorTransform(selSyncId, Vector3(height, radius, radius));
			}
			break;
			}
		}

		if (edit2) // rotation edit
		{
			const Vector3 rpy(ANGLE2RAD(m_eulerAngle.x)
				, ANGLE2RAD(m_eulerAngle.y), ANGLE2RAD(m_eulerAngle.z));
			tfm.rot.Euler(rpy);
			node->m_transform.rot = tfm.rot;
			actor->SetGlobalPose(tfm);
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
void cPhenoEditorView::RenderSelectActorJointInfo(const int syncId)
{
	using namespace graphic;
	cRenderer &renderer = g_pheno->GetRenderer();

	phys::cPhysicsSync *physSync = g_pheno->m_physSync;
	phys::sSyncInfo *sync = physSync->FindSyncInfo(syncId);
	if (!sync)
		return;
	if (!sync->actor)
		return;

	phys::cRigidActor *actor = sync->actor;
	graphic::cNode *node = sync->node;

	if (actor->m_joints.size() != m_jointInfos.size())
		return; // error occurred!!

	set<phys::cJoint*> rmJoints; // remove joints

	// show all joint information contained rigid actor
	ImGui::TextUnformatted("Actor Joint Information");
	for (uint i=0; i < actor->m_joints.size(); ++i)
	{
		phys::cJoint *joint = actor->m_joints[i];
		evc::sGenotypeLink &info = m_jointInfos[i];

		const ImGuiTreeNodeFlags node_flags = 0;
		ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
		if (ImGui::TreeNodeEx((void*)((int)actor + i), node_flags, "Joint-%d", i + 1))
		{
			const char *jointStr[] = { "Fixed", "Spherical", "Revolute", "Prismatic", "Distance", "D6", "Compound" };
			ImGui::Text("%s JointType", jointStr[(int)joint->m_type]);

			if (ImGui::Button("Pivot Setting"))
			{
				g_pheno->ChangeEditMode(ePhenoEditMode::Pivot0);
				g_pheno->m_selJoint = joint;
				g_pheno->m_gizmo.m_type = graphic::eGizmoEditType::None;
			}

			ImGui::SameLine(150);
			if (ImGui::Button("Apply Pivot"))
			{
				cJointRenderer *jointRenderer = g_pheno->FindJointRenderer(joint);
				if (jointRenderer)
					jointRenderer->ApplyPivot(*g_pheno->m_physics);

				g_pheno->ChangeEditMode(ePhenoEditMode::Normal);
			}

			switch (joint->m_type)
			{
			case phys::eJointType::Fixed: break;
			case phys::eJointType::Spherical: RenderSphericalJointSetting(joint, info); break;
			case phys::eJointType::Revolute: RenderRevoluteJointSetting(joint, info); break;
			case phys::eJointType::Prismatic: RenderPrismaticJointSetting(joint, info); break;
			case phys::eJointType::Distance: RenderDistanceJointSetting(joint, info); break;
			case phys::eJointType::D6: RenderD6JointSetting(joint, info); break;
			case phys::eJointType::Compound: RenderCompoundSetting(joint, info); break;
			default:
				assert(0);
				break;
			}

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

			ImGui::Separator();
			ImGui::TreePop();
		}
	}

	// remove joint?
	if (!rmJoints.empty())
	{
		// clear selection
		g_pheno->ChangeEditMode(ePhenoEditMode::Normal);
		g_pheno->m_selJoint = nullptr;
	}
	for (auto &joint : rmJoints)
		physSync->RemoveSyncInfo(joint);
}


void cPhenoEditorView::RenderJointInfo()
{
	ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
	if (ImGui::CollapsingHeader("Create Joint"))
	{
		if (CheckCancelUIJoint())
			g_pheno->ChangeEditMode(ePhenoEditMode::Normal); // cancel ui

		if (g_pheno->m_selects.size() == 2)
		{
			// check already connection
			{
				auto it = g_pheno->m_selects.begin();
				const int syncId0 = *it++;
				const int syncId1 = *it++;

				phys::cPhysicsSync *physSync = g_pheno->m_physSync;
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

			if ((g_pheno->GetEditMode() != ePhenoEditMode::Pivot0)
				&& (g_pheno->GetEditMode() != ePhenoEditMode::Pivot1)
				&& (g_pheno->GetEditMode() != ePhenoEditMode::Revolute)
				&& (g_pheno->GetEditMode() != ePhenoEditMode::JointEdit))
			{
				auto it = g_pheno->m_selects.begin();
				g_pheno->m_pairSyncId0 = *it++;
				g_pheno->m_pairSyncId1 = *it++;
				g_pheno->ChangeEditMode(ePhenoEditMode::JointEdit); // joint edit mode
			}
		}

		if ((g_pheno->GetEditMode() == ePhenoEditMode::Pivot0)
			|| (g_pheno->GetEditMode() == ePhenoEditMode::Pivot1)
			|| (g_pheno->GetEditMode() == ePhenoEditMode::Revolute)
			|| (g_pheno->GetEditMode() == ePhenoEditMode::JointEdit))
		{
			ImGui::Checkbox("Fix Selection", &g_pheno->m_fixJointSelection);

			const char *jointType = "Fixed\0Spherical\0Revolute\0Prismatic\0Distance\0D6\0Compound\0\0";
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
			case phys::eJointType::Compound: RenderCompound(); break;
			}
		}
	}
}


void cPhenoEditorView::RenderFixedJoint()
{
	const int syncId0 = g_pheno->m_pairSyncId0;
	const int syncId1 = g_pheno->m_pairSyncId1;

	phys::cPhysicsSync *physSync = g_pheno->m_physSync;
	phys::sSyncInfo *sync0 = physSync->FindSyncInfo(syncId0);
	phys::sSyncInfo *sync1 = physSync->FindSyncInfo(syncId1);
	if (!sync0 || !sync1)
		return;

	const char *axisStr = "X\0Y\0Z\0\0";
	const static Vector3 axis[3] = { Vector3(1,0,0), Vector3(0,1,0), Vector3(0,0,1) };
	static int axisIdx = 0;
	const bool editAxis = ImGui::Combo("Joint Axis", &axisIdx, axisStr);

	UpdateUIJoint(sync0, sync1, editAxis, axis[axisIdx]);

	if (ImGui::Button("Pivot Setting"))
	{
		g_pheno->ChangeEditMode(ePhenoEditMode::Pivot0);
		g_pheno->m_selJoint = &g_pheno->m_uiJoint;
		g_pheno->m_gizmo.m_type = graphic::eGizmoEditType::None;
	}

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.4f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.8f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0, 1));
	if (ImGui::Button("Create Fixed Joint"))
	{
		g_pheno->UpdateActorDimension(sync0->actor, true);
		g_pheno->UpdateActorDimension(sync1->actor, true);

		const Transform pivot0 = g_pheno->m_uiJointRenderer.GetPivotWorldTransform(0);
		const Transform pivot1 = g_pheno->m_uiJointRenderer.GetPivotWorldTransform(1);

		phys::cJoint *joint = new phys::cJoint();
		joint->CreateFixed(*g_pheno->m_physics
			, sync0->actor, sync0->node->m_transform, pivot0.pos
			, sync1->actor, sync1->node->m_transform, pivot1.pos);

		cJointRenderer *jointRenderer = new cJointRenderer();
		jointRenderer->Create(*g_pheno->m_physSync, joint);
		physSync->AddJoint(joint, jointRenderer);

		g_pheno->ChangeEditMode(ePhenoEditMode::Normal);
	}
	ImGui::PopStyleColor(3);
}


void cPhenoEditorView::RenderSphericalJoint()
{
	using namespace physx;

	const int syncId0 = g_pheno->m_pairSyncId0;
	const int syncId1 = g_pheno->m_pairSyncId1;

	phys::cPhysicsSync *physSync = g_pheno->m_physSync;
	phys::sSyncInfo *sync0 = physSync->FindSyncInfo(syncId0);
	phys::sSyncInfo *sync1 = physSync->FindSyncInfo(syncId1);
	if (!sync0 || !sync1)
		return;

	static Vector2 limit(PxPi / 2.f, PxPi / 6.f);
	static bool isLimit;
	ImGui::Text("Limit Cone (Radian)");
	ImGui::SameLine();
	ImGui::Checkbox("##Limit", &isLimit);
	ImGui::DragFloat("Y Limit Angle", &limit.x, 0.001f);
	ImGui::DragFloat("Z Limit Angle", &limit.y, 0.001f);	

	const char *axisStr = "X\0Y\0Z\0\0";
	const static Vector3 axis[3] = { Vector3(1,0,0), Vector3(0,1,0), Vector3(0,0,1) };
	static int axisIdx = 0;
	const bool editAxis = ImGui::Combo("Joint Axis", &axisIdx, axisStr);

	UpdateUIJoint(sync0, sync1, editAxis, axis[axisIdx]);

	if (ImGui::Button("Pivot Setting"))
	{
		g_pheno->ChangeEditMode(ePhenoEditMode::Pivot0);
		g_pheno->m_selJoint = &g_pheno->m_uiJoint;
		g_pheno->m_gizmo.m_type = graphic::eGizmoEditType::None;
	}

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.4f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.8f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0, 1));
	if (ImGui::Button("Create Spherical Joint"))
	{
		g_pheno->UpdateActorDimension(sync0->actor, true);
		g_pheno->UpdateActorDimension(sync1->actor, true);

		const Transform pivot0 = g_pheno->m_uiJointRenderer.GetPivotWorldTransform(0);
		const Transform pivot1 = g_pheno->m_uiJointRenderer.GetPivotWorldTransform(1);

		phys::cJoint *joint = new phys::cJoint();
		joint->CreateSpherical(*g_pheno->m_physics
			, sync0->actor, sync0->node->m_transform, pivot0.pos
			, sync1->actor, sync1->node->m_transform, pivot1.pos);

		if (isLimit)
		{
			joint->SetConeLimit(PxJointLimitCone(limit.x, limit.y, 0.01f));
			joint->SetSphericalJointFlag(PxSphericalJointFlag::eLIMIT_ENABLED, true);
		}

		cJointRenderer *jointRenderer = new cJointRenderer();
		jointRenderer->Create(*g_pheno->m_physSync, joint);
		physSync->AddJoint(joint, jointRenderer);

		g_pheno->ChangeEditMode(ePhenoEditMode::Normal);
	}
	ImGui::PopStyleColor(3);
}


void cPhenoEditorView::RenderRevoluteJoint()
{
	using namespace physx;

	const int syncId0 = g_pheno->m_pairSyncId0;
	const int syncId1 = g_pheno->m_pairSyncId1;

	phys::cPhysicsSync *physSync = g_pheno->m_physSync;
	phys::sSyncInfo *sync0 = physSync->FindSyncInfo(syncId0);
	phys::sSyncInfo *sync1 = physSync->FindSyncInfo(syncId1);
	if (!sync0 || !sync1)
		return;

	static Vector2 limit(-PxPi / 2.f, PxPi / 2.f);
	static bool isDrive = false;
	static float velocity = 1.f;
	static bool isCycleDrive = false;
	static float cycleDrivePeriod = 3.f;
	static float cycleDriveVelocityAccel = 1.0f;
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

	if (!isDrive)
		isCycleDrive = false;

	ImGui::TextUnformatted("Cycle");
	ImGui::SameLine();
	ImGui::Checkbox("##Cycle", &isCycleDrive);
	//ImGui::DragFloat("Cycle Period", &cycleDrivePeriod, 0.001f);
	ImGui::DragFloat("Drive Accleration", &cycleDriveVelocityAccel, 0.001f);

	if (ImGui::Button("Pivot Setting"))
	{
		g_pheno->ChangeEditMode(ePhenoEditMode::Pivot0);
		g_pheno->m_selJoint = &g_pheno->m_uiJoint;
		g_pheno->m_gizmo.m_type = graphic::eGizmoEditType::None;
	}

	UpdateUIJoint(sync0, sync1, editAxis, axis[axisIdx]);

	ImGui::Spacing();
	ImGui::Spacing();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.4f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.8f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0, 1));
	if (ImGui::Button("Create Revolute Joint"))
	{
		g_pheno->UpdateActorDimension(sync0->actor, true);
		g_pheno->UpdateActorDimension(sync1->actor, true);

		const Transform pivot0 = g_pheno->m_uiJointRenderer.GetPivotWorldTransform(0);
		const Transform pivot1 = g_pheno->m_uiJointRenderer.GetPivotWorldTransform(1);

		phys::cJoint *joint = new phys::cJoint();
		joint->CreateRevolute(*g_pheno->m_physics
			, sync0->actor, sync0->node->m_transform, pivot0.pos
			, sync1->actor, sync1->node->m_transform, pivot1.pos
			, axis[axisIdx]);

		joint->EnableAngularLimit(isLimit);

		if (isLimit)
			joint->SetAngularLimit(PxJointAngularLimitPair(limit.x, limit.y, 0.01f));
		else
			joint->SetAngularLimit(PxJointAngularLimitPair(-PxPi*2.f, PxPi*2.f, 0.01f));
			//joint->SetAngularLimit(PxJointAngularLimitPair(FLT_MIN, FLT_MAX));

		joint->EnableDrive(isDrive);
		if (isDrive)
			joint->SetDriveVelocity(velocity);

		joint->EnableCycleDrive(isDrive && isCycleDrive);
		if (isDrive && isCycleDrive)
			joint->SetCycleDrivePeriod(cycleDrivePeriod, cycleDriveVelocityAccel);

		cJointRenderer *jointRenderer = new cJointRenderer();
		jointRenderer->Create(*g_pheno->m_physSync, joint);
		physSync->AddJoint(joint, jointRenderer);

		g_pheno->ChangeEditMode(ePhenoEditMode::Normal);
	}
	ImGui::PopStyleColor(3);
}


void cPhenoEditorView::RenderPrismaticJoint()
{
	using namespace physx;

	const int syncId0 = g_pheno->m_pairSyncId0;
	const int syncId1 = g_pheno->m_pairSyncId1;

	phys::cPhysicsSync *physSync = g_pheno->m_physSync;
	phys::sSyncInfo *sync0 = physSync->FindSyncInfo(syncId0);
	phys::sSyncInfo *sync1 = physSync->FindSyncInfo(syncId1);
	if (!sync0 || !sync1)
		return;

	static Vector2 limit1(-1.f, 1.f); // lower, upper
	static Vector2 limit2(10.f, 0.0f); // stiffness, damping (spring)
	static Vector2 limit3(3.f, 0.0f); // length
	static bool isLimit = false;
	static bool isSpring = true;

	ImGui::Text("Linear Limit");
	ImGui::SameLine();
	ImGui::Checkbox("##Limit", &isLimit);
	ImGui::DragFloat("Lower Limit", &limit1.x, 0.001f);
	ImGui::DragFloat("Upper Limit", &limit1.y, 0.001f);
	ImGui::Text("Spring");
	ImGui::SameLine();
	ImGui::Checkbox("##Spring", &isSpring);
	ImGui::DragFloat("Stiffness", &limit2.x, 0.001f);
	ImGui::DragFloat("Damping", &limit2.y, 0.001f);
	ImGui::DragFloat("Length (linear)", &limit3.x, 0.001f);

	const char *axisStr = "X\0Y\0Z\0\0";
	const static Vector3 axis[3] = { Vector3(1,0,0), Vector3(0,1,0), Vector3(0,0,1) };
	static int axisIdx = 0;
	const bool editAxis = ImGui::Combo("Prismatic Axis", &axisIdx, axisStr);

	if (ImGui::Button("Pivot Setting"))
	{
		g_pheno->ChangeEditMode(ePhenoEditMode::Pivot0);
		g_pheno->m_selJoint = &g_pheno->m_uiJoint;
		g_pheno->m_gizmo.m_type = graphic::eGizmoEditType::None;
	}

	UpdateUIJoint(sync0, sync1, editAxis, axis[axisIdx]);

	ImGui::Spacing();
	ImGui::Spacing();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.4f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.8f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0, 1));
	if (ImGui::Button("Create Prismatic Joint"))
	{
		g_pheno->UpdateActorDimension(sync0->actor, true);
		g_pheno->UpdateActorDimension(sync1->actor, true);

		const Transform pivot0 = g_pheno->m_uiJointRenderer.GetPivotWorldTransform(0);
		const Transform pivot1 = g_pheno->m_uiJointRenderer.GetPivotWorldTransform(1);

		phys::cJoint *joint = new phys::cJoint();
		joint->CreatePrismatic(*g_pheno->m_physics
			, sync0->actor, sync0->node->m_transform, pivot0.pos
			, sync1->actor, sync1->node->m_transform, pivot1.pos
			, axis[axisIdx]);

		joint->EnableLinearLimit(isLimit);

		if (isLimit)
		{
			if (isSpring)
			{
				joint->SetLinearLimit(PxJointLinearLimitPair(limit1.x, limit1.y
					, physx::PxSpring(limit2.x, limit2.y)));
			}
			else
			{
				PxTolerancesScale scale;
				scale.length = limit3.x;
				joint->SetLinearLimit(PxJointLinearLimitPair(scale, limit1.x, limit1.y));
			}
		}

		cJointRenderer *jointRenderer = new cJointRenderer();
		jointRenderer->Create(*g_pheno->m_physSync, joint);
		physSync->AddJoint(joint, jointRenderer);

		g_pheno->ChangeEditMode(ePhenoEditMode::Normal);
	}
	ImGui::PopStyleColor(3);
}


void cPhenoEditorView::RenderDistanceJoint()
{
	using namespace physx;

	const int syncId0 = g_pheno->m_pairSyncId0;
	const int syncId1 = g_pheno->m_pairSyncId1;

	phys::cPhysicsSync *physSync = g_pheno->m_physSync;
	phys::sSyncInfo *sync0 = physSync->FindSyncInfo(syncId0);
	phys::sSyncInfo *sync1 = physSync->FindSyncInfo(syncId1);
	if (!sync0 || !sync1)
		return;

	static Vector2 limit(0.f, 2.f);
	static bool isLimit = false;

	ImGui::Text("Distance Limit");
	ImGui::SameLine();
	ImGui::Checkbox("##Limit", &isLimit);
	ImGui::DragFloat("min distance", &limit.x, 0.001f);
	ImGui::DragFloat("max distance", &limit.y, 0.001f);

	const char *axisStr = "X\0Y\0Z\0\0";
	const static Vector3 axis[3] = { Vector3(1,0,0), Vector3(0,1,0), Vector3(0,0,1) };
	static int axisIdx = 0;
	const bool editAxis = ImGui::Combo("Joint Axis", &axisIdx, axisStr);

	UpdateUIJoint(sync0, sync1, editAxis, axis[axisIdx]);

	if (ImGui::Button("Pivot Setting"))
	{
		g_pheno->ChangeEditMode(ePhenoEditMode::Pivot0);
		g_pheno->m_selJoint = &g_pheno->m_uiJoint;
		g_pheno->m_gizmo.m_type = graphic::eGizmoEditType::None;
	}

	ImGui::Spacing();
	ImGui::Spacing();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.4f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.8f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0, 1));
	if (ImGui::Button("Create Distance Joint"))
	{
		g_pheno->UpdateActorDimension(sync0->actor, true);
		g_pheno->UpdateActorDimension(sync1->actor, true);

		const Transform pivot0 = g_pheno->m_uiJointRenderer.GetPivotWorldTransform(0);
		const Transform pivot1 = g_pheno->m_uiJointRenderer.GetPivotWorldTransform(1);

		phys::cJoint *joint = new phys::cJoint();
		joint->CreateDistance(*g_pheno->m_physics
			, sync0->actor, sync0->node->m_transform, pivot0.pos
			, sync1->actor, sync1->node->m_transform, pivot1.pos);

		if (isLimit)
		{
			joint->EnableDistanceLimit(isLimit);
			joint->SetDistanceLimit(limit.x, limit.y);
		}

		cJointRenderer *jointRenderer = new cJointRenderer();
		jointRenderer->Create(*g_pheno->m_physSync, joint);
		physSync->AddJoint(joint, jointRenderer);

		g_pheno->ChangeEditMode(ePhenoEditMode::Normal);
	}
	ImGui::PopStyleColor(3);
}


// 
void cPhenoEditorView::RenderD6Joint()
{
	using namespace physx;

	const int syncId0 = g_pheno->m_pairSyncId0;
	const int syncId1 = g_pheno->m_pairSyncId1;

	const char *motionAxis[6] = { "X         ", "Y         ", "Z          "
		, "Twist   ", "Swing1", "Swing2" };
	const char *motionStr = "Lock\0Limit\0Free\0\0";
	const char *driveAxis[6] = { "X         ", "Y         ", "Z          "
		, "Swing   ", "Twist", "Slerp" };

	static int motionVal[6] = { 0, 0, 0, 0, 0, 0 };
	static bool driveVal[6] = { 0, 0, 0, 0, 0, 0 };
	struct sDriveParam 
	{
		float stiffness;
		float damping;
		float forceLimit;
		bool accel;
	};
	static sDriveParam driveConfigs[6] = {
		{10.f, 0.f, PX_MAX_F32, true}, // X
		{10.f, 0.f, PX_MAX_F32, true}, // Y
		{10.f, 0.f, PX_MAX_F32, true}, // Z
		{10.f, 0.f, PX_MAX_F32, true}, // Twist
		{10.f, 0.f, PX_MAX_F32, true}, // Swing1
		{10.f, 0.f, PX_MAX_F32, true}, // Swing2
	};
	static PxJointLinearLimit linearLimit(1.f, PxSpring(10.f, 0.f));
	static PxJointAngularLimitPair twistLimit(-PxPi / 2.f, PxPi / 2.f);
	static PxJointLimitCone swingLimit(-PxPi / 2.f, PxPi / 2.f);
	static bool isLinearLimit = false;
	static bool isTwistLimit = false;
	static bool isSwingLimit = false;
	static Vector3 linearDriveVelocity(0, 0, 0);
	static Vector3 angularDriveVelocity(0, 0, 0);

	phys::cPhysicsSync *physSync = g_pheno->m_physSync;
	phys::sSyncInfo *sync0 = physSync->FindSyncInfo(syncId0);
	phys::sSyncInfo *sync1 = physSync->FindSyncInfo(syncId1);
	if (!sync0 || !sync1)
		return;

	// Motion
	ImGui::SetNextTreeNodeOpen(true, ImGuiCond_FirstUseEver);
	if (ImGui::TreeNode("Motion"))
	{
		for (int i = 0; i < 6; ++i)
		{
			int id = i * 100;
			ImGui::PushItemWidth(200);
			ImGui::TextUnformatted(motionAxis[i]);
			ImGui::SameLine();
			ImGui::PushID(id++);
			if (ImGui::Combo("##Motion", &motionVal[i], motionStr))
			{
				if (driveVal[i])
					driveVal[i] = (0 != motionVal[i]); // motion lock -> drive lock
			}
			ImGui::PopID();
			ImGui::PopItemWidth();
		}
		ImGui::TreePop();
	}
	ImGui::Separator();
	// Drive
	ImGui::SetNextTreeNodeOpen(true, ImGuiCond_FirstUseEver);
	if (ImGui::TreeNode("Drive"))
	{
		for (int i = 0; i < 6; ++i)
		{
			int id = i * 1000;
			ImGui::Checkbox(driveAxis[i], &driveVal[i]);

			if (driveVal[i])
			{
				sDriveParam &drive = driveConfigs[i];

				ImGui::Indent(30);
				ImGui::PushItemWidth(150);

				ImGui::TextUnformatted("Stiffness   ");
				ImGui::SameLine();
				ImGui::PushID(id++);
				ImGui::DragFloat("##Stiffness", &drive.stiffness);
				ImGui::PopID();

				ImGui::TextUnformatted("Dampping");
				ImGui::SameLine();
				ImGui::PushID(id++);
				ImGui::DragFloat("##Dampping", &drive.damping);
				ImGui::PopID();

				ImGui::TextUnformatted("Force Limit");
				ImGui::SameLine();
				ImGui::PushID(id++);
				ImGui::DragFloat("##Force Limit", &drive.forceLimit);
				ImGui::PopID();

				ImGui::TextUnformatted("Accel");
				ImGui::SameLine();
				ImGui::PushID(id++);
				ImGui::Checkbox("##Accel", &drive.accel);
				ImGui::PopID();

				ImGui::PopItemWidth();
				ImGui::Unindent(30);

				ImGui::Separator();
			}//~drive

			ImGui::Spacing();
		}//~for drive axis 6
		ImGui::TreePop();
	}//~drive tree node

	ImGui::Separator();
	ImGui::TextUnformatted("Linear Drive Velocity");
	ImGui::DragFloat3("##Linear Drive Velocity", (float*)&linearDriveVelocity, 0.001f, 0.f, 1000.f);
	ImGui::TextUnformatted("Angular Drive Velocity");
	ImGui::DragFloat3("##Angular Drive Velocity", (float*)&angularDriveVelocity, 0.001f, 0.f, 1000.f);
	ImGui::Separator();

	// linear limit
	{
		ImGui::TextUnformatted("Linear Limit");
		ImGui::SameLine();
		ImGui::Checkbox("##Linear Limit", &isLinearLimit);
		if (isLinearLimit)
		{
			ImGui::Indent(30);
			ImGui::PushItemWidth(150);
			ImGui::DragFloat("Extend", &linearLimit.value, 0.001f);
			ImGui::DragFloat("Stiffness", &linearLimit.stiffness, 0.001f);
			ImGui::DragFloat("Damping", &linearLimit.damping, 0.001f);
			ImGui::PopItemWidth();
			ImGui::Unindent(30);
		}
	}

	// twist limit
	{
		ImGui::TextUnformatted("Twist Limit ");
		ImGui::SameLine();
		ImGui::Checkbox("##Twist Limit", &isTwistLimit);
		if (isTwistLimit)
		{
			ImGui::Indent(30);
			ImGui::PushItemWidth(150);
			ImGui::DragFloat("Lower Angle", &twistLimit.lower, 0.001f);
			ImGui::DragFloat("Upper Angle", &twistLimit.upper, 0.001f);
			ImGui::PopItemWidth();
			ImGui::Unindent(30);
		}
	}

	// swing limit
	{
		ImGui::TextUnformatted("Swing Limit");
		ImGui::SameLine();
		ImGui::Checkbox("##Swing Limit", &isSwingLimit);
		if (isSwingLimit)
		{
			ImGui::Indent(30);
			ImGui::PushItemWidth(150);
			ImGui::DragFloat("Y Angle", &swingLimit.yAngle, 0.001f);
			ImGui::DragFloat("Z Angle", &swingLimit.zAngle, 0.001f);
			ImGui::PopItemWidth();
			ImGui::Unindent(30);
		}
	}

	if (ImGui::Button("Pivot Setting"))
	{
		g_pheno->ChangeEditMode(ePhenoEditMode::Pivot0);
		g_pheno->m_selJoint = &g_pheno->m_uiJoint;
		g_pheno->m_gizmo.m_type = graphic::eGizmoEditType::None;
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.6f, 0.1f, 1.f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.8f, 0.1f, 1.f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0.1f, 1.f));
	if (ImGui::Button("Create D6 Joint"))
	{
		g_pheno->UpdateActorDimension(sync0->actor, true);
		g_pheno->UpdateActorDimension(sync1->actor, true);

		const Transform pivot0 = g_pheno->m_uiJointRenderer.GetPivotWorldTransform(0);
		const Transform pivot1 = g_pheno->m_uiJointRenderer.GetPivotWorldTransform(1);

		phys::cJoint *joint = new phys::cJoint();
		joint->CreateD6(*g_pheno->m_physics
			, sync0->actor, sync0->node->m_transform, pivot0.pos
			, sync1->actor, sync1->node->m_transform, pivot1.pos);

		for (int i = 0; i < 6; ++i)
			joint->SetMotion((PxD6Axis::Enum)i, (PxD6Motion::Enum)motionVal[i]);

		for (int i = 0; i < 6; ++i)
		{
			if (driveVal[i])
			{
				joint->SetD6Drive((PxD6Drive::Enum)i
					, physx::PxD6JointDrive(driveConfigs[i].stiffness
						, driveConfigs[i].damping
						, driveConfigs[i].forceLimit
						, driveConfigs[i].accel));
			}
		}

		if (isLinearLimit)
			joint->SetD6LinearLimit(linearLimit);
		if (isTwistLimit)
			joint->SetD6TwistLimit(twistLimit);
		if (isSwingLimit)
			joint->SetD6SwingLimit(swingLimit);

		joint->SetD6DriveVelocity(linearDriveVelocity, angularDriveVelocity);

		cJointRenderer *jointRenderer = new cJointRenderer();
		jointRenderer->Create(*g_pheno->m_physSync, joint);
		physSync->AddJoint(joint, jointRenderer);

		g_pheno->ChangeEditMode(ePhenoEditMode::Normal);
	}
	ImGui::PopStyleColor(3);
	ImGui::Spacing();
	ImGui::Spacing();
}


void cPhenoEditorView::RenderCompound()
{
	const int syncId0 = g_pheno->m_pairSyncId0;
	const int syncId1 = g_pheno->m_pairSyncId1;

	phys::cPhysicsSync *physSync = g_pheno->m_physSync;
	phys::sSyncInfo *sync0 = physSync->FindSyncInfo(syncId0);
	phys::sSyncInfo *sync1 = physSync->FindSyncInfo(syncId1);
	if (!sync0 || !sync1)
		return;

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.4f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.8f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0, 1));
	if (ImGui::Button("Create Compound"))
	{
		g_pheno->UpdateActorDimension(sync0->actor, true);
		g_pheno->UpdateActorDimension(sync1->actor, true);

		physSync->AddCompound(sync0, sync1);
		g_pheno->ClearJointEdit();
	}
	ImGui::PopStyleColor(3);
}


void cPhenoEditorView::RenderRevoluteJointSetting(phys::cJoint *joint
	, INOUT evc::sGenotypeLink &info)
{
	using namespace physx;

	ImGui::Text("Angular Limit (Radian)");
	ImGui::SameLine();
	ImGui::Checkbox("##Limit", &info.limit.angular.isLimit);
	ImGui::Indent(30);
	ImGui::PushItemWidth(150);
	ImGui::DragFloat("Lower Angle", &info.limit.angular.lower, 0.001f);
	ImGui::DragFloat("Upper Angle", &info.limit.angular.upper, 0.001f);
	ImGui::PopItemWidth();
	ImGui::Unindent(30);

	ImGui::TextUnformatted("Drive");
	ImGui::SameLine();
	ImGui::Checkbox("##Drive", &info.drive.isDrive);
	ImGui::Indent(30);
	ImGui::PushItemWidth(150);
	ImGui::DragFloat("Velocity", &info.drive.velocity, 0.001f);

	//const char *axisStr = "X\0Y\0Z\0\0";
	//const static Vector3 axis[3] = { Vector3(1,0,0), Vector3(0,1,0), Vector3(0,0,1) };
	//static int axisIdx = 0;
	//ImGui::Combo("Revolute Axis", &axisIdx, axisStr);
	ImGui::PopItemWidth();
	ImGui::Unindent(30);

	if (!info.drive.isDrive)
		info.drive.isCycle = false;

	ImGui::TextUnformatted("Cycle");
	ImGui::SameLine();
	ImGui::Checkbox("##Cycle", &info.drive.isCycle);
	ImGui::Indent(30);
	ImGui::PushItemWidth(150);
	//ImGui::DragFloat("Cycle Period", &info.drive.period, 0.001f);
	ImGui::DragFloat("Drive Accleration", &info.drive.driveAccel, 0.001f);
	ImGui::PopItemWidth();
	ImGui::Unindent(30);

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.4f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.8f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0, 1));
	if (ImGui::Button("Apply Option"))
	{
		joint->EnableAngularLimit(info.limit.angular.isLimit);
		if (info.limit.angular.isLimit)
		{
			PxJointAngularLimitPair limit(-PxPi / 2.f, PxPi / 2.f, 0.01f);
			limit.lower = info.limit.angular.lower;
			limit.upper = info.limit.angular.upper;
			joint->SetAngularLimit(limit);
		}

		joint->EnableDrive(info.drive.isDrive);
		if (info.drive.isDrive)
			joint->SetDriveVelocity(info.drive.velocity);

		joint->EnableCycleDrive(info.drive.isDrive && info.drive.isCycle);
		if (info.drive.isDrive && info.drive.isCycle)
			joint->SetCycleDrivePeriod(info.drive.period, info.drive.driveAccel);

		joint->m_actor0->WakeUp();
		joint->m_actor1->WakeUp();
	}
	ImGui::PopStyleColor(3);
}


void cPhenoEditorView::RenderPrismaticJointSetting(phys::cJoint *joint
	, INOUT evc::sGenotypeLink &info)
{
	using namespace physx;

	ImGui::Separator();
	ImGui::Text("Linear Limit");
	ImGui::SameLine();
	ImGui::Checkbox("##Limit", &info.limit.linear.isLimit);
	ImGui::Indent(30);
	ImGui::PushItemWidth(150);
	ImGui::DragFloat("Lower Limit", &info.limit.linear.lower, 0.001f);
	ImGui::DragFloat("Upper Limit", &info.limit.linear.upper, 0.001f);
	ImGui::PopItemWidth();
	ImGui::Unindent(30);
	
	ImGui::Text("Spring");
	ImGui::SameLine();
	ImGui::Checkbox("##Spring", &info.limit.linear.isSpring);
	ImGui::Indent(30);
	ImGui::PushItemWidth(150);
	ImGui::DragFloat("Stiffness", &info.limit.linear.stiffness, 0.001f);
	ImGui::DragFloat("Damping", &info.limit.linear.damping, 0.001f);
	ImGui::DragFloat("Length (linear)", &info.limit.linear.value, 0.001f);
	ImGui::PopItemWidth();
	ImGui::Unindent(30);

	//const char *axisStr = "X\0Y\0Z\0\0";
	//const static Vector3 axis[3] = { Vector3(1,0,0), Vector3(0,1,0), Vector3(0,0,1) };
	//static int axisIdx = 0;
	//const bool editAxis = ImGui::Combo("Prismatic Axis", &axisIdx, axisStr);

	ImGui::Spacing();
	ImGui::Spacing();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.4f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.8f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0, 1));
	if (ImGui::Button("Apply Option"))
	{
		joint->EnableLinearLimit(info.limit.linear.isLimit);

		if (info.limit.linear.isLimit)
		{
			if (info.limit.linear.isSpring)
			{
				joint->SetLinearLimit(PxJointLinearLimitPair(
					info.limit.linear.lower, info.limit.linear.upper
					, physx::PxSpring(info.limit.linear.stiffness
						, info.limit.linear.damping)));
			}
			else
			{
				PxTolerancesScale scale;
				scale.length = info.limit.linear.value;
				joint->SetLinearLimit(PxJointLinearLimitPair(scale
					, info.limit.linear.lower, info.limit.linear.upper));
			}
		}

		if (cJointRenderer *j = g_pheno->FindJointRenderer(joint))
			j->UpdateLimit();
	}
	ImGui::PopStyleColor(3);
}


void cPhenoEditorView::RenderDistanceJointSetting(phys::cJoint *joint
	, INOUT evc::sGenotypeLink &info)
{
	ImGui::Text("Distance Limit");
	ImGui::SameLine();
	ImGui::Checkbox("##Limit", &info.limit.distance.isLimit);
	ImGui::DragFloat("min distance", &info.limit.distance.minDistance, 0.001f);
	ImGui::DragFloat("max distance", &info.limit.distance.maxDistance, 0.001f);

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.4f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.8f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0, 1));
	if (ImGui::Button("Apply Option"))
	{
		joint->EnableDistanceLimit(info.limit.distance.isLimit);
		if (info.limit.distance.isLimit)
			joint->SetDistanceLimit(info.limit.distance.minDistance
				, info.limit.distance.maxDistance);
	}
	ImGui::PopStyleColor(3);
}


void cPhenoEditorView::RenderD6JointSetting(phys::cJoint *joint
	, INOUT evc::sGenotypeLink &info)
{
	using namespace physx;

	const char *motionAxis[6] = { "X         ", "Y         ", "Z          "
		, "Twist   ", "Swing1", "Swing2" };
	const char *motionStr = "Lock\0Limit\0Free\0\0";
	const char *driveAxis[6] = { "X         ", "Y         ", "Z          "
		, "Swing   ", "Twist", "Slerp" };

	// Motion
	ImGui::SetNextTreeNodeOpen(true, ImGuiCond_FirstUseEver);
	if (ImGui::TreeNode("Motion"))
	{
		for (int i = 0; i < 6; ++i)
		{
			int id = i * 100;
			ImGui::PushItemWidth(200);
			ImGui::TextUnformatted(motionAxis[i]);
			ImGui::SameLine();
			ImGui::PushID(id++);
			if (ImGui::Combo("##Motion", &info.limit.d6.motion[i], motionStr))
			{
				if (info.limit.d6.drive[i].isDrive) // motion lock -> drive lock
					info.limit.d6.drive[i].isDrive = (0 != info.limit.d6.motion[i]);
			}
			ImGui::PopID();
			ImGui::PopItemWidth();
		}
		ImGui::TreePop();
	}
	ImGui::Separator();
	// Drive
	ImGui::SetNextTreeNodeOpen(true, ImGuiCond_FirstUseEver);
	if (ImGui::TreeNode("Drive"))
	{
		for (int i = 0; i < 6; ++i)
		{
			int id = i * 1000;
			ImGui::Checkbox(driveAxis[i], &info.limit.d6.drive[i].isDrive);

			if (info.limit.d6.drive[i].isDrive)
			{
				evc::sDriveParam &drive = info.limit.d6.drive[i];

				ImGui::Indent(30);
				ImGui::PushItemWidth(150);

				ImGui::TextUnformatted("Stiffness   ");
				ImGui::SameLine();
				ImGui::PushID(id++);
				ImGui::DragFloat("##Stiffness", &drive.stiffness);
				ImGui::PopID();

				ImGui::TextUnformatted("Dampping");
				ImGui::SameLine();
				ImGui::PushID(id++);
				ImGui::DragFloat("##Dampping", &drive.damping);
				ImGui::PopID();

				ImGui::TextUnformatted("Force Limit");
				ImGui::SameLine();
				ImGui::PushID(id++);
				ImGui::DragFloat("##Force Limit", &drive.forceLimit);
				ImGui::PopID();

				ImGui::TextUnformatted("Accel");
				ImGui::SameLine();
				ImGui::PushID(id++);
				ImGui::Checkbox("##Accel", &drive.accel);
				ImGui::PopID();

				ImGui::PopItemWidth();
				ImGui::Unindent(30);

				ImGui::Separator();
			}//~drive

			ImGui::Spacing();
		}//~for drive axis 6
		ImGui::TreePop();
	}//~drive tree node

	ImGui::Separator();
	ImGui::TextUnformatted("Linear Drive Velocity");
	ImGui::DragFloat3("##Linear Drive Velocity", (float*)info.limit.d6.linearVelocity, 0.001f, 0.f, 1000.f);
	ImGui::TextUnformatted("Angular Drive Velocity");
	ImGui::DragFloat3("##Angular Drive Velocity", (float*)info.limit.d6.angularVelocity, 0.001f, 0.f, 1000.f);
	ImGui::Separator();

	// linear limit
	{
		ImGui::TextUnformatted("Linear Limit");
		ImGui::SameLine();
		ImGui::Checkbox("##Linear Limit", &info.limit.d6.linear.isLimit);
		if (info.limit.d6.linear.isLimit)
		{
			ImGui::Indent(30);
			ImGui::PushItemWidth(150);
			ImGui::DragFloat("Extend", &info.limit.d6.linear.value, 0.001f);
			ImGui::DragFloat("Stiffness", &info.limit.d6.linear.stiffness, 0.001f);
			ImGui::DragFloat("Damping", &info.limit.d6.linear.damping, 0.001f);
			ImGui::PopItemWidth();
			ImGui::Unindent(30);
		}
	}

	// twist limit
	{
		ImGui::TextUnformatted("Twist Limit ");
		ImGui::SameLine();
		ImGui::Checkbox("##Twist Limit", &info.limit.d6.twist.isLimit);
		if (info.limit.d6.twist.isLimit)
		{
			ImGui::Indent(30);
			ImGui::PushItemWidth(150);
			ImGui::DragFloat("Lower Angle", &info.limit.d6.twist.lower, 0.001f);
			ImGui::DragFloat("Upper Angle", &info.limit.d6.twist.upper, 0.001f);
			ImGui::PopItemWidth();
			ImGui::Unindent(30);
		}
	}

	// swing limit
	{
		ImGui::TextUnformatted("Swing Limit");
		ImGui::SameLine();
		ImGui::Checkbox("##Swing Limit", &info.limit.d6.swing.isLimit);
		if (info.limit.d6.swing.isLimit)
		{
			ImGui::Indent(30);
			ImGui::PushItemWidth(150);
			ImGui::DragFloat("Y Angle", &info.limit.d6.swing.yAngle, 0.001f);
			ImGui::DragFloat("Z Angle", &info.limit.d6.swing.zAngle, 0.001f);
			ImGui::PopItemWidth();
			ImGui::Unindent(30);
		}
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.4f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.8f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0, 1));
	if (ImGui::Button("Apply Option"))
	{
		for (int i = 0; i < 6; ++i)
			joint->SetMotion((PxD6Axis::Enum)i, (PxD6Motion::Enum)info.limit.d6.motion[i]);

		for (int i = 0; i < 6; ++i)
		{
			if (info.limit.d6.drive[i].isDrive)
			{
				joint->SetD6Drive((PxD6Drive::Enum)i
					, physx::PxD6JointDrive(info.limit.d6.drive[i].stiffness
						, info.limit.d6.drive[i].damping
						, info.limit.d6.drive[i].forceLimit
						, info.limit.d6.drive[i].accel));
			}
		}

		if (info.limit.d6.linear.isLimit)
		{
			const PxJointLinearLimit linearLimit(info.limit.d6.linear.value
				, PxSpring(info.limit.d6.linear.stiffness, info.limit.d6.linear.damping));
			joint->SetD6LinearLimit(linearLimit);
		}
		if (info.limit.d6.twist.isLimit)
		{
			PxJointAngularLimitPair twistLimit(info.limit.d6.twist.lower
				, info.limit.d6.twist.upper);
			joint->SetD6TwistLimit(twistLimit);
		}
		if (info.limit.d6.swing.isLimit)
		{
			PxJointLimitCone swingLimit(info.limit.d6.swing.yAngle
				, info.limit.d6.swing.zAngle);
			joint->SetD6SwingLimit(swingLimit);
		}

		joint->SetD6DriveVelocity(*(Vector3*)info.limit.d6.linearVelocity
			, *(Vector3*)info.limit.d6.angularVelocity);
	}
	ImGui::PopStyleColor(3);
}


void cPhenoEditorView::RenderCompoundSetting(phys::cJoint *joint
	, INOUT evc::sGenotypeLink &info)
{

}


void cPhenoEditorView::RenderSphericalJointSetting(phys::cJoint *joint
	, INOUT evc::sGenotypeLink &info)
{
	using namespace physx;

	ImGui::Text("Limit Cone (Radian)");
	ImGui::SameLine();
	ImGui::Checkbox("##Limit", &info.limit.cone.isLimit);
	ImGui::Indent(30);
	ImGui::PushItemWidth(150);
	ImGui::DragFloat("Y Angle", &info.limit.cone.yAngle, 0.001f);
	ImGui::DragFloat("Z Angle", &info.limit.cone.zAngle, 0.001f);
	ImGui::PopItemWidth();
	ImGui::Unindent(30);

	//const char *axisStr = "X\0Y\0Z\0\0";
	//const static Vector3 axis[3] = { Vector3(1,0,0), Vector3(0,1,0), Vector3(0,0,1) };
	//static int axisIdx = 0;
	//ImGui::Combo("Revolute Axis", &axisIdx, axisStr);

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.4f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.8f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0, 1));
	if (ImGui::Button("Apply Option"))
	{
		PxJointLimitCone limit(-PxPi / 2.f, PxPi / 2.f, 0.01f);
		limit.yAngle = info.limit.cone.yAngle;
		limit.zAngle = info.limit.cone.zAngle;
		joint->SetConeLimit(limit);
		joint->EnableConeLimit(info.limit.cone.isLimit);

		joint->m_actor0->WakeUp();
		joint->m_actor1->WakeUp();
	}
	ImGui::PopStyleColor(3);
}


// check cancel show ui joint
// return true if change state
bool cPhenoEditorView::CheckCancelUIJoint()
{
	if (!g_pheno->m_showUIJoint)
	{
		// fixed, spherical, distance, d6 joint?
		// check cancel ui joint
		if ((g_pheno->m_selects.size() != 2)
			&& (g_pheno->GetEditMode() == ePhenoEditMode::JointEdit))
			goto $cancel;
		return false;
	}

	if ((g_pheno->m_selects.size() != 1)
		&& (g_pheno->m_selects.size() != 2)
		&& (g_pheno->m_selects.size() != 3))
	{
		goto $cancel;
	}
	else if (g_pheno->m_selects.size() == 1)
	{
		phys::sSyncInfo *sync0 = g_pheno->FindSyncInfo(g_pheno->m_selects[0]);
		if (!sync0->joint) // ui joint selection?
			goto $cancel;
	}
	else
	{
		// check rigidactor x 2
		if (g_pheno->m_selects.size() == 2)
		{
			phys::sSyncInfo *sync0 = g_pheno->FindSyncInfo(g_pheno->m_selects[0]);
			phys::sSyncInfo *sync1 = g_pheno->FindSyncInfo(g_pheno->m_selects[1]);
			if (!sync0 || !sync1)
				goto $cancel;
			if (sync0->joint || sync1->joint)
				goto $cancel;
		}
		else if (g_pheno->m_selects.size() == 3)
		{
			// check joint link rigid actor x 2
			phys::sSyncInfo *syncs[3] = { nullptr, nullptr, nullptr };
			syncs[0] = g_pheno->FindSyncInfo(g_pheno->m_selects[0]);
			syncs[1] = g_pheno->FindSyncInfo(g_pheno->m_selects[1]);
			syncs[2] = g_pheno->FindSyncInfo(g_pheno->m_selects[2]);
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

	return false;

$cancel:
	g_pheno->m_showUIJoint = false;
	g_pheno->m_physSync->RemoveSyncInfo(&g_pheno->m_uiJoint);
	return true;
}


// check change selection
void cPhenoEditorView::CheckChangeSelection()
{
	if (m_oldSelects.size() != g_pheno->m_selects.size())
	{
		goto $change;
	}
	else if (!m_oldSelects.empty())
	{
		// compare select ids
		for (uint i = 0; i < g_pheno->m_selects.size(); ++i)
			if (m_oldSelects[i] != g_pheno->m_selects[i])
				goto $change;
	}

	return; // no change

$change:
	m_isChangeSelection = true;
	m_oldSelects = g_pheno->m_selects;
	if (!g_pheno->m_selects.empty())
		UpdateJointInfo(*g_pheno->m_selects.begin());
}


// update ui joint information
void cPhenoEditorView::UpdateUIJoint(phys::sSyncInfo *sync0
	, phys::sSyncInfo *sync1, const bool editAxis
	, const Vector3 &revoluteAxis)
{
	phys::cPhysicsSync *physSync = g_pheno->m_physSync;

	// update ui joint (once process)
	if (!g_pheno->m_showUIJoint || editAxis || m_isChangeSelection)
	{
		// already register joint?
		if (!g_pheno->FindJointRenderer(&g_pheno->m_uiJoint))
			physSync->AddJoint(&g_pheno->m_uiJoint, &g_pheno->m_uiJointRenderer, false);

		// change selection?
		const bool isPrevSelection = (((g_pheno->m_uiJoint.m_actor0 == sync0->actor)
			&& (g_pheno->m_uiJoint.m_actor1 == sync1->actor))
			|| ((g_pheno->m_uiJoint.m_actor1 == sync0->actor)
				&& (g_pheno->m_uiJoint.m_actor0 == sync1->actor)));

		g_pheno->m_showUIJoint = true;

		{
			g_pheno->m_uiJoint.m_type = phys::eJointType::Revolute;
			g_pheno->m_uiJoint.m_actor0 = sync0->actor;
			g_pheno->m_uiJoint.m_actor1 = sync1->actor;
			g_pheno->m_uiJoint.m_actorLocal0 = sync0->node->m_transform;
			g_pheno->m_uiJoint.m_actorLocal1 = sync1->node->m_transform;
			g_pheno->m_uiJoint.m_revoluteAxis = revoluteAxis;

			g_pheno->m_uiJoint.m_pivots[0].dir = Vector3::Zeroes;
			g_pheno->m_uiJoint.m_pivots[0].len = 0;
			g_pheno->m_uiJoint.m_pivots[1].dir = Vector3::Zeroes;
			g_pheno->m_uiJoint.m_pivots[1].len = 0;

			g_pheno->m_uiJointRenderer.m_joint = &g_pheno->m_uiJoint;
			g_pheno->m_uiJointRenderer.m_sync0 = sync0;
			g_pheno->m_uiJointRenderer.m_sync1 = sync1;

			// auto pivot position targeting
			const Vector3 pos0 = sync0->node->m_transform.pos;
			const Vector3 pos1 = sync1->node->m_transform.pos;
			const Ray ray0(pos1, revoluteAxis);
			const Ray ray1(pos1, -revoluteAxis);

			float dist0 = 0, dist1 = 0;
			sync0->node->Picking(ray0, graphic::eNodeType::MODEL, false, &dist0);
			sync0->node->Picking(ray1, graphic::eNodeType::MODEL, false, &dist1);
			if (dist0 != 0 || dist1 != 0) // find pivot pos
			{
				const Vector3 pivot0 = (dist0 != 0) ?
					revoluteAxis * dist0 + pos1
					: -revoluteAxis * dist1 + pos1;

				g_pheno->m_uiJoint.m_pivots[0].dir = (pivot0 - pos0).Normal();
				g_pheno->m_uiJoint.m_pivots[0].len = pivot0.Distance(pos0);
			}
			//~auto pivot position

			const Vector3 jointPos = (g_pheno->m_uiJointRenderer.GetPivotWorldTransform(0).pos +
				g_pheno->m_uiJointRenderer.GetPivotWorldTransform(1).pos) / 2.f;
			g_pheno->m_uiJointRenderer.SetPivotPosByRevoluteAxis(revoluteAxis, jointPos);
		}
	}
}


// update joint information to genotype link information
void cPhenoEditorView::UpdateJointInfo(const int syncId)
{
	using namespace physx;
	using namespace phys;

	phys::cPhysicsSync *physSync = g_pheno->m_physSync;
	phys::sSyncInfo *sync = physSync->FindSyncInfo(syncId);
	if (!sync)
		return;
	if (!sync->actor)
		return;

	m_jointInfos.clear();

	phys::cRigidActor *actor = sync->actor;
	for (uint i = 0; i < actor->m_joints.size(); ++i)
	{
		phys::cJoint *joint = actor->m_joints[i];

		evc::sGenotypeLink glink;
		switch (joint->m_type)
		{
		case eJointType::Fixed:
		{
			glink.type = eJointType::Fixed;
		}
		break;

		case eJointType::Spherical:
		{
			glink.type = eJointType::Spherical;
			const physx::PxJointLimitCone limit = joint->GetConeLimit();
			glink.limit.cone.isLimit = joint->IsConeLimit();
			glink.limit.cone.yAngle = limit.yAngle;
			glink.limit.cone.zAngle = limit.zAngle;
		}
		break;

		case eJointType::Revolute:
		{
			glink.type = eJointType::Revolute;
			const physx::PxJointAngularLimitPair limit = joint->GetAngularLimit();
			glink.limit.angular.isLimit = joint->IsAngularLimit();
			glink.limit.angular.lower = limit.lower;
			glink.limit.angular.upper = limit.upper;

			glink.drive.isDrive = joint->IsDrive();
			glink.drive.velocity = joint->GetDriveVelocity();
			glink.drive.maxVelocity = joint->GetDriveVelocity();
			glink.drive.isCycle = joint->IsCycleDrive();
			const Vector2 cycle = joint->GetCycleDrivePeriod();
			glink.drive.period = cycle.x;
			glink.drive.driveAccel = cycle.y;
		}
		break;

		case eJointType::Prismatic:
		{
			glink.type = eJointType::Prismatic;
			const physx::PxJointLinearLimitPair limit = joint->GetLinearLimit();
			glink.limit.linear.isLimit = joint->IsLinearLimit();
			glink.limit.linear.isSpring = limit.stiffness != 0.f;
			glink.limit.linear.value = 0.f;
			glink.limit.linear.lower = limit.lower;
			glink.limit.linear.upper = limit.upper;
			glink.limit.linear.stiffness = limit.stiffness;
			glink.limit.linear.damping = limit.damping;
			glink.limit.linear.contactDistance = limit.contactDistance;
			glink.limit.linear.bounceThreshold = limit.bounceThreshold;
		}
		break;

		case eJointType::Distance:
		{
			glink.type = eJointType::Distance;
			glink.limit.distance.isLimit = joint->IsDistanceLimit();
			const Vector2 limit = joint->GetDistanceLimit();
			glink.limit.distance.minDistance = limit.x;
			glink.limit.distance.maxDistance = limit.y;
		}
		break;

		case eJointType::D6:
		{
			glink.type = eJointType::D6;

			for (int i = 0; i < 6; ++i)
				glink.limit.d6.motion[i] = (int)joint->GetMotion((PxD6Axis::Enum)i);

			for (int i = 0; i < 6; ++i)
			{
				const PxD6JointDrive drive = joint->GetD6Drive((PxD6Drive::Enum)i);
				glink.limit.d6.drive[i].isDrive = drive.stiffness != 0.f;
				glink.limit.d6.drive[i].stiffness = drive.stiffness;
				glink.limit.d6.drive[i].damping = drive.damping;
				glink.limit.d6.drive[i].forceLimit = drive.forceLimit;
			}

			const PxJointLinearLimit linearLimit = joint->GetD6LinearLimit();
			const PxJointAngularLimitPair twistLimit = joint->GetD6TwistLimit();
			const PxJointLimitCone swingLimit = joint->GetD6SwingLimit();

			glink.limit.d6.linear.isLimit = linearLimit.stiffness != 0.f;
			glink.limit.d6.linear.value = linearLimit.value;
			glink.limit.d6.linear.stiffness = linearLimit.stiffness;
			glink.limit.d6.linear.damping = linearLimit.damping;
			glink.limit.d6.linear.contactDistance = linearLimit.contactDistance;
			glink.limit.d6.linear.bounceThreshold = linearLimit.bounceThreshold;

			glink.limit.d6.twist.isLimit = twistLimit.lower != 0.f;
			glink.limit.d6.twist.lower = twistLimit.lower;
			glink.limit.d6.twist.upper = twistLimit.upper;

			glink.limit.d6.swing.isLimit = swingLimit.yAngle != 0.f;
			glink.limit.d6.swing.yAngle = swingLimit.yAngle;
			glink.limit.d6.swing.zAngle = swingLimit.zAngle;

			const std::pair<Vector3, Vector3> ret = joint->GetD6DriveVelocity();
			*(Vector3*)glink.limit.d6.linearVelocity = std::get<0>(ret);
			*(Vector3*)glink.limit.d6.angularVelocity = std::get<1>(ret);
		}
		break;

		default:
			assert(0);
			break;
		}

		m_jointInfos.push_back(glink);
	}
}
