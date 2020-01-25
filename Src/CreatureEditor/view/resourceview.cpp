
#include "stdafx.h"
#include "resourceview.h"

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
	if (ImGui::Button("Refresh"))
		UpdateResourceFiles();

	static ImGuiTextFilter filter;
	ImGui::SameLine();
	filter.Draw("Search");

	static int selectIdx = -1;
	int i = 0;

	ImGui::SetNextTreeNodeOpen(true, ImGuiCond_FirstUseEver);
	if (ImGui::TreeNode((void*)0, "Creature Files"))
	{
		ImGui::Columns(5, "texturecolumns5", false);
		for (auto &str : m_fileList)
		{
			const ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow 
				| ImGuiTreeNodeFlags_OpenOnDoubleClick
				| ((i == selectIdx) ? ImGuiTreeNodeFlags_Selected : 0);

			if (filter.PassFilter(str.c_str()))
			{
				ImGui::TreeNodeEx((void*)(intptr_t)i, node_flags | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen
					, str.c_str());

				if (ImGui::IsItemClicked())
				{
					selectIdx = i;
					if (ImGui::IsMouseDoubleClicked(0))
					{
						const StrPath fileName = StrPath("./media/creature/") + str;
						evc::ReadPhenoTypeFile(g_global->GetRenderer()
							, fileName.c_str());
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
