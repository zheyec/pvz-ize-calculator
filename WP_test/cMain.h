#pragma once

#include <wx/wx.h>
#include <wx/grid.h>
#include <wx/thread.h>
#include <sstream>
#include "memoryUtils.h"

class MyThread;
class cMain : public wxFrame {
public:
	cMain();

public:
	wxButton* m_btn1 = nullptr;
	wxGrid* m_grid1 = nullptr;
	wxMenu* menuSettings = nullptr;
	Memory mem;
	bool stop = false;
	wxColour bgColour = wxColour(240, 240, 240);
	wxFont cellFont = wxWindow::GetFont();
	wxFont cellFontBold;
	MyThread* m_pThread;
	wxCriticalSection m_pThreadCS;

	void OnButtonClicked(wxCommandEvent& evt);
	void OnExit(wxCommandEvent& evt);
	void OnAbout(wxCommandEvent& evt);
	void OnDoc(wxCommandEvent& evt);
	void OnSelect(wxGridRangeSelectEvent& evt);
	void OnCellLeftClick(wxGridEvent& evt);
	void SetValues();
	void OnThreadUpdate(wxThreadEvent& evt);
	void OnClose(wxCloseEvent& evt);
	void OnStartThread();
	void StopThread();
	void resetStyle();
	wxDECLARE_EVENT_TABLE();

protected:
	int test = 0;
};