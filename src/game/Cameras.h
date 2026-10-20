/*
 * Copyright 2026, Akakor.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef MG_CAMERAS_H
#define MG_CAMERAS_H


// Surveillance and laser cameras (logic/actors/camera.asm).

#include <stdint.h>


void		InitCamera(uint16_t ix);
void		InitCameraLaser(uint16_t ix);
void		CameraLogic(uint16_t ix);
void		LaserCameraLogic(uint16_t ix);


#endif	// MG_CAMERAS_H
