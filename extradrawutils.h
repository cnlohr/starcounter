#ifndef _EXTRADRAWUTILS_H
#define _EXTRADRAWUTILS_H

int LoadFont( void );
void CNFGTackPixelBlend( int x, int y, int ta );
int DrawGlyph( int c, float x, float y, float size );
int GetGlyphWidth( int c, float ffx, float ffy, float size );
void DrawStr( const char * s, float x, float y, float size );
int StrWidth( const char * s, float x, float y, float size );

#endif

#ifdef EXTRADRAWUTILS_IMPLEMENTATION
stbtt_fontinfo font;

int LoadFont( void )
{
	uint8_t * ttf_buffer;
	FILE * f = fopen( "AudioLinkConsole-Bold.ttf", "rb" );
	if( !f )
	{
		fprintf( stderr, "Error: Can't open ttf\n" );
		return -4;
	}
	fseek( f, 0, SEEK_END );
	int len = ftell( f );
	fseek( f, 0, SEEK_SET );
	ttf_buffer = malloc( len );
	if( fread( ttf_buffer, len, 1, f ) != 1 || len == 0 )
	{
		fprintf( stderr, "Error: Can't read ttf\n" );
		return -4;
	}
	fclose( f );
	if( stbtt_InitFont( &font, ttf_buffer, stbtt_GetFontOffsetForIndex( ttf_buffer, 0 ) ) == 0 )
	{
		fprintf( stderr, "Error: Can't interpret font\n" );
		return -4;
	}
}

void CNFGTackPixelBlend( int x, int y, int ta )
{
	if( x < 0 || y < 0 || x >= CNFGBufferx || y >= CNFGBuffery ) return;
	uint32_t tc = CNFGLastColor;
	uint32_t lc = CNFGBuffer[x+CNFGBufferx*y];

	uint8_t r = (tc>>16);
	uint8_t g = (tc>>8);
	uint8_t b = (tc>>0);
	float a = (ta)/255.0;
	float ia = 1.0 - a;

	r = r * a + ((lc>>16)&0xff) * ia;
	g = g * a + ((lc>>8)&0xff) * ia;
	b = b * a + ((lc>>0)&0xff)  * ia;

	CNFGBuffer[x+CNFGBufferx*y] = (r<<16) | (g<<8) | (b<<0);
}

int DrawGlyph( int c, float x, float y, float size )
{
	if( c < 0 ) return 0;

	float th = stbtt_ScaleForPixelHeight(&font, size );

	unsigned char * fontchars;
	int fontcharsw;
	int fontcharsh;
	int fontcharsxo;
	int fontcharsyo;
	float ffx = x - (int)x;
	float ffy = y - (int)y;

	fontchars = stbtt_GetCodepointBitmapSubpixel( &font, th, th, ffx, ffy, c,
		&fontcharsw, &fontcharsh, &fontcharsxo, &fontcharsyo );

	int w = fontcharsw;
	int h = fontcharsh;

	if( fontchars )
	{
		x += fontcharsxo;
		y += fontcharsyo + size/2;

		uint8_t * p = fontchars;
		int tx, ty;
		for( tx = 0; tx < w; tx++ )
		for( ty = 0; ty < h; ty++ )
		{
			int tc = p[w*ty+tx];
			//CNFGColor( 0xffffffff ); //(tc<<24) | (tc<<16) | (tc<<8) | 0xff );
			CNFGTackPixelBlend( tx + x, ty + y, tc );
		}
		free( fontchars );
		w += fontcharsxo;
	}
	return ( w > 0 ) ? w : size*.5;
}

void DrawStr( const char * s, float x, float y, float size )
{
	int i;
	int xofs = 0;
	int c;
	for( i = 0; c = s[i]; i++ )
	{
		xofs += DrawGlyph( c, x + xofs, y, size );
	}
}

int GetGlyphWidth( int c, float ffx, float ffy, float size )
{
	if( c < 0 ) return 0;

	float th = stbtt_ScaleForPixelHeight(&font, size);

	ffx = ffx - (int)ffx;
	ffy = ffy - (int)ffy;
	int w = 0;
	int x = 0;
	// CAn't use Box - it's not accurate.
	//	stbtt_GetGlyphBitmapBoxSubpixel( &font, c, th, th, ffx, ffy, &x, 0, &w, 0 );
	uint8_t * r = stbtt_GetCodepointBitmapSubpixel( &font, th, th, ffx, ffy, c,
		&w, 0, &x, 0 );
	free( r );

	w += x;
	return ( w > 0 ) ? w : size*.5;
}

int StrWidth( const char * s, float x, float y, float size )
{
	int xofs = 0;
	int i;
	int c;
	for( i = 0; c = s[i]; i++ )
	{
		int w = GetGlyphWidth( c, x, y, size );
		xofs += w;
		x += w;
	}

	return xofs;
}

#endif
