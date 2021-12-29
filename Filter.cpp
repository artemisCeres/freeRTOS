/*
 * Filter.cpp
 *
 *  Created on: 3 Dec 2021
 *      Author: Gaylord
 */

#include "Filter.h"

Filter::Filter() : waitTime(0), tickStart(0){
	// TODO Auto-generated constructor stub

}

Filter::~Filter() {
	// TODO Auto-generated destructor stub
}

void Filter::set(int ms){
    waitTime = ms;
}

void Filter::start(int systick_start){
    tickStart = systick_start;
}

bool Filter::stop(int systick_stop){
    elapsedTime = systick_stop - tickStart;
    if(elapsedTime < waitTime){
        return false;
    }
    return true;
}
