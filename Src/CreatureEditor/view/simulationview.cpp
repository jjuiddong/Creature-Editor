
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
	if (ImGui::Button("Play >"))
	{

	}

	if (ImGui::Button("Recovery Saved Pose <<--"))
	{

	}

	if (ImGui::Button("Save Current Selection Pose"))
	{

	}
	
}
