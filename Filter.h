/*
 * Filter.h
 *
 *  Created on: 3 Dec 2021
 *      Author: Gaylord
 */

#ifndef FILTER_H_
#define FILTER_H_

class Filter {
public:
	Filter();
	virtual ~Filter();
	void set(int ms);
	void start(int systick_start);
	bool stop(int systick_stop);
	int elapsedTime;
private:
private:
	int waitTime = 50;
	int tickStart;
	int tickEnd;
};

#endif /* FILTER_H_ */
