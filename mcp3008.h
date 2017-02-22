/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   mcp3008.h
 * Author: Mitch
 *
 * Created on 10 February 2017, 23:57
 */

#ifndef MCP3008_H
#define MCP3008_H

// This struct is to hold up to eight possible readings from the ADC
// Remember to alter the main reading part of script if adding more inputs

struct returnVars {
	int L0[8];
};

class mcp3008
{
public:
	returnVars measureSensor();
private:
	int measureInput(int);
};

#endif /* MCP3008_H */

