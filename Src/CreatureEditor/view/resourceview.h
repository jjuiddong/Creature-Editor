//
// 2020-01-24, jjuiddong
// Resource View
//
#pragma once


class cResourceView : public framework::cDockWindow
{
public:
	cResourceView(const StrId &name);
	virtual ~cResourceView();

	virtual void OnUpdate(const float deltaSeconds) override;
	virtual void OnRender(const float deltaSeconds) override;
	void UpdateResourceFiles();
	StrPath GetSelectFileName();


protected:
	void RenderPopupMenu();
	void LoadPhenotypeView(const StrPath &fileName);
	void LoadGenotypeView(const StrPath &fileName);


public:
	StrPath m_dirPath; // ./media/creatures/
	vector<string> m_fileList;
	int m_selectFileIdx;
};
