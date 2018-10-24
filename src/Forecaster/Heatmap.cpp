#include "Forecaster.h"


namespace Forecast {


Heatmap::Heatmap() {
	
}

void Heatmap::Init(double low, double high, double min_ystep, double ystep, int length, int xstep) {
	this->low = low;
	this->high = high;
	this->ystep = ystep;
	this->xstep = xstep;
	height = high - low;
	ysize = height / ystep + 1;
	xsize = length / xstep;
	div = (ystep / min_ystep) * xstep;
	data.SetCount(0);
	data.SetCount(ysize * xsize, 0);
}

int Heatmap::Pos(int x, double y) const {
	int yi = (y - low) / ystep;
	if (yi < 0 || yi >= ysize)
		return -1;
	int xi = x / xstep;
	int pos = xi * ysize + yi;
	return pos;
}

void Heatmap::Add(int x, double y, double value) {
	int pos = Pos(x, y);
	if (pos >= 0 && pos < data.GetCount())
		data[pos] += value;
}




}