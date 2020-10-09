#include "cMain.h"
#include "windows.h"
#include "memoryUtils.h"
#include "puzzleSolver.h"
#include "wx/thread.h"

using namespace std;

class MyThread : public wxThread {
public:
	MyThread(cMain* handler)
		: wxThread(wxTHREAD_DETACHED) {
		m_pHandler = handler;
	}
	bool stop = false;

	~MyThread();

protected:
	virtual wxThread::ExitCode Entry();
	cMain* m_pHandler;
};

enum {
	ID_DOC = 1,
	ID_BGRUN,
	ID_AUTOCLT,
	MYTHREAD_UPDATE = wxID_HIGHEST + 1
};

wxBEGIN_EVENT_TABLE(cMain, wxFrame)
EVT_BUTTON(10001, OnButtonClicked)
EVT_MENU(wxID_EXIT, OnExit)
EVT_MENU(wxID_ABOUT, OnAbout)
EVT_MENU(ID_DOC, OnDoc)
EVT_GRID_RANGE_SELECT(OnSelect)
EVT_GRID_CELL_LEFT_CLICK(OnCellLeftClick)
EVT_CLOSE(OnClose)
EVT_THREAD(MYTHREAD_UPDATE, OnThreadUpdate)
wxEND_EVENT_TABLE()

cMain::cMain() : wxFrame(nullptr, wxID_ANY, "IZE血量计算器", wxDefaultPosition, wxSize(348, 450), wxDEFAULT_FRAME_STYLE & ~(wxRESIZE_BORDER | wxMAXIMIZE_BOX)) {
	cellFont.SetPointSize(10);
	cellFontBold = cellFont.Bold();
	menuSettings = new wxMenu;
	menuSettings->AppendCheckItem(ID_BGRUN, "&后台运行", "开启/关闭游戏后台运行");
	menuSettings->AppendCheckItem(ID_AUTOCLT, "&自动收集", "开启/关闭自动收集");
	menuSettings->Check(ID_BGRUN, true);
	wxMenu* menuHelp = new wxMenu;
	menuHelp->Append(ID_DOC, "&使用说明", " ");
	menuHelp->Append(wxID_ABOUT, "&关于...", " ");
	menuHelp->Append(wxID_EXIT, "&退出", " ");
	wxMenuBar* menuBar = new wxMenuBar;
	menuBar->Append(menuSettings, "&设置");
	menuBar->Append(menuHelp, "&帮助");
	SetMenuBar(menuBar);
	CreateStatusBar(1, wxSB_FLAT);
	wxPanel* panel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(343, 450), wxWANTS_CHARS);
	panel->SetBackgroundColour(bgColour);
	wxColour btnColour = wxColour(225, 225, 225);
	m_btn1 = new wxButton(panel, 10001, "一键算血", wxPoint(97, 16), wxSize(150, 30));
	m_btn1->SetBackgroundColour(btnColour);
	m_grid1 = new wxGrid(panel, 10002, wxPoint(3, 60), wxSize(324, 350));
	m_grid1->CreateGrid(7, 6);
	m_grid1->SetDefaultRowSize(48);
	m_grid1->SetDefaultColSize(48);
	m_grid1->SetColSize(5, 66);
	m_grid1->SetColSize(3, 66);
	m_grid1->HideColLabels();
	m_grid1->HideRowLabels();
	wxString colLabs[5] = { "撑杆", "慢速", "梯子", "橄榄", "撑杆梯子" };
	wxString rowLabs[5] = { "第一行", "第二行", "第三行", "第四行", "第五行" };
	wxFont labelFont1 = wxWindow::GetFont().MakeBold();
	labelFont1.SetPointSize(10);
	wxFont labelFont2 = wxWindow::GetFont();
	labelFont2.SetPointSize(10);
	for (int i = 1; i < 6; i++) {
		m_grid1->SetCellValue(0, i, colLabs[i - 1]);
		m_grid1->SetCellValue(i, 0, rowLabs[i - 1]);
		m_grid1->SetCellAlignment(0, i, wxALIGN_CENTRE, wxALIGN_CENTRE);
		m_grid1->SetCellAlignment(i, 0, wxALIGN_CENTRE, wxALIGN_CENTRE);
		m_grid1->SetCellFont(0, i, labelFont1);
		m_grid1->SetCellFont(i, 0, labelFont2);
	}
	for (int i = 0; i < 6; i++) {
		m_grid1->DisableRowResize(i);
		m_grid1->DisableColResize(i);
	}
	for (int i = 0; i < 7; i++)
		for (int j = 0; j < 6; j++) {
			m_grid1->SetCellBackgroundColour(i, j, bgColour);
		}
	m_grid1->EnableEditing(false);
	m_grid1->EnableGridLines(false);
	m_grid1->SetCellHighlightPenWidth(0);
	m_grid1->SetCellHighlightROPenWidth(0);
	m_grid1->ShowScrollbars(wxSHOW_SB_NEVER, wxSHOW_SB_NEVER);
	for (int i = 1; i < 6; i++)
		for (int j = 1; j < 6; j++) {
			m_grid1->SetCellAlignment(i, j, wxALIGN_CENTRE, wxALIGN_CENTRE);
			m_grid1->SetCellFont(i, j, cellFont);
		}
	mem = Memory{};
	OnStartThread();
}

void cMain::OnExit(wxCommandEvent& evt) {
	m_pThread->stop = true;
	Close(true);
}

void cMain::OnAbout(wxCommandEvent& evt) {
	wxMessageBox("这是一个用于计算IZE单破血量的计算器。\n\n版本号: v1.0.2\n\n开发者: Crescendo\nbilibili: Crescebdo\n贴吧: OTZzzz\n\n使用工具: Visual Studio 2019, wxWidgets 3.4.1",
		"关于IZE计算器", wxOK);
}

void cMain::OnDoc(wxCommandEvent& evt) {
	string r = R"(本计算器适用于IZE单破血量的计算，支持英文原版与汉化第二版。

模拟算血时，不会考虑其他路的杨桃、三线或磁铁（倾斜主题除外）。同时，默认土豆已引爆、窝瓜已引走、大嘴已填饱。

显示结果时，带有括号的代表【不推荐】，加粗并高亮的代表【推荐】。

撑杆算血：计算撑杆跳过第一个非地刺、非窝瓜植物后所受伤害。
【推荐】的情况：算血不超过14。
【不推荐】的情况：被跳过的植物是裂荚、杨桃或向日葵，或算血超过39。

慢速算血：计算路障或铁桶所受伤害。
【推荐】的情况：算血不超过25，或算血不超过61且当前行无磁铁。
【不推荐】的情况：算血超过72。

梯子算血：计算梯子所受伤害（饰品+本体）。饰品掉落后，所有伤害转移至本体上。
【推荐】的情况：本体算血不超过14，且当前行无磁铁。
【不推荐】的情况：当前行有磁铁，或本体算血超过19。

橄榄算血：计算橄榄所受伤害。
【推荐】的情况：算血不超过77且当前行无磁铁。
【不推荐】的情况：当前行有磁铁，或算血超过82。

撑杆梯子算血：计算先放撑杆，落地后放梯子时梯子所受的伤害。
推荐/不推荐标准同梯子。

注：
算血公式见cv6263782。此程序的计算结果完全基于分段算血公式，不等同于实际游戏情况；其计算结果均可用计算器手动验算。)";
	wxMessageBox(r, "使用说明", wxOK);
}

// 禁止选择
void cMain::OnSelect(wxGridRangeSelectEvent& evt) {
	wxGridCellCoordsArray selTop = m_grid1->GetSelectionBlockTopLeft();
	wxGridCellCoordsArray selBottom = m_grid1->GetSelectionBlockBottomRight();
	for (int i = 0; i < selTop.Count(); i++) {
		m_grid1->ClearSelection();
	}
	evt.Skip();
}

// 禁止选择
void cMain::OnCellLeftClick(wxGridEvent& evt) {
	m_grid1->SelectRow(evt.GetRow());
	evt.Skip();
}

// 一键算血
void cMain::OnButtonClicked(wxCommandEvent& evt) {
	SetValues();
	evt.Skip();
}

void cMain::resetStyle() {
	for (int i = 1; i < 6; i++)
		for (int j = 1; j < 6; j++) {
			m_grid1->SetCellValue(i, j, "");
			m_grid1->SetCellBackgroundColour(i, j, bgColour);
			m_grid1->SetCellFont(i, j, cellFont);
			m_grid1->SetCellTextColour(i, j, *wxBLACK);
		}
}

// 更新血量
void cMain::SetValues() {
	int** plants = mem.readPlants(menuSettings->IsChecked(ID_BGRUN), menuSettings->IsChecked(ID_AUTOCLT));
	if (plants == nullptr) {
		SetStatusText(wxT("尚未进入IZE"), 0);
		resetStyle();
	}
	else {
		SetStatusText(wxT(""), 0);
		Puzzle puzzle = Puzzle(plants, mem.isQX);
		string** result = puzzle.result;
		int** highlight = puzzle.highlight;
		for (int i = 1; i < 6; i++)
			for (int j = 1; j < 6; j++) {
				string str = result[i - 1][j - 1];
				int hl = highlight[i - 1][j - 1];
				if (hl == -1) {
					m_grid1->SetCellFont(i, j, cellFont);
					m_grid1->SetCellTextColour(i, j, wxColour(150, 150, 150));
					m_grid1->SetCellBackgroundColour(i, j, bgColour);
					str = "(" + str + ")";
				}
				else if (hl == 0) {
					m_grid1->SetCellFont(i, j, cellFont);
					m_grid1->SetCellTextColour(i, j, *wxBLACK);
					m_grid1->SetCellBackgroundColour(i, j, bgColour);
				}
				else if (hl == 1) {
					m_grid1->SetCellFont(i, j, cellFontBold);
					m_grid1->SetCellTextColour(i, j, *wxBLACK);
					m_grid1->SetCellBackgroundColour(i, j, wxColour(210, 210, 210));
				}
				m_grid1->SetCellValue(i, j, wxString(str));
			}
		for (int i = 0; i < 5; i++) {
			delete[] plants[i];
		}
		delete[] plants;
	}
	m_grid1->ForceRefresh();
}

void cMain::OnClose(wxCloseEvent&) {
	m_pThread->stop = true;
	StopThread();

	// now wait till thread is actually destroyed
	while (1) {
		{ // was the ~MyThread() function executed?
			wxCriticalSectionLocker enter(m_pThreadCS);
			if (!m_pThread) break;
		}

		// wait for thread completion
		wxThread::This()->Sleep(1);
	}

	Destroy();
}

void cMain::OnStartThread() {
	if (m_pThread != NULL) return;

	m_pThread = new MyThread(this);
	if (m_pThread->Run() != wxTHREAD_NO_ERROR) {
		wxLogError("Can't create the thread!");
		delete m_pThread;
		m_pThread = NULL;
	}
}

void cMain::OnThreadUpdate(wxThreadEvent& evt) {
	SetValues();
}

void cMain::StopThread() {
	wxCriticalSectionLocker enter(m_pThreadCS);
	if (m_pThread) // does the thread still exist?
	{
		if (m_pThread->Delete() != wxTHREAD_NO_ERROR)
			wxLogError("Can't delete the thread!");
	}
}

wxThread::ExitCode MyThread::Entry() {
	while (true) {
		if (stop) break;
		wxThread::This()->Sleep(500);		// sleep 500 ms
		if (stop) break;
		wxQueueEvent(m_pHandler, new wxThreadEvent(wxEVT_THREAD, MYTHREAD_UPDATE));
	}
	return (wxThread::ExitCode)0; // success
}

MyThread::~MyThread() {
	wxCriticalSectionLocker enter(m_pHandler->m_pThreadCS);
	m_pHandler->m_pThread = NULL;
}