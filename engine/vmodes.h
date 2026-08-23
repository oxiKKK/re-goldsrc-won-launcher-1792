// vmodes.h - header for videomodes
#ifndef VMODES_H
#define VMODES_H
#pragma once


typedef enum
{
	VT_None = 0,
	VT_Software,
	VT_OpenGL,
	VT_Direct3D,
} VidTypes;

// a pixel can be one, two, or four bytes
typedef byte pixel_t;

typedef struct viddef_s
{
	pixel_t*		buffer;			// invisible buffer
	pixel_t*		colormap;		// 256 * VID_GRADES size
	unsigned short* colormap16;		// 256 * VID_GRADES size
	int				fullbright;		// index of first fullbright color
	int				bits;
	int				is15bit;
	unsigned		rowbytes;		// may be > width if displayed in a window
	unsigned		width;
	unsigned		height;
	float			aspect;			// width / height -- < 0 is taller than wide
	int				numpages;
	int				recalc_refdef;	// if true, recalc vid-based stuff
	pixel_t*		conbuffer;
	int				conrowbytes;
	unsigned		conwidth;
	unsigned		conheight;
	unsigned 		maxwarpwidth;
	unsigned 		maxwarpheight;
	pixel_t*		direct;			// direct drawing to framebuffer, if not
										//  NULL
	VidTypes		vidtype;
} viddef_t;


#endif //VMODES_H