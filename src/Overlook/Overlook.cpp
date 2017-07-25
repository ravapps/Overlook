#include "Overlook.h"

namespace Overlook {

Overlook::Overlook() :
	mgrctrl(sys),
	rtctrl(sys)
{
	Title("Overlook");
	Icon(OverlookImg::icon());
	MinimizeBox().MaximizeBox().Sizeable();
	
	Add(tabs.SizePos());
	tabs.Add(visins);
	tabs.Add(visins, "Traditional");
	tabs.Add(exposurectrl);
	tabs.Add(exposurectrl, "Exposure Tester");
	tabs.Add(mgrctrl);
	tabs.Add(mgrctrl, "Agent Manager");
	tabs.Add(rtctrl);
	tabs.Add(rtctrl, "Real-Time Account");
	tabs.WhenSet << THISBACK1(Data, false);
	
	
	visins.Add(droplist_split.TopPos(2, 26).HSizePos(2, 2));
	droplist_split << ctrllist << symlist << tflist << config;
	droplist_split.Horz();
	prev_core = NULL;
	prev_view = NULL;
	ctrllist <<= THISBACK(SetView);
	symlist <<= THISBACK(SetView);
	tflist <<= THISBACK(SetView);
	config.SetLabel("Configure");
	config <<= THISBACK(Configure);
	
	PostCallback(THISBACK(Refresher));
}

Overlook::~Overlook() {
	if (prev_view) {
		visins.RemoveChild(prev_view);
		delete prev_view;
		prev_view = NULL;
	}
}

void Overlook::Refresher() {
	Data(true);
	int tab = tabs.Get();
	if (tab == 3) {
		PostRefresher();
	}
	else {
		tc.Set(1000, THISBACK(PostRefresher));
	}
}

void Overlook::Data(bool periodic) {
	int tab = tabs.Get();
	if (tab == 0) {
		if (prev_view)
			prev_view->Data();
	}
	else if (tab == 1) {
		
	}
	else if (tab == 2) {
		mgrctrl.Data();
	}
	else if (tab == 3) {
		rtctrl.Data();
	}
}

void Overlook::Init() {
	sys.Init();
	rtctrl.Init();
	exposurectrl.Init();
	
	// Init gui
	for(int i = 0; i < sys.GetPeriodCount(); i++)
		tflist.Add(sys.GetPeriodString(i));
	for(int i = 0; i < sys.GetTotalSymbolCount(); i++)
		symlist.Add(sys.GetSymbol(i));
	for(int i = 0; i < System::GetCtrlFactories().GetCount(); i++)
		ctrllist.Add(System::GetCtrlFactories()[i].a);
	
	tflist.SetIndex(tflist.GetCount()-2); // TODO: clear these development values
	symlist.SetIndex(0);
	ctrllist.SetIndex(0);
	
	PostCallback(THISBACK(SetView));
}

void Overlook::Start() {
	sys.Start();
}

void Overlook::Deinit() {
	sys.Stop();
}

void Overlook::SetView() {
	int c = ctrllist.GetIndex();
	int s = symlist.GetIndex();
	int t = tflist.GetIndex();
	
	if (prev_view) {
		visins.RemoveChild(prev_view);
		delete prev_view;
		prev_view = NULL;
		prev_core = NULL;
	}
	
	Core* core;
	CustomCtrl* view;
	view = System::GetCtrlFactories()[c].c();
	
	core = sys.CreateSingle(c, s, t);
	ASSERT(core);
	
	view->core = core;
	view->SetSymbol(s);
	view->SetTf(t);
	
	view->Init(core);
	
	view->Data();
	
	visins.Add(view->VSizePos(30).HSizePos());
	prev_view = view;
	prev_core = core;
}

void Overlook::Configure() {
	if (!prev_core) return;
	
	static One<TopWindow> tw;
	static bool save;
	save = false;
	tw.Create();
	
	tw->Title("Configure arguments");
	Button ok;
	ParentCtrl ctrl;
	Array<Label> labels;
	Array<EditDoubleSpin> edits;
	ok.SetLabel("OK");
	tw->Add(ctrl.HSizePos().VSizePos(0,30));
	tw->Add(ok.BottomPos(3,24).RightPos(3, 100));
	ok.WhenAction << [=] () {
		save = true;
		tw->Close();
	};
	int c = ctrllist.GetIndex();
	int s = symlist.GetIndex();
	int t = tflist.GetIndex();
	ArgChanger reg;
	reg.SetLoading();
	prev_core->IO(reg);
	
	Size sz(320, reg.args.GetCount()*30 + 50);
	tw->SetRect(sz);
	int xoff = (int)(sz.cx * 0.5);
	
	for(int i = 0; i < reg.args.GetCount(); i++) {
		Label& lbl = labels.Add();
		lbl.SetLabel(reg.keys[i]);
		lbl.SetAlign(ALIGN_RIGHT);
		EditDoubleSpin& edit = edits.Add();
		lbl.SetRect(4, i*30, xoff-8, 30);
		edit.SetRect(xoff, i*30, sz.cx-xoff, 30);
		edit.SetData(reg.args[i]);
		edit.WhenEnter = ok.WhenAction;
		ctrl.Add(lbl);
		ctrl.Add(edit);
	}
	
	tw->Run();
	tw.Clear();
	
	if (save) {
		// These settings does not affect the combination, so these can be applied to the current combination.
		for(int i = 0; i < reg.args.GetCount(); i++) {
			reg.args[i] = edits[i].GetData();
		}
		reg.SetStoring();
		prev_core->IO(reg);
		prev_core->ClearContent();
		prev_core->InitAll();
		prev_core->Refresh();
		Refresh();
	}
}

}
