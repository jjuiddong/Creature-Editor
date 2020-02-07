
#include "stdafx.h"
#include "resourceview.h"
#include "3dview.h"
#include "genoview.h"
#include "../creature/creature.h"

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
		ImGui::Columns(4, "texturecolumns5", false);
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
						// create creature
						const StrPath fileName = StrPath("./media/creature/") + str;

						// phenotype view load
						{
							const graphic::cCamera3D &camera = g_global->m_3dView->m_camera;
							const Vector2 size(camera.m_width, camera.m_height);
							const Ray ray = camera.GetRay((int)size.x / 2, (int)size.y / 2 + (int)size.y / 5);
							const Plane ground(Vector3(0, 1, 0), 0);
							const Vector3 targetPos = ground.Pick(ray.orig, ray.dir);
							g_pheno->ReadCreatureFile(fileName, targetPos);
						}

						// genotype view load
						{
							const graphic::cCamera3D &camera = g_global->m_genoView->m_camera;
							const Vector2 size(camera.m_width, camera.m_height);
							const Ray ray = camera.GetRay((int)size.x / 2, (int)size.y / 2 + (int)size.y / 5);
							const Plane ground(Vector3(0, 1, 0), 0);
							const Vector3 targetPos = ground.Pick(ray.orig, ray.dir);
							g_geno->ReadCreatureFile(fileName, targetPos);					
						}

					}//~IsDoubleClicked
				}//~IsItemClicked
				ImGui::NextColumn();
			}//~PassFilter
			++i;
		}
		ImGui::TreePop();
	}
}


void cResourceView::UpdateResourceFiles()
{
	m_fileList.clear();

	list<string> exts;
	exts.push_back(".pnt");
	exts.push_back(".gnt");
	const StrPath dir = StrPath("./media/creature/").GetFullFileName();
	common::CollectFiles2(exts, dir.c_str(), dir.c_str(), m_fileList);
}
