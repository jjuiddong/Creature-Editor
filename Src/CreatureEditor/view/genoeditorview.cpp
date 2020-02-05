
#include "stdafx.h"
#include "genoeditorview.h"

using namespace graphic;


cGenoEditorView::cGenoEditorView(const StrId &name)
	: framework::cDockWindow(name)
	, m_radius(0.5f)
	, m_halfHeight(1.f)
	, m_density(1.f)
	, m_isChangeSelection(false)
{
}

cGenoEditorView::~cGenoEditorView()
{
}


void cGenoEditorView::OnUpdate(const float deltaSeconds)
{
}


void cGenoEditorView::OnRender(const float deltaSeconds)
{
	ImGui::TextUnformatted("GenoType Editor");
	ImGui::Separator();

	CheckChangeSelection();

	RenderSpawnTransform();
	RenderSelectionInfo();
	RenderLinkInfo();

	// final process, recovery
	m_isChangeSelection = false;
}


void cGenoEditorView::RenderSpawnTransform()
{
	ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
	if (ImGui::CollapsingHeader("Create GenoType Node"))
	{
	if (ImGui::Button("SpawnPos"))
		g_geno->ChangeEditMode(eGenoEditMode::SpawnLocation);

		ImGui::TextUnformatted("Pos  ");
		ImGui::SameLine();
		ImGui::DragFloat3("##position", (float*)&g_geno->m_spawnTransform.pos, 0.001f);

		ImGui::TextUnformatted("Dim ");
		ImGui::SameLine();
		ImGui::DragFloat3("##scale", (float*)&g_geno->m_spawnTransform.scale, 0.001f, 0.01f, 1000.f);

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
		int id = -1;
		if (ImGui::Button("Box"))
		{
			id = g_geno->SpawnBox(g_geno->m_spawnTransform.pos);
		}

		ImGui::SameLine();
		if (ImGui::Button("Sphere"))
		{
			id = g_geno->SpawnSphere(g_geno->m_spawnTransform.pos);
		}

		ImGui::SameLine();
		if (ImGui::Button("Capsule"))
		{
			id = g_geno->SpawnCapsule(g_geno->m_spawnTransform.pos);
		}

		ImGui::SameLine();
		if (ImGui::Button("Cylinder"))
		{
			id = g_geno->SpawnCylinder(g_geno->m_spawnTransform.pos);
		}

		ImGui::PopStyleColor(3);

		ImGui::Spacing();
		ImGui::Spacing();
	}
}


void cGenoEditorView::RenderSelectionInfo()
{
	using namespace graphic;
	const int id = (g_geno->m_selects.size() == 1) ? *g_geno->m_selects.begin() : -1;

	ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
	if (ImGui::CollapsingHeader("Select GenoType Node"))
	{
		evc::cGNode *gnode = g_geno->FindGNode(id);
		evc::cGLink *glink = g_geno->FindGLink(id);
		if (!gnode)
			return;

		Transform tfm = gnode->m_transform;

		if (m_isChangeSelection)
		{
			const Vector3 rpy = tfm.rot.Euler();
			m_eulerAngle = Vector3(RAD2ANGLE(rpy.x), RAD2ANGLE(rpy.y), RAD2ANGLE(rpy.z));
		}

		// edit position
		ImGui::TextUnformatted("Pos            ");
		ImGui::SameLine();
		const bool edit0 = ImGui::DragFloat3("##position2", (float*)&tfm.pos, 0.001f);

		// edit scale
		bool edit1 = false;
		bool edit1_1 = false;
		float radius = 0.f, halfHeight = 0.f, height = 0.f; // capsule,cylinder info
		Vector2 dim;
		switch (gnode->m_shape)
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
			dim = gnode->GetCapsuleDimension();
			radius = dim.x;
			halfHeight = dim.y;
			ImGui::TextUnformatted("Radius       ");
			ImGui::SameLine();
			edit1 = ImGui::DragFloat("##scale2", &radius, 0.001f, 0.01f, 1000.f);
			ImGui::TextUnformatted("HalfHeight");
			ImGui::SameLine();
			edit1_1 = ImGui::DragFloat("##halfheight2", &halfHeight, 0.001f, 0.01f, 1000.f);
			break;
		case phys::eShapeType::Cylinder:
			dim = gnode->GetCylinderDimension();
			radius = dim.x;
			height = dim.y;
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

		if (edit0) // position edit
		{
			tfm.pos.y = max(0.f, tfm.pos.y);
			gnode->m_transform.pos = tfm.pos;
		}

		if (edit1 || edit1_1) // scale edit
		{
			// change 3d model dimension
			switch (gnode->m_shape)
			{
			case phys::eShapeType::Box:
				gnode->m_transform = 
					Transform(gnode->m_transform.pos, tfm.scale, gnode->m_transform.rot);
				break;
			case phys::eShapeType::Sphere: gnode->SetSphereRadius(tfm.scale.x); break;
			case phys::eShapeType::Capsule: gnode->SetCapsuleDimension(radius, halfHeight); break;
			case phys::eShapeType::Cylinder: gnode->SetCylinderDimension(radius, height); break;
			}
		}

		if (edit2) // rotation edit
		{
			const Vector3 rpy(ANGLE2RAD(m_eulerAngle.x)
				, ANGLE2RAD(m_eulerAngle.y), ANGLE2RAD(m_eulerAngle.z));
			tfm.rot.Euler(rpy);
			gnode->m_transform.rot = tfm.rot;
		}

		ImGui::Separator();
		RenderSelectNodeLinkInfo(id);

		ImGui::Spacing();
		ImGui::Spacing();
	}//~CollapsingHeader
}


// show select actor joint information
void cGenoEditorView::RenderSelectNodeLinkInfo(const int id)
{
	using namespace graphic;

	evc::cGNode *gnode = g_geno->FindGNode(id);
	if (!gnode)
		return;

	set<evc::cGLink*> rmLinks; // remove links

	// show all link information contained genotype node
	ImGui::TextUnformatted("GenoType Link Information");
	for (uint i = 0; i < gnode->m_links.size(); ++i)
	{
		evc::cGLink *glink = gnode->m_links[i];

		const ImGuiTreeNodeFlags node_flags = 0;
		ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
		if (ImGui::TreeNodeEx((void*)((int)glink + i), node_flags, "link-%d", i + 1))
		{
			const char *jointStr[] = { "Fixed", "Spherical", "Revolute", "Prismatic", "Distance", "D6" };
			ImGui::Text("%s JointType", jointStr[(int)glink->m_type]);

			if (ImGui::Button("Pivot Setting"))
			{
				g_geno->ChangeEditMode(eGenoEditMode::Pivot0);
				g_geno->m_selLink = glink;
				g_geno->m_gizmo.m_type = graphic::eGizmoEditType::None;
			}

			ImGui::SameLine(150);
			if (ImGui::Button("Apply Pivot"))
			{
				//cJointRenderer *jointRenderer = g_geno->FindJointRenderer(joint);
				//if (jointRenderer)
				//	jointRenderer->ApplyPivot(*g_geno->m_physics);
				g_geno->ChangeEditMode(eGenoEditMode::Normal);
			}

			switch (glink->m_type)
			{
			case phys::eJointType::Fixed: break;
			case phys::eJointType::Spherical: RenderSphericalJointSetting(glink); break;
			case phys::eJointType::Revolute: RenderRevoluteJointSetting(glink); break;
			case phys::eJointType::Prismatic: RenderPrismaticJointSetting(glink); break;
			case phys::eJointType::Distance: RenderDistanceJointSetting(glink); break;
			case phys::eJointType::D6: RenderD6JointSetting(glink); break;
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
			text.Format("Remove Loint-%d", i + 1);
			if (ImGui::Button(text.c_str()))
			{
				rmLinks.insert(glink);
			}
			ImGui::PopStyleColor(3);

			ImGui::Separator();
			ImGui::TreePop();
		}
	}

	// remove link?
	if (!rmLinks.empty())
	{
		// clear selection
		g_geno->ChangeEditMode(eGenoEditMode::Normal);
		g_geno->m_selLink = nullptr;
	}
	for (auto &link : rmLinks)
		g_geno->RemoveGLink(link);
}


void cGenoEditorView::RenderLinkInfo()
{
	ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
	if (ImGui::CollapsingHeader("Create Link"))
	{
		if (CheckCancelUIJoint())
			g_geno->ChangeEditMode(eGenoEditMode::Normal); // cancel ui

		if (g_geno->m_selects.size() == 2)
		{
			// check already connection
			{
				auto it = g_geno->m_selects.begin();
				const int id0 = *it++;
				const int id1 = *it++;

				evc::cGNode *gnode0 = g_geno->FindGNode(id0);
				evc::cGNode *gnode1 = g_geno->FindGNode(id1);
				if (!gnode0 || !gnode1)
					return;
				for (auto &j1 : gnode0->m_links)
					for (auto &j2 : gnode1->m_links)
						if (j1 == j2)
							return; // already connection link
			}

			if ((g_geno->GetEditMode() != eGenoEditMode::Pivot0)
				&& (g_geno->GetEditMode() != eGenoEditMode::Pivot1)
				&& (g_geno->GetEditMode() != eGenoEditMode::Revolute)
				&& (g_geno->GetEditMode() != eGenoEditMode::JointEdit))
			{
				auto it = g_geno->m_selects.begin();
				g_geno->m_pairId0 = *it++;
				g_geno->m_pairId1 = *it++;
				g_geno->ChangeEditMode(eGenoEditMode::JointEdit); // joint edit mode
			}
		}

		if ((g_geno->GetEditMode() == eGenoEditMode::Pivot0)
			|| (g_geno->GetEditMode() == eGenoEditMode::Pivot1)
			|| (g_geno->GetEditMode() == eGenoEditMode::Revolute)
			|| (g_geno->GetEditMode() == eGenoEditMode::JointEdit))
		{
			ImGui::Checkbox("Fix Selection", &g_geno->m_fixJointSelection);

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


void cGenoEditorView::RenderFixedJoint()
{
	const int id0 = g_geno->m_pairId0;
	const int id1 = g_geno->m_pairId1;
	evc::cGNode *gnode0 = g_geno->FindGNode(id0);
	evc::cGNode *gnode1 = g_geno->FindGNode(id1);
	if (!gnode0 || !gnode1)
		return;

	const char *axisStr = "X\0Y\0Z\0\0";
	const static Vector3 axis[3] = { Vector3(1,0,0), Vector3(0,1,0), Vector3(0,0,1) };
	static int axisIdx = 0;
	const bool editAxis = ImGui::Combo("Joint Axis", &axisIdx, axisStr);

	UpdateUILink(gnode0, gnode1, editAxis, axis[axisIdx]);

	if (ImGui::Button("Pivot Setting"))
	{
		g_geno->ChangeEditMode(eGenoEditMode::Pivot0);
		g_geno->m_selLink = &g_geno->m_uiLink;
		g_geno->m_gizmo.m_type = graphic::eGizmoEditType::None;
	}

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.4f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.8f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0, 1));
	if (ImGui::Button("Create Fixed Link"))
	{
		const Transform pivot0 = g_geno->m_uiLink.GetPivotWorldTransform(0);
		const Transform pivot1 = g_geno->m_uiLink.GetPivotWorldTransform(1);

		evc::cGLink *link = new evc::cGLink();
		link->CreateFixed(gnode0, pivot0.pos, gnode1, pivot1.pos);
		g_geno->AddGLink(link);

		g_geno->ChangeEditMode(eGenoEditMode::Normal);
	}
	ImGui::PopStyleColor(3);
}


void cGenoEditorView::RenderSphericalJoint()
{
	using namespace physx;

	const int id0 = g_geno->m_pairId0;
	const int id1 = g_geno->m_pairId1;
	evc::cGNode *gnode0 = g_geno->FindGNode(id0);
	evc::cGNode *gnode1 = g_geno->FindGNode(id1);
	if (!gnode0 || !gnode1)
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

	UpdateUILink(gnode0, gnode1, editAxis, axis[axisIdx]);

	if (ImGui::Button("Pivot Setting"))
	{
		g_geno->ChangeEditMode(eGenoEditMode::Pivot0);
		g_geno->m_selLink = &g_geno->m_uiLink;
		g_geno->m_gizmo.m_type = graphic::eGizmoEditType::None;
	}

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.4f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.8f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0, 1));
	if (ImGui::Button("Create Spherical Joint"))
	{
		const Transform pivot0 = g_geno->m_uiLink.GetPivotWorldTransform(0);
		const Transform pivot1 = g_geno->m_uiLink.GetPivotWorldTransform(1);

		evc::cGLink *link = new evc::cGLink();
		link->CreateSpherical(gnode0, pivot0.pos, gnode1, pivot1.pos);
		g_geno->AddGLink(link);

		//if (isLimit)
		//{
		//	joint->SetConeLimit(PxJointLimitCone(limit.x, limit.y, 0.01f));
		//	joint->SetSphericalJointFlag(PxSphericalJointFlag::eLIMIT_ENABLED, true);
		//}

		g_geno->ChangeEditMode(eGenoEditMode::Normal);
	}
	ImGui::PopStyleColor(3);
}


void cGenoEditorView::RenderRevoluteJoint()
{
	using namespace physx;

	const int id0 = g_geno->m_pairId0;
	const int id1 = g_geno->m_pairId1;
	evc::cGNode *gnode0 = g_geno->FindGNode(id0);
	evc::cGNode *gnode1 = g_geno->FindGNode(id1);
	if (!gnode0 || !gnode1)
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
	ImGui::DragFloat("Cycle Period", &cycleDrivePeriod, 0.001f);
	ImGui::DragFloat("Drive Accleration", &cycleDriveVelocityAccel, 0.001f);

	if (ImGui::Button("Pivot Setting"))
	{
		g_geno->ChangeEditMode(eGenoEditMode::Pivot0);
		g_geno->m_selLink = &g_geno->m_uiLink;
		g_geno->m_gizmo.m_type = graphic::eGizmoEditType::None;
	}

	UpdateUILink(gnode0, gnode1, editAxis, axis[axisIdx]);

	ImGui::Spacing();
	ImGui::Spacing();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.4f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.8f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0, 1));
	if (ImGui::Button("Create Revolute Joint"))
	{
		const Transform pivot0 = g_geno->m_uiLink.GetPivotWorldTransform(0);
		const Transform pivot1 = g_geno->m_uiLink.GetPivotWorldTransform(1);

		evc::cGLink *link = new evc::cGLink();
		link->CreateRevolute(gnode0, pivot0.pos, gnode1, pivot1.pos, axis[axisIdx]);
		g_geno->AddGLink(link);

		//joint->EnableAngularLimit(isLimit);

		//if (isLimit)
		//	joint->SetAngularLimit(PxJointAngularLimitPair(limit.x, limit.y, 0.01f));
		//else
		//	joint->SetAngularLimit(PxJointAngularLimitPair(-PxPi * 2.f, PxPi*2.f, 0.01f));
		////joint->SetAngularLimit(PxJointAngularLimitPair(FLT_MIN, FLT_MAX));

		//joint->EnableDrive(isDrive);
		//if (isDrive)
		//	joint->SetDriveVelocity(velocity);

		//joint->EnableCycleDrive(isDrive && isCycleDrive);
		//if (isDrive && isCycleDrive)
		//	joint->SetCycleDrivePeriod(cycleDrivePeriod, cycleDriveVelocityAccel);

		g_geno->ChangeEditMode(eGenoEditMode::Normal);
	}
	ImGui::PopStyleColor(3);
}


void cGenoEditorView::RenderPrismaticJoint()
{
	using namespace physx;

	const int id0 = g_geno->m_pairId0;
	const int id1 = g_geno->m_pairId1;
	evc::cGNode *gnode0 = g_geno->FindGNode(id0);
	evc::cGNode *gnode1 = g_geno->FindGNode(id1);
	if (!gnode0 || !gnode1)
		return;

	static Vector2 limit1(1.f, 2.f); // lower, upper
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
		g_geno->ChangeEditMode(eGenoEditMode::Pivot0);
		g_geno->m_selLink = &g_geno->m_uiLink;
		g_geno->m_gizmo.m_type = graphic::eGizmoEditType::None;
	}

	UpdateUILink(gnode0, gnode1, editAxis, axis[axisIdx]);

	ImGui::Spacing();
	ImGui::Spacing();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.4f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.8f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0, 1));
	if (ImGui::Button("Create Prismatic Joint"))
	{
		const Transform pivot0 = g_geno->m_uiLink.GetPivotWorldTransform(0);
		const Transform pivot1 = g_geno->m_uiLink.GetPivotWorldTransform(1);

		evc::cGLink *link = new evc::cGLink();
		link->CreatePrismatic(gnode0, pivot0.pos, gnode1, pivot1.pos, axis[axisIdx]);
		g_geno->AddGLink(link);

		//joint->EnableLinearLimit(isLimit);

		//if (isLimit)
		//{
		//	if (isSpring)
		//	{
		//		joint->SetLinearLimit(PxJointLinearLimitPair(limit1.x, limit1.y
		//			, physx::PxSpring(limit2.x, limit2.y)));
		//	}
		//	else
		//	{
		//		PxTolerancesScale scale;
		//		scale.length = limit3.x;
		//		joint->SetLinearLimit(PxJointLinearLimitPair(scale, limit1.x, limit1.y));
		//	}
		//}

		g_geno->ChangeEditMode(eGenoEditMode::Normal);
	}
	ImGui::PopStyleColor(3);
}


void cGenoEditorView::RenderDistanceJoint()
{
	using namespace physx;

	const int id0 = g_geno->m_pairId0;
	const int id1 = g_geno->m_pairId1;
	evc::cGNode *gnode0 = g_geno->FindGNode(id0);
	evc::cGNode *gnode1 = g_geno->FindGNode(id1);
	if (!gnode0 || !gnode1)
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

	UpdateUILink(gnode0, gnode1, editAxis, axis[axisIdx]);

	if (ImGui::Button("Pivot Setting"))
	{
		g_geno->ChangeEditMode(eGenoEditMode::Pivot0);
		g_geno->m_selLink = &g_geno->m_uiLink;
		g_geno->m_gizmo.m_type = graphic::eGizmoEditType::None;
	}

	ImGui::Spacing();
	ImGui::Spacing();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.4f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.8f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0, 1));
	if (ImGui::Button("Create Distance Joint"))
	{
		const Transform pivot0 = g_geno->m_uiLink.GetPivotWorldTransform(0);
		const Transform pivot1 = g_geno->m_uiLink.GetPivotWorldTransform(1);

		evc::cGLink *link = new evc::cGLink();
		link->CreateDistance(gnode0, pivot0.pos, gnode1, pivot1.pos);
		g_geno->AddGLink(link);

		//if (isLimit)
		//{
		//	joint->EnableDistanceLimit(isLimit);
		//	joint->SetDistanceLimit(limit.x, limit.y);
		//}

		g_geno->ChangeEditMode(eGenoEditMode::Normal);
	}
	ImGui::PopStyleColor(3);
}


// 
void cGenoEditorView::RenderD6Joint()
{
	using namespace physx;

	const int id0 = g_geno->m_pairId0;
	const int id1 = g_geno->m_pairId1;
	evc::cGNode *gnode0 = g_geno->FindGNode(id0);
	evc::cGNode *gnode1 = g_geno->FindGNode(id1);
	if (!gnode0 || !gnode1)
		return;

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
		g_geno->ChangeEditMode(eGenoEditMode::Pivot0);
		g_geno->m_selLink = &g_geno->m_uiLink;
		g_geno->m_gizmo.m_type = graphic::eGizmoEditType::None;
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.6f, 0.1f, 1.f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.8f, 0.1f, 1.f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0.1f, 1.f));
	if (ImGui::Button("Create D6 Joint"))
	{
		const Transform pivot0 = g_geno->m_uiLink.GetPivotWorldTransform(0);
		const Transform pivot1 = g_geno->m_uiLink.GetPivotWorldTransform(1);

		evc::cGLink *link = new evc::cGLink();
		link->CreateD6(gnode0, pivot0.pos, gnode1, pivot1.pos);
		g_geno->AddGLink(link);

		//for (int i = 0; i < 6; ++i)
		//	joint->SetMotion((PxD6Axis::Enum)i, (PxD6Motion::Enum)motionVal[i]);

		//for (int i = 0; i < 6; ++i)
		//{
		//	if (driveVal[i])
		//	{
		//		joint->SetD6Drive((PxD6Drive::Enum)i
		//			, physx::PxD6JointDrive(driveConfigs[i].stiffness
		//				, driveConfigs[i].damping
		//				, driveConfigs[i].forceLimit
		//				, driveConfigs[i].accel));
		//	}
		//}

		//if (isLinearLimit)
		//	joint->SetD6LinearLimit(linearLimit);
		//if (isTwistLimit)
		//	joint->SetD6TwistLimit(twistLimit);
		//if (isSwingLimit)
		//	joint->SetD6SwingLimit(swingLimit);

		//joint->SetD6DriveVelocity(linearDriveVelocity, angularDriveVelocity);

		g_geno->ChangeEditMode(eGenoEditMode::Normal);
	}
	ImGui::PopStyleColor(3);
	ImGui::Spacing();
	ImGui::Spacing();
}


void cGenoEditorView::RenderRevoluteJointSetting(evc::cGLink *link)
{
	using namespace physx;

	//static bool isLimit = false;
	//static PxJointAngularLimitPair limit(-PxPi / 2.f, PxPi / 2.f, 0.01f);
	//static bool isDrive = false;
	//static float driveVelocity = 0.f;

	//if (m_isChangeSelection)
	//{
	//	isLimit = joint->IsAngularLimit();
	//	limit = joint->GetAngularLimit();
	//	isDrive = joint->IsDrive();
	//	driveVelocity = joint->GetDriveVelocity();
	//}

	//ImGui::Text("Angular Limit (Radian)");
	//ImGui::SameLine();
	//ImGui::Checkbox("##Limit", &isLimit);
	//ImGui::Indent(30);
	//ImGui::PushItemWidth(150);
	//ImGui::DragFloat("Lower Angle", &limit.lower, 0.001f);
	//ImGui::DragFloat("Upper Angle", &limit.upper, 0.001f);
	//ImGui::PopItemWidth();
	//ImGui::Unindent(30);

	//ImGui::TextUnformatted("Drive");
	//ImGui::SameLine();
	//ImGui::Checkbox("##Drive", &isDrive);
	//ImGui::Indent(30);
	//ImGui::PushItemWidth(150);
	//ImGui::DragFloat("Velocity", &driveVelocity, 0.001f);

	//const char *axisStr = "X\0Y\0Z\0\0";
	//const static Vector3 axis[3] = { Vector3(1,0,0), Vector3(0,1,0), Vector3(0,0,1) };
	//static int axisIdx = 0;
	//ImGui::Combo("Revolute Axis", &axisIdx, axisStr);
	//ImGui::PopItemWidth();
	//ImGui::Unindent(30);

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.4f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.8f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0, 1));
	if (ImGui::Button("Apply Option"))
	{
		//joint->EnableAngularLimit(isLimit);
		//if (isLimit)
		//	joint->SetAngularLimit(limit);

		//joint->EnableDrive(isDrive);
		//if (isDrive)
		//	joint->SetDriveVelocity(driveVelocity);

		//joint->m_actor0->WakeUp();
		//joint->m_actor1->WakeUp();
	}
	ImGui::PopStyleColor(3);
}


void cGenoEditorView::RenderPrismaticJointSetting(evc::cGLink *link)
{
	using namespace physx;

	//static Vector2 limit1(1.f, 2.f); // lower, upper
	//static Vector2 limit2(10.f, 0.0f); // stiffness, damping (spring)
	//static Vector2 limit3(3.f, 0.0f); // length
	//static bool isLimit = false;
	//static bool isSpring = true;

	//if (m_isChangeSelection)
	//{
	//	isLimit = joint->IsLinearLimit();
	//	PxJointLinearLimitPair limit(0, 0, PxSpring(0, 0));
	//	limit = joint->GetLinearLimit();
	//	limit1.x = isLimit ? limit.lower : 0.f;
	//	limit1.y = isLimit ? limit.upper : 0.f;
	//	limit2.x = limit.stiffness;
	//	limit2.y = limit.damping;
	//	//limit3.x = ?;
	//	isSpring = limit.stiffness != 0.f;
	//}

	//ImGui::Separator();
	//ImGui::Text("Linear Limit");
	//ImGui::SameLine();
	//ImGui::Checkbox("##Limit", &isLimit);
	//ImGui::Indent(30);
	//ImGui::PushItemWidth(150);
	//ImGui::DragFloat("Lower Limit", &limit1.x, 0.001f);
	//ImGui::DragFloat("Upper Limit", &limit1.y, 0.001f);
	//ImGui::PopItemWidth();
	//ImGui::Unindent(30);

	//ImGui::Text("Spring");
	//ImGui::SameLine();
	//ImGui::Checkbox("##Spring", &isSpring);
	//ImGui::Indent(30);
	//ImGui::PushItemWidth(150);
	//ImGui::DragFloat("Stiffness", &limit2.x, 0.001f);
	//ImGui::DragFloat("Damping", &limit2.y, 0.001f);
	//ImGui::DragFloat("Length (linear)", &limit3.x, 0.001f);
	//ImGui::PopItemWidth();
	//ImGui::Unindent(30);

	//const char *axisStr = "X\0Y\0Z\0\0";
	//const static Vector3 axis[3] = { Vector3(1,0,0), Vector3(0,1,0), Vector3(0,0,1) };
	//static int axisIdx = 0;
	//const bool editAxis = ImGui::Combo("Prismatic Axis", &axisIdx, axisStr);

	//ImGui::Spacing();
	//ImGui::Spacing();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.4f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.8f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0, 1));
	if (ImGui::Button("Apply Option"))
	{
		//joint->EnableLinearLimit(isLimit);

		//if (isLimit)
		//{
		//	if (isSpring)
		//	{
		//		joint->SetLinearLimit(PxJointLinearLimitPair(limit1.x, limit1.y
		//			, physx::PxSpring(limit2.x, limit2.y)));
		//	}
		//	else
		//	{
		//		PxTolerancesScale scale;
		//		scale.length = limit3.x;
		//		joint->SetLinearLimit(PxJointLinearLimitPair(scale, limit1.x, limit1.y));
		//	}
		//}
	}
	ImGui::PopStyleColor(3);
}


void cGenoEditorView::RenderDistanceJointSetting(evc::cGLink *link)
{
	//static Vector2 limit(0.f, 2.f);
	//static bool isLimit = false;

	//if (m_isChangeSelection)
	//{
	//	isLimit = joint->IsDistanceLimit();
	//	limit = joint->GetDistanceLimit();
	//}

	//ImGui::Text("Distance Limit");
	//ImGui::SameLine();
	//ImGui::Checkbox("##Limit", &isLimit);
	//ImGui::DragFloat("min distance", &limit.x, 0.001f);
	//ImGui::DragFloat("max distance", &limit.y, 0.001f);

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.4f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.8f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0, 1));
	if (ImGui::Button("Apply Option"))
	{
		//joint->EnableDistanceLimit(isLimit);
		//if (isLimit)
		//	joint->SetDistanceLimit(limit.x, limit.y);
	}
	ImGui::PopStyleColor(3);
}


void cGenoEditorView::RenderD6JointSetting(evc::cGLink *link)
{
	//using namespace physx;

	//const char *motionAxis[6] = { "X         ", "Y         ", "Z          "
	//	, "Twist   ", "Swing1", "Swing2" };
	//const char *motionStr = "Lock\0Limit\0Free\0\0";
	//const char *driveAxis[6] = { "X         ", "Y         ", "Z          "
	//	, "Swing   ", "Twist", "Slerp" };

	//static int motionVal[6] = { 0, 0, 0, 0, 0, 0 };
	//static bool driveVal[6] = { 0, 0, 0, 0, 0, 0 };
	//struct sDriveParam
	//{
	//	float stiffness;
	//	float damping;
	//	float forceLimit;
	//	bool accel;
	//};
	//static sDriveParam driveConfigs[6] = {
	//	{10.f, 0.f, PX_MAX_F32, true}, // X
	//	{10.f, 0.f, PX_MAX_F32, true}, // Y
	//	{10.f, 0.f, PX_MAX_F32, true}, // Z
	//	{10.f, 0.f, PX_MAX_F32, true}, // Twist
	//	{10.f, 0.f, PX_MAX_F32, true}, // Swing1
	//	{10.f, 0.f, PX_MAX_F32, true}, // Swing2
	//};
	//static PxJointLinearLimit linearLimit(1.f, PxSpring(10.f, 0.f));
	//static PxJointAngularLimitPair twistLimit(-PxPi / 2.f, PxPi / 2.f);
	//static PxJointLimitCone swingLimit(-PxPi / 2.f, PxPi / 2.f);
	//static bool isLinearLimit = false;
	//static bool isTwistLimit = false;
	//static bool isSwingLimit = false;
	//static Vector3 linearDriveVelocity(0, 0, 0);
	//static Vector3 angularDriveVelocity(0, 0, 0);

	//if (m_isChangeSelection)
	//{
	//	for (int i = 0; i < 6; ++i)
	//		motionVal[i] = (int)joint->GetMotion((PxD6Axis::Enum)i);

	//	for (int i = 0; i < 6; ++i)
	//	{
	//		const PxD6JointDrive drive = joint->GetD6Drive((PxD6Drive::Enum)i);
	//		driveConfigs[i].stiffness = drive.stiffness;
	//		driveConfigs[i].damping = drive.damping;
	//		driveConfigs[i].forceLimit = drive.forceLimit;
	//		driveVal[i] = drive.stiffness != 0.f;
	//	}

	//	linearLimit = joint->GetD6LinearLimit();
	//	twistLimit = joint->GetD6TwistLimit();
	//	swingLimit = joint->GetD6SwingLimit();
	//	isLinearLimit = linearLimit.stiffness != 0.f;
	//	isTwistLimit = twistLimit.lower != 0.f;
	//	isSwingLimit = swingLimit.yAngle != 0.f;

	//	std::pair<Vector3, Vector3> ret = joint->GetD6DriveVelocity();
	//	linearDriveVelocity = std::get<0>(ret);
	//	angularDriveVelocity = std::get<1>(ret);
	//}

	//// Motion
	//ImGui::SetNextTreeNodeOpen(true, ImGuiCond_FirstUseEver);
	//if (ImGui::TreeNode("Motion"))
	//{
	//	for (int i = 0; i < 6; ++i)
	//	{
	//		int id = i * 100;
	//		ImGui::PushItemWidth(200);
	//		ImGui::TextUnformatted(motionAxis[i]);
	//		ImGui::SameLine();
	//		ImGui::PushID(id++);
	//		if (ImGui::Combo("##Motion", &motionVal[i], motionStr))
	//		{
	//			if (driveVal[i])
	//				driveVal[i] = (0 != motionVal[i]); // motion lock -> drive lock
	//		}
	//		ImGui::PopID();
	//		ImGui::PopItemWidth();
	//	}
	//	ImGui::TreePop();
	//}
	//ImGui::Separator();
	//// Drive
	//ImGui::SetNextTreeNodeOpen(true, ImGuiCond_FirstUseEver);
	//if (ImGui::TreeNode("Drive"))
	//{
	//	for (int i = 0; i < 6; ++i)
	//	{
	//		int id = i * 1000;
	//		ImGui::Checkbox(driveAxis[i], &driveVal[i]);

	//		if (driveVal[i])
	//		{
	//			sDriveParam &drive = driveConfigs[i];

	//			ImGui::Indent(30);
	//			ImGui::PushItemWidth(150);

	//			ImGui::TextUnformatted("Stiffness   ");
	//			ImGui::SameLine();
	//			ImGui::PushID(id++);
	//			ImGui::DragFloat("##Stiffness", &drive.stiffness);
	//			ImGui::PopID();

	//			ImGui::TextUnformatted("Dampping");
	//			ImGui::SameLine();
	//			ImGui::PushID(id++);
	//			ImGui::DragFloat("##Dampping", &drive.damping);
	//			ImGui::PopID();

	//			ImGui::TextUnformatted("Force Limit");
	//			ImGui::SameLine();
	//			ImGui::PushID(id++);
	//			ImGui::DragFloat("##Force Limit", &drive.forceLimit);
	//			ImGui::PopID();

	//			ImGui::TextUnformatted("Accel");
	//			ImGui::SameLine();
	//			ImGui::PushID(id++);
	//			ImGui::Checkbox("##Accel", &drive.accel);
	//			ImGui::PopID();

	//			ImGui::PopItemWidth();
	//			ImGui::Unindent(30);

	//			ImGui::Separator();
	//		}//~drive

	//		ImGui::Spacing();
	//	}//~for drive axis 6
	//	ImGui::TreePop();
	//}//~drive tree node

	//ImGui::Separator();
	//ImGui::TextUnformatted("Linear Drive Velocity");
	//ImGui::DragFloat3("##Linear Drive Velocity", (float*)&linearDriveVelocity, 0.001f, 0.f, 1000.f);
	//ImGui::TextUnformatted("Angular Drive Velocity");
	//ImGui::DragFloat3("##Angular Drive Velocity", (float*)&angularDriveVelocity, 0.001f, 0.f, 1000.f);
	//ImGui::Separator();

	//// linear limit
	//{
	//	ImGui::TextUnformatted("Linear Limit");
	//	ImGui::SameLine();
	//	ImGui::Checkbox("##Linear Limit", &isLinearLimit);
	//	if (isLinearLimit)
	//	{
	//		ImGui::Indent(30);
	//		ImGui::PushItemWidth(150);
	//		ImGui::DragFloat("Extend", &linearLimit.value, 0.001f);
	//		ImGui::DragFloat("Stiffness", &linearLimit.stiffness, 0.001f);
	//		ImGui::DragFloat("Damping", &linearLimit.damping, 0.001f);
	//		ImGui::PopItemWidth();
	//		ImGui::Unindent(30);
	//	}
	//}

	//// twist limit
	//{
	//	ImGui::TextUnformatted("Twist Limit ");
	//	ImGui::SameLine();
	//	ImGui::Checkbox("##Twist Limit", &isTwistLimit);
	//	if (isTwistLimit)
	//	{
	//		ImGui::Indent(30);
	//		ImGui::PushItemWidth(150);
	//		ImGui::DragFloat("Lower Angle", &twistLimit.lower, 0.001f);
	//		ImGui::DragFloat("Upper Angle", &twistLimit.upper, 0.001f);
	//		ImGui::PopItemWidth();
	//		ImGui::Unindent(30);
	//	}
	//}

	//// swing limit
	//{
	//	ImGui::TextUnformatted("Swing Limit");
	//	ImGui::SameLine();
	//	ImGui::Checkbox("##Swing Limit", &isSwingLimit);
	//	if (isSwingLimit)
	//	{
	//		ImGui::Indent(30);
	//		ImGui::PushItemWidth(150);
	//		ImGui::DragFloat("Y Angle", &swingLimit.yAngle, 0.001f);
	//		ImGui::DragFloat("Z Angle", &swingLimit.zAngle, 0.001f);
	//		ImGui::PopItemWidth();
	//		ImGui::Unindent(30);
	//	}
	//}

	//ImGui::Spacing();
	//ImGui::Separator();
	//ImGui::Spacing();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.4f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.8f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0, 1));
	if (ImGui::Button("Apply Option"))
	{
		//for (int i = 0; i < 6; ++i)
		//	joint->SetMotion((PxD6Axis::Enum)i, (PxD6Motion::Enum)motionVal[i]);

		//for (int i = 0; i < 6; ++i)
		//{
		//	if (driveVal[i])
		//	{
		//		joint->SetD6Drive((PxD6Drive::Enum)i
		//			, physx::PxD6JointDrive(driveConfigs[i].stiffness
		//				, driveConfigs[i].damping
		//				, driveConfigs[i].forceLimit
		//				, driveConfigs[i].accel));
		//	}
		//}

		//if (isLinearLimit)
		//	joint->SetD6LinearLimit(linearLimit);
		//if (isTwistLimit)
		//	joint->SetD6TwistLimit(twistLimit);
		//if (isSwingLimit)
		//	joint->SetD6SwingLimit(swingLimit);

		//joint->SetD6DriveVelocity(linearDriveVelocity, angularDriveVelocity);
	}
	ImGui::PopStyleColor(3);
}


void cGenoEditorView::RenderSphericalJointSetting(evc::cGLink *link)
{
	//using namespace physx;

	//static bool isLimit = false;
	//static PxJointLimitCone limit(-PxPi / 2.f, PxPi / 2.f, 0.01f);

	//if (m_isChangeSelection)
	//{
	//	isLimit = joint->IsConeLimit();
	//	limit = joint->GetConeLimit();
	//}

	//ImGui::Text("Limit Cone (Radian)");
	//ImGui::SameLine();
	//ImGui::Checkbox("##Limit", &isLimit);
	//ImGui::Indent(30);
	//ImGui::PushItemWidth(150);
	//ImGui::DragFloat("Y Angle", &limit.yAngle, 0.001f);
	//ImGui::DragFloat("Z Angle", &limit.zAngle, 0.001f);
	//ImGui::PopItemWidth();
	//ImGui::Unindent(30);

	////const char *axisStr = "X\0Y\0Z\0\0";
	////const static Vector3 axis[3] = { Vector3(1,0,0), Vector3(0,1,0), Vector3(0,0,1) };
	////static int axisIdx = 0;
	////ImGui::Combo("Revolute Axis", &axisIdx, axisStr);

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.4f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.8f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0, 1));
	if (ImGui::Button("Apply Option"))
	{
		//joint->SetConeLimit(limit);
		//joint->EnableConeLimit(isLimit);

		//joint->m_actor0->WakeUp();
		//joint->m_actor1->WakeUp();
	}
	ImGui::PopStyleColor(3);
}


// check cancel show ui joint
// return true if change state
bool cGenoEditorView::CheckCancelUIJoint()
{
	if (!g_geno->m_showUILink)
	{
		// fixed, spherical, distance, d6 joint?
		// check cancel ui joint
		if ((g_geno->m_selects.size() != 2)
			&& (g_geno->GetEditMode() == eGenoEditMode::JointEdit))
			goto $cancel;
		return false;
	}

	if ((g_geno->m_selects.size() != 1)
		&& (g_geno->m_selects.size() != 2)
		&& (g_geno->m_selects.size() != 3))
	{
		goto $cancel;
	}
	else if (g_geno->m_selects.size() == 1)
	{
		evc::cGLink *glink = g_geno->FindGLink(g_geno->m_selects[0]);
		if (!glink) // ui link selection?
			goto $cancel;
	}
	else
	{
		// check genotype node x 2
		if (g_geno->m_selects.size() == 2)
		{
			evc::cGNode *gnode0 = g_geno->FindGNode(g_geno->m_selects[0]);
			evc::cGNode *gnode1 = g_geno->FindGNode(g_geno->m_selects[1]);
			evc::cGLink *glink0 = g_geno->FindGLink(g_geno->m_selects[0]);
			evc::cGLink *glink1 = g_geno->FindGLink(g_geno->m_selects[1]);
			if (!gnode0 || !gnode1)
				goto $cancel;
			//if (sync0->joint || sync1->joint)
			//	goto $cancel;
		}
		else if (g_geno->m_selects.size() == 3)
		{
			// check link, genotype node x 2
			evc::cGNode *nodes[3] = { nullptr, nullptr, nullptr };
			nodes[0] = g_geno->FindGNode(g_geno->m_selects[0]);
			nodes[1] = g_geno->FindGNode(g_geno->m_selects[1]);
			nodes[2] = g_geno->FindGNode(g_geno->m_selects[2]);
			if (!nodes[0] || !nodes[1] || !nodes[2])
				goto $cancel;

			//int jointIdx = -1;
			//for (int i = 0; i < 3; ++i)
			//	if (syncs[i]->joint)
			//		jointIdx = i;

			//if (jointIdx < 0)
			//	goto $cancel;

			//int i0 = (jointIdx + 1) % 3;
			//int i1 = (jointIdx + 2) % 3;
			//if (((syncs[jointIdx]->joint->m_actor0 == syncs[i0]->actor)
			//	&& (syncs[jointIdx]->joint->m_actor1 == syncs[i1]->actor))
			//	|| ((syncs[jointIdx]->joint->m_actor1 == syncs[i0]->actor)
			//		&& (syncs[jointIdx]->joint->m_actor0 == syncs[i1]->actor)))
			//{
			//	// ok
			//}
			//else
			//{
			//	goto $cancel;
			//}
		}
	}

	return false;

$cancel:
	g_geno->m_showUILink = false;
	g_geno->RemoveGLink(&g_geno->m_uiLink);
	return true;
}


// check change selection
void cGenoEditorView::CheckChangeSelection()
{
	if (m_oldSelects.size() != g_geno->m_selects.size())
	{
		goto $change;
	}
	else if (!m_oldSelects.empty())
	{
		// compare select ids
		for (uint i = 0; i < g_geno->m_selects.size(); ++i)
			if (m_oldSelects[i] != g_geno->m_selects[i])
				goto $change;
	}

	return; // no change

$change:
	m_isChangeSelection = true;
	m_oldSelects = g_geno->m_selects;
}


// update ui link information
void cGenoEditorView::UpdateUILink(evc::cGNode *gnode0, evc::cGNode *gnode1
	, const bool editAxis, const Vector3 &revoluteAxis)
{
	// update ui link (once process)
	if (!g_geno->m_showUILink || editAxis || m_isChangeSelection)
	{
		// already register link?
		if (!g_geno->FindGLink(g_geno->m_uiLink.m_id))
			g_geno->AddGLink(&g_geno->m_uiLink);

		// change selection?
		const bool isPrevSelection = (((g_geno->m_uiLink.m_gnode0 == gnode0)
			&& (g_geno->m_uiLink.m_gnode1 == gnode1))
			|| ((g_geno->m_uiLink.m_gnode1 == gnode0)
				&& (g_geno->m_uiLink.m_gnode0 == gnode1)));

		g_geno->m_showUILink = true;

		{
			g_geno->m_uiLink.m_type = phys::eJointType::Revolute;
			g_geno->m_uiLink.m_gnode0 = gnode0;
			g_geno->m_uiLink.m_gnode1 = gnode1;
			g_geno->m_uiLink.m_nodeLocal0 = gnode0->m_transform;
			g_geno->m_uiLink.m_nodeLocal1 = gnode1->m_transform;
			g_geno->m_uiLink.m_revoluteAxis = revoluteAxis;

			g_geno->m_uiLink.m_pivots[0].dir = Vector3::Zeroes;
			g_geno->m_uiLink.m_pivots[0].len = 0;
			g_geno->m_uiLink.m_pivots[1].dir = Vector3::Zeroes;
			g_geno->m_uiLink.m_pivots[1].len = 0;

			//g_geno->m_uiLinkRenderer.m_joint = &g_geno->m_uiJoint;
			//g_geno->m_uiJointRenderer.m_sync0 = sync0;
			//g_geno->m_uiJointRenderer.m_sync1 = sync1;

			// auto pivot position targeting
			const Vector3 pos0 = gnode0->m_transform.pos;
			const Vector3 pos1 = gnode1->m_transform.pos;
			const Ray ray0(pos1, revoluteAxis);
			const Ray ray1(pos1, -revoluteAxis);

			float dist0 = 0, dist1 = 0;
			gnode0->Picking(ray0, graphic::eNodeType::MODEL, false, &dist0);
			gnode1->Picking(ray1, graphic::eNodeType::MODEL, false, &dist1);
			if (dist0 != 0 || dist1 != 0) // find pivot pos
			{
				const Vector3 pivot0 = (dist0 != 0) ?
					revoluteAxis * dist0 + pos1
					: -revoluteAxis * dist1 + pos1;

				g_geno->m_uiLink.m_pivots[0].dir = (pivot0 - pos0).Normal();
				g_geno->m_uiLink.m_pivots[0].len = pivot0.Distance(pos0);
			}
			//~auto pivot position

			const Vector3 linkPos = (g_geno->m_uiLink.GetPivotWorldTransform(0).pos +
				g_geno->m_uiLink.GetPivotWorldTransform(1).pos) / 2.f;
			g_geno->m_uiLink.SetRevoluteAxis(revoluteAxis, linkPos);
		}
	}
}