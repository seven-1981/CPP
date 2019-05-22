#ifndef _FCWINDOWCOLOR_H
#define _FCWINDOWCOLOR_H

#include <GL/freeglut.h>

enum class FCWindowColor_e
{
	ColorRed,
	ColorGreen,
	ColorBlue,
	ColorWhite,
	ColorBlack,
	ColorYellow,
	ColorCyan,
	ColorMagenta,
	ColorSilver,
	ColorGray,
	ColorBrown,
	ColorYellowDark,
	ColorGreenDark,
	ColorPurple,
	ColorTeal,
	ColorNavy
};

class FCWindowColor
{
public:
	static void set(FCWindowColor_e color_to_set)
	{
		RGB col = convert(color_to_set);
		glColor3f(col.red, col.green, col.blue);
	}
		
private:
	struct RGB
	{
		RGB(float r, float g, float b) :
		  red(r), green(g), blue(b) { }
		float red = 0.0f;
		float green = 0.0f;
		float blue = 0.0f;
	};
	
	static RGB convert(FCWindowColor_e color)
	{
		float r = 0.0f, g = 0.0f, b = 0.0f;
		switch (color)
		{
			case FCWindowColor_e::ColorRed:
				r = 1.0f; g = 0.0f; b = 0.0f;
				break;
			case FCWindowColor_e::ColorGreen:
				r = 0.0f; g = 1.0f; b = 0.0f;
				break;
			case FCWindowColor_e::ColorBlue:
				r = 0.0f; g = 0.0f; b = 1.0f;
				break;
			case FCWindowColor_e::ColorWhite:
				r = 1.0f; g = 1.0f; b = 1.0f;
				break;
			case FCWindowColor_e::ColorBlack:
				r = 0.0f; g = 0.0f; b = 0.0f;
				break;
			case FCWindowColor_e::ColorYellow:
				r = 1.0f; g = 1.0f; b = 0.0f;
				break;
			case FCWindowColor_e::ColorCyan:
				r = 0.0f; g = 1.0f; b = 1.0f;
				break;
			case FCWindowColor_e::ColorMagenta:
				r = 1.0f; g = 0.0f; b = 1.0f;
				break;
			case FCWindowColor_e::ColorSilver:
				r = 0.75f; g = 0.75f; b = 0.75f;
				break;
			case FCWindowColor_e::ColorGray:
				r = 0.5f; g = 0.5f; b = 0.5f;
				break;
			case FCWindowColor_e::ColorBrown:
				r = 0.5f; g = 0.0f; b = 0.0f;
				break;
			case FCWindowColor_e::ColorYellowDark:
				r = 0.5f; g = 0.5f; b = 0.0f;
				break;
			case FCWindowColor_e::ColorGreenDark:
				r = 0.0f; g = 0.5f; b = 0.0f;
				break;
			case FCWindowColor_e::ColorPurple:
				r = 0.5f; g = 0.0f; b = 0.5f;
				break;
			case FCWindowColor_e::ColorTeal:
				r = 0.0f; g = 0.5f; b = 0.5f;
				break;
			case FCWindowColor_e::ColorNavy:
				r = 0.0f; g = 0.0f; b = 0.5f;
				break;	
				
			default:
				r = 1.0f; g = 1.0f; b = 1.0f;
				break;
		}
		
		RGB ret_col { r, g, b };
		return ret_col;
	}
	
};

#endif
