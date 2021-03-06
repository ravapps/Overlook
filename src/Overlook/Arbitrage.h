#ifndef _Overlook_Arbitrage_h_
#define _Overlook_Arbitrage_h_

// Original source: https://www.mql5.com/en/code/9356


namespace Overlook {

class Arbitrage {
	
protected:
	friend class ArbitrageCtrl;
	
	
	static const int MAX_ALLSYMBOLS = 380;
	static const int MAX_VARIANTSYMBOLS = 74;
	static const int MAX_REALSYMBOLS = 380;
	static const int MAX_VARIANTPAIRS = 5402;
	
	struct VariantPair : Moveable<VariantPair> {
		int arbitrage_count = 0;
		bool xtrade = 0;
		double xposition = 0;
		
		VariantPair() {}
		VariantPair(const VariantPair& s) {*this = s;}
		void operator=(const VariantPair& s) {
			arbitrage_count = s.arbitrage_count;
			xtrade = s.xtrade;
			xposition = s.xposition;
		}
	};
	
	struct VariantSym : Moveable<VariantSym> {
		double bid = 0, ask = 0;
		int math = 0, all_sym = 0;
		
		VariantSym() {}
		VariantSym(const VariantSym& s) {*this = s;}
		void operator=(const VariantSym& s) {
			bid = s.bid;
			ask = s.ask;
			math = s.math;
			all_sym = s.all_sym;
		}
	};
	
	struct Symbol : Moveable<Symbol> {
		Vector<VariantPair> pair;
		Vector<VariantSym> sym;
		
		double point = 0.0;
		double min_pips = 0.0;
		
		Symbol() {}
		Symbol(const Symbol& s) {*this = s;}
		void operator=(const Symbol& s) {
			pair <<= s.pair;
			sym <<= s.sym;
			point = s.point;
			min_pips = s.min_pips;
		}
	};
	
	Vector<Symbol> symbols;
	Vector<String> currencies, real_symbols;
	String str_out;
	double min_pips = -3;
	double bids_real[MAX_REALSYMBOLS];
	double asks_real[MAX_REALSYMBOLS];
	double position[MAX_REALSYMBOLS];
	double prev_position[MAX_REALSYMBOLS];
	double lots = 1;
	double ALPHA = 0.001;
	int max_arbitrage_count = 0;
	int real_sym_count = 0;
	bool running = false, stopped = true;
	
	
public:
	typedef Arbitrage CLASSNAME;
	Arbitrage();
	~Arbitrage();
	
	void Init();
	void Process();
	void GetRealSymbols();
	bool IsRealSymbol(const String& s);
	int  GetNumIsRealSymbol(const String& s) {return GetSystem().FindSymbol(s);}
	void GetAllSymbols();
	void GetPipsD();
	void GetXTrade(String FileName);
	void PrintBeginInfo();
	void TradeArbitrage();
	void OpenArbitragePosition(int sym_id, int variant1_id, int variant2_id, double Vol);
	String SymbolToStr(int i, int j);
	String SymbolToFile(int i, int j);
	String ArbitragePositions();
	void RefreshPositions();
	void MonitoringArbitrage(int sym_id, int variant1_id, int variant2_id);
	void OpenSymbolPosition(int sym_id, int Variant, int Type, double Vol);
	void SymbolDone(double Vol, int Symb);
	void GetBidAsk(int i, int j);
	void WriteStatistic(String FileName);
	void GetRealBidAsk();
	
	
	
};

inline Arbitrage& GetArbitrage() {return Single<Arbitrage>();}


class ArbitrageCtrl : public ParentCtrl {
	Splitter split;
	ArrayCtrl arblist, siglist;
	
public:
	typedef ArbitrageCtrl CLASSNAME;
	ArbitrageCtrl();
	
	void Data();
	
};

}

#endif
