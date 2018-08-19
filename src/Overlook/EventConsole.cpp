#include "Overlook.h"

namespace Overlook {

EventConsole::EventConsole() {
	
}

EventConsole::~EventConsole() {
	StoreThis();
}

void EventConsole::Init() {
	LoadThis();
}

void EventConsole::Start() {
	System& sys = GetSystem();
	
	EventStatistics& es = GetEventStatistics();
	EventOptimization& eo = GetEventOptimization();
	
	if (!eo.opt.IsEnd())
		return;
	
	int slot_id = es.GetLatestSlotId();
	if (snaps.IsEmpty() || snaps.Top().slot_id != slot_id) {
		ConstBuffer& time_buf = es.db[0]->GetBuffer(4);
		if (time_buf.IsEmpty()) return;
		
		EventSnap& snap = snaps.Add();
		snap.slot_id = slot_id;
		
		snap.time = Time(1970,1,1) + time_buf.Top();
		int last_pos = time_buf.GetCount() - 1;
		
		// Calendar events
		Time calbegin = snap.time - 2*60*60;
		Time calend = snap.time + 2*60*60;
		const CalendarCommon& cal = GetCalendar();
		cal.GetEvents(snap.cal, calbegin, calend, 3);
		
		
		// Event statistics
		for(int i = 0; i < sys.GetNetCount(); i++) {
			for(int j = 0; j < EventStatistics::SRC_COUNT; j++) {
				const StatSlot& ss = es.GetLatestSlot(i, j);
				double mean = ss.av.GetMean();
				bool inverse = mean < 0.0;
				if (inverse) mean = -mean;
				int count = ss.av.GetEventCount();
				double cdf = ss.av.GetCDF(0.0, !inverse);
				int grade = (1.0 - cdf) / 0.05;
				double abs_cdf = ss.abs_av.GetCDF(0.0003, true);
				int abs_grade = (1.0 - abs_cdf) / 0.05;
				//LOG(i << " " << j << " " << cdf << " " << mean);
				if (grade < EventOptimization::grade_count && abs_grade < EventOptimization::grade_count) {
					EventSnap::Stat& s = snap.stats.Add();
					s.net = i;
					s.src = j;
					s.mean = mean;
					s.cdf = cdf;
					s.inverse = inverse;
					String desc = es.GetDescription(j);
					LOG(desc);
					s.signal = es.GetSignal(i, last_pos, j);
					s.count = count;
					s.grade = grade;
					if (inverse) s.signal *= -1;
				}
			}
		}
		Sort(snap.stats, EventSnap::Stat());
		
		
		// Automation
		Sentiment& sent = GetSentiment();
		SentimentSnapshot& ss = sent.AddSentiment();
		ss.added = GetUtcTime();
		ss.comment = "Clear";
		ss.cur_pres.SetCount(sys.GetCurrencyCount(), 0);
		ss.pair_pres.SetCount(sys.GetSymbolCount(), 0);
		ss.fmlevel = 1.0;
		for(int i = 0; i < snap.stats.GetCount(); i++) {
			EventSnap::Stat& s = snap.stats[i];
			if (s.signal) {
				snap.comment = Format("Active %d %d %d", s.net, s.src, (int)s.inverse);
				ss.comment = snap.comment;
				ss.fmlevel = eo.opt.GetBestSolution()[s.grade];
				
				System::NetSetting& net = sys.GetNet(s.net);
				
				for(int j = 0; j < sent.GetSymbolCount(); j++) {
					String sym = sent.GetSymbol(j);
					int k = net.symbols.Find(sym);
					if (k == -1) continue;
					ss.pair_pres[j] = s.signal * net.symbols[k];
				}
				
				break;
			}
		}
		sent.StoreThis();
		
		StoreThis();
	}
}








EventConsoleCtrl::EventConsoleCtrl() {
	Add(hsplit.SizePos());
	hsplit.Horz();
	
	hsplit << history << calendar << events /*<< actions*/;
	
	history.AddColumn("Time");
	history.AddColumn("Comment");
	history.ColumnWidths("2 3");
	history <<= THISBACK(LoadHistory);
	calendar.AddColumn("What");
	calendar.AddColumn("When");
	events.AddColumn("Net");
	events.AddColumn("Description");
	events.AddColumn("Average");
	events.AddColumn("Cdf");
	events.AddColumn("Count");
	events.AddColumn("Signal");
	
	
	actions.Add(act.BottomPos(0,30).HSizePos());
	actions.Add(skip.BottomPos(30,30).HSizePos());
	actions.Add(automation.BottomPos(60,30).HSizePos());
	actions.Add(against.BottomPos(90,30).HSizePos());
	actions.Add(actlbl.VSizePos(0,120).HSizePos());
	
	against.SetLabel("Against");
	automation.SetLabel("Automation");
	skip.SetLabel("Skip");
	act.SetLabel("Act");
}

void EventConsoleCtrl::Data() {
	EventConsole& ec = GetEventConsole();
	
	bool load_history = ec.GetSnapCount() && history.GetCount() == 0;
	
	for(int i = 0; i < ec.GetSnapCount(); i++) {
		EventSnap& s = ec.GetSnap(i);
		
		history.Set(i, 0, s.time);
		history.Set(i, 1, s.comment);
		
	}
	
	if (load_history) {
		history.SetCursor(history.GetCount()-1);
		LoadHistory();
	}
	
	
}

void EventConsoleCtrl::LoadHistory() {
	EventConsole& ec = GetEventConsole();
	System& sys = GetSystem();
	EventStatistics& es = GetEventStatistics();
	
	int cursor = history.GetCursor();
	
	const EventSnap& snap = ec.snaps[cursor];
	
	for(int i = 0; i < snap.cal.GetCount(); i++) {
		const CalEvent& e = snap.cal[i];
		
		calendar.Set(i, 0, e.title);
		calendar.Set(i, 1, e.timestamp);
	}
	calendar.SetCount(snap.cal.GetCount());
	
	for(int i = 0; i < snap.stats.GetCount(); i++) {
		const EventSnap::Stat& s = snap.stats[i];
		
		events.Set(i, 0, sys.GetSymbol(sys.GetNetSymbol(s.net)));
		events.Set(i, 1, es.GetDescription(s.src) + (s.inverse ? " inverse" : ""));
		events.Set(i, 2, s.mean);
		events.Set(i, 3, s.cdf);
		events.Set(i, 4, s.count);
		events.Set(i, 5, s.signal == 0 ? "" : (s.signal > 0 ? "Long" : "Short"));
	}
	events.SetCount(snap.stats.GetCount());
	
}

}
