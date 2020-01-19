//
// 2020-01-19, jjuiddong
// Editor View
//
#pragma once


class cEditorView : public framework::cDockWindow
{
public:
	cEditorView(const StrId &name);
	virtual ~cEditorView();

	virtual void OnUpdate(const float deltaSeconds) override;
	virtual void OnRender(const float deltaSeconds) override;


public:
};
