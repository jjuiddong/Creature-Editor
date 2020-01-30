
#include "stdafx.h"
#include "resourceview.h"
#include "3dview.h"

using namespace graphic;

cResourceView::cResourceView(const StrId &name)
	: framework::cDockWindow(name)
{
	//UpdateResourceFiles();
}

cResourceView::~cResourceView()
{
}


void cResourceView::OnUpdate(const float deltaSeconds)
{
}


void cResourceView::OnRender(const float deltaSeconds)
{
	static ImGuiTextFilter filter;
	ImGui::Text("Search");
	ImGui::SameLine();
	filter.Draw("##Search", 200);

	ImGui::SameLine();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.6f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.1f, 0.9f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.4f, 0, 1));
	if (ImGui::Button("Refresh (F5)"))
		UpdateResourceFiles();
	ImGui::PopStyleColor(3);

	static int selectIdx = -1;
	int i = 0;

	ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Always);
	if (ImGui::TreeNode((void*)0, "Creature Files"))
	{
		ImGui::Columns(5, "texturecolumns5", false);
		for (auto &str : m_fileList)
		{
			const ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow 
				| ImGuiTreeNodeFlags_OpenOnDoubleClick
				| ImGuiTreeNodeFlags_Leaf 
				| ImGuiTreeNodeFlags_NoTreePushOnOpen
				| ((i == selectIdx) ? ImGuiTreeNodeFlags_Selected : 0);

			if (filter.PassFilter(str.c_str()))
			{
				ImGui::TreeNodeEx((void*)(intptr_t)i, node_flags, str.c_str());

				if (ImGui::IsItemClicked())
				{
					selectIdx = i;
					if (ImGui::IsMouseDoubleClicked(0))
					{
						const StrPath fileName = StrPath("./media/creature/") + str;
						vector<int> syncIds;
						evc::ReadPhenoTypeFile(g_global->GetRenderer()
							, fileName.c_str()
							, &syncIds);

						// moving actor position
						if (phys::sSyncInfo *sync = g_global->FindSyncInfo(syncIds[0]))
						{
							const Ray ray = g_global->m_3dView->m_camera.GetRay();
							Vector3 spawnPos = ray.orig + ray.dir * 10.f 
								- sync->node->m_transform.pos;
							spawnPos.y = 0;

							g_global->UpdateAllConnectionActorTransform(sync->actor
								, Transform(spawnPos));

							// selection
							g_global->m_state = eEditState::Normal;
							g_global->m_gizmo.SetControlNode(nullptr);
							g_global->m_gizmo.LockEditType(graphic::eGizmoEditType::SCALE, false);
							g_global->m_selJoint = nullptr;
							g_global->ClearSelection();

							g_global->SetAllConnectionActorSelect(sync->actor);
						}

						// unlock actor
						if (!g_global->m_isSpawnLock && !syncIds.empty())
						{
							phys::sSyncInfo *sync = g_global->FindSyncInfo(syncIds[0]);
							if (sync)
								g_global->SetAllConnectionActorKinematic(sync->actor, false);
						}
					}
				}
				ImGui::NextColumn();
			}
			++i;
		}
		ImGui::TreePop();
	}
}


void cResourceView::UpdateResourceFiles()
{
	m_fileList.clear();

	list<string> exts;
	exts.push_back(".pnt"); exts.push_back(".PNT");
	const StrPath dir = StrPath("./media/creature/").GetFullFileName();
	common::CollectFiles2(exts, dir.c_str(), dir.c_str(), m_fileList);
}