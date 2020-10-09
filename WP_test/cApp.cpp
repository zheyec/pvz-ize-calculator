#include "cApp.h"
#include "resource.h"
wxIMPLEMENT_APP(cApp);
bool cApp::OnInit()
{
	m_frame1 = new cMain();
	m_frame1->SetIcon(wxICON(sample));
	m_frame1->Show();

	return true;
}
