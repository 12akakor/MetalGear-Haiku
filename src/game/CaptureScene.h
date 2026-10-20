/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef MG_CAPTURE_SCENE_H
#define MG_CAPTURE_SCENE_H


// Snake is captured and put in a cell (logic/capturescene.asm).

#include <stdint.h>


void		CaptureSceneLogic();
void		InitCaptureScene(uint16_t ix);
void		CaptureGuardsLogic(uint16_t ix);


#endif	// MG_CAPTURE_SCENE_H
