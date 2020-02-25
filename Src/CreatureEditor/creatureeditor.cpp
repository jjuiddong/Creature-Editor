//
// Creature Editor
//
#include "stdafx.h"
#include "view/3dview.h"
#include "view/phenoeditorview.h"
#include "view/resourceview.h"
#include "view/simulationview.h"
#include "view/genoview.h"
#include "view/genoeditorview.h"
#include "view/nnview.h"
#include "view/evolutionview.h"


cGlobal *g_global = nullptr;
cPhenoTypeManager *g_pheno = nullptr;
cGenoTypeManager *g_geno = nullptr;
cNNManager *g_nn = nullptr;

using namespace graphic;
using namespace framework;

class cViewer : public framework::cGameMain2
{
public:
	cViewer();
	virtual ~cViewer();
	virtual bool OnInit() override;
	virtual void OnUpdate(const float deltaSeconds) override;
	virtual void OnRender(const float deltaSeconds) override;
	virtual void OnEventProc(const sf::Event &evt) override;
};

INIT_FRAMEWORK3(cViewer);


cViewer::cViewer()
{
	m_windowName = L"Creature Editor";
	m_isLazyMode = true;
	//const RECT r = { 0, 0, 1024, 768 };
	const RECT r = { 0, 0, 1280, 960 };
	m_windowRect = r;
	graphic::cResourceManager::Get()->SetMediaDirectory("./media/");
}

cViewer::~cViewer()
{
	SAFE_DELETE(g_geno);
	SAFE_DELETE(g_pheno);
	SAFE_DELETE(g_nn);
	SAFE_DELETE(g_global);
}


bool cViewer::OnInit()
{
	dbg::RemoveLog();
	dbg::RemoveErrLog();

	const float WINSIZE_X = float(m_windowRect.right - m_windowRect.left);
	const float WINSIZE_Y = float(m_windowRect.bottom - m_windowRect.top);
	GetMainCamera().SetCamera(Vector3(10.0f, 10, -30.f), Vector3(-5, 0, 10), Vector3(0, 1, 0));
	GetMainCamera().SetProjection(MATH_PI / 4.f, (float)WINSIZE_X / (float)WINSIZE_Y, 1.f, 10000.f);
	GetMainCamera().SetViewPort(WINSIZE_X, WINSIZE_Y);

	GetMainLight().Init(cLight::LIGHT_DIRECTIONAL,
		Vector4(0.2f, 0.2f, 0.2f, 1), Vector4(0.9f, 0.9f, 0.9f, 1),
		Vector4(0.2f, 0.2f, 0.2f, 1));
	const Vector3 lightPos(-300, 300, -300);
	const Vector3 lightLookat(0, 0, 0);
	GetMainLight().SetPosition(lightPos);
	GetMainLight().SetDirection((lightLookat - lightPos).Normal());

	g_global = new cGlobal();
	if (!g_global->Init(m_renderer))
		return false;

	g_pheno = new cPhenoTypeManager();
	if (!g_pheno->Init(m_renderer, &g_global->m_physics, g_global->m_physSync))
		return false;

	g_geno = new cGenoTypeManager();
	if (!g_geno->Init(m_renderer, &g_global->m_physics, g_global->m_physSync))
		return false;

	g_nn = new cNNManager();
	if (!g_nn->Init(m_renderer))
		return false;

	c3DView *p3dView = new c3DView("3D View");
	p3dView->Create(eDockState::DOCKWINDOW, eDockSlot::TAB, this, NULL);
	bool result = p3dView->Init(m_renderer);
	assert(result);

	cGenoView *genoView = new cGenoView("GenoType View");
	genoView->Create(eDockState::DOCKWINDOW, eDockSlot::TAB, this, p3dView);
	result = genoView->Init(m_renderer);
	assert(result);

	cNNView *nnView = new cNNView("Neural Net");
	nnView->Create(eDockState::DOCKWINDOW, eDockSlot::TAB, this, p3dView);
	result = nnView->Init(m_renderer);
	assert(result);

	cPhenoEditorView *peditView = new cPhenoEditorView("Pheno Type");
	peditView->Create(eDockState::DOCKWINDOW, eDockSlot::RIGHT, this, p3dView, 0.25f
		, framework::eDockSizingOption::PIXEL);

	cGenoEditorView *geditView = new cGenoEditorView("Geno Type");
	geditView->Create(eDockState::DOCKWINDOW, eDockSlot::TAB, this, peditView, 0.5f);

	cResourceView *resourceView = new cResourceView("Resource");
	resourceView->Create(eDockState::DOCKWINDOW, eDockSlot::BOTTOM, this, p3dView, 0.2f
		, framework::eDockSizingOption::PIXEL);

	cSimulationView *simView = new cSimulationView("Simulation");
	simView->Create(eDockState::DOCKWINDOW, eDockSlot::RIGHT, this, resourceView, 0.5f);

	cEvolutionView *evoView = new cEvolutionView("Evolution");
	evoView->Create(eDockState::DOCKWINDOW, eDockSlot::TAB, this, simView);

	g_global->m_3dView = p3dView;
	g_global->m_peditorView = peditView;
	g_global->m_geditorView = geditView;
	g_global->m_resourceView = resourceView;
	g_global->m_simView = simView;
	g_global->m_genoView = genoView;
	g_global->m_nnView = nnView;
	g_global->m_pheno = g_pheno;
	g_global->m_geno = g_geno;
	g_global->m_nn = g_nn;

	m_gui.SetContext();
	m_gui.SetStyleColorsDark();

	return true;
}


void cViewer::OnUpdate(const float deltaSeconds)
{
	__super::OnUpdate(deltaSeconds);
	GetMainCamera().Update(deltaSeconds);
}


void cViewer::OnRender(const float deltaSeconds)
{
}


void cViewer::OnEventProc(const sf::Event &evt)
{
	ImGuiIO& io = ImGui::GetIO();
	switch (evt.type)
	{
	case sf::Event::KeyPressed:
		//switch (evt.key.cmd) {
		//case sf::Keyboard::Escape: close(); break;
		//}
		break;
	}
}
