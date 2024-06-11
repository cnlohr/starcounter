#ifndef _EXTRADRAWUTILS_H
#define _EXTRADRAWUTILS_H

int LoadFont( void );
int DrawGlyph( Olivec_Canvas oc, int c, float x, float y, float size, uint32_t color  );
void DrawStr( Olivec_Canvas oc, const char * s, float x, float y, float size, uint32_t color  );
int GetGlyphWidth( int c, float ffx, float ffy, float size );
int StrWidth( const char * s, float x, float y, float size );
void Spark( int x, int y );
void RenderSparkles( Olivec_Canvas oc );

#endif

#ifdef EXTRADRAWUTILS_IMPLEMENTATION
stbtt_fontinfo font;

#define MAX_SPARKLES 16384
#define SPARKLE_LIFE 60
#define SPARKLE_LAUNCH 80
#define SPARKLE_SIZE 3.5

struct sparkle
{
	int age;
	uint32_t color;
	float rspeed;
	int bx, by;
	float dx, dy;
} sparkles[MAX_SPARKLES];


void Spark( int x, int y )
{
	int i;
	for( i = 0; i < MAX_SPARKLES; i++ )
	{
		struct sparkle * s = &sparkles[i];
		if( s->age < 1 )
		{
			s->age = 1;

			float exppow = pow((rand()%100)/100.0, 0.25); // random power, but favor outside.
			float expang = ((rand()%62831)/1000.0); // radians
			float dx = cos( expang ) * exppow;
			float dy = sin( expang ) * exppow;

			s->rspeed = ((rand()%100)-50)/200.0;
			s->dx = dx;
			s->dy = dy;
			s->bx = x;
			s->by = y;
			s->color = palette[rand()%(sizeof(palette)/sizeof(palette[0]))];
			break;
		}
	}
}


void RenderSparkles( Olivec_Canvas oc )
{
	int i;
	for( i = 0; i < MAX_SPARKLES; i++ )
	{
		struct sparkle * s = &sparkles[i];
		if( s->age <= 0 ) continue;

		if( s->age++ >= SPARKLE_LIFE )
		{
			s->age = 0;
			continue;
		}

		float expand = SPARKLE_LAUNCH * powf( (float)(s->age) / SPARKLE_LIFE, .7 ); // Ease animation out.  Faster at onset.
		int opacity = ( SPARKLE_LIFE - s->age ) * 255 / SPARKLE_LIFE;
		uint32_t color = ( s->color & 0xffffff) | (opacity<<24);
		float ox = s->bx + ( s->dx * expand );
		float oy = s->by + ( s->dy * expand ) + pow( s->age * .1, 2.0 );

		if( 1 )
		{
			float rt = s->rspeed * s->age;
			float rs = SPARKLE_SIZE;

			int spoke;
			for( spoke = 0; spoke < 5; spoke++ )
			{
				float a1x = 0;
				float a1y = 0;
				float a2x = sinf( rt ) * rs;
				float a2y = cosf( rt ) * rs;
				float a3x = sinf( rt + 0.628318531 ) * rs * .5;
				float a3y = cosf( rt + 0.628318531 ) * rs * .5;
				float a4x = sinf( rt - 0.628318531 ) * rs * .5;
				float a4y = cosf( rt - 0.628318531 ) * rs * .5;
				olivec_triangle( oc, ox + a1x, oy + a1y, ox + a2x, oy + a2y, ox + a3x, oy + a3y, color );
				olivec_triangle( oc, ox + a1x, oy + a1y, ox + a2x, oy + a2y, ox + a4x, oy + a4y, color );
				olivec_line( oc, ox + a2x, oy + a2y, ox + a3x, oy + a3y, 0x00ffffff | ( opacity/2) );
				olivec_line( oc, ox + a2x, oy + a2y, ox + a4x, oy + a4y, 0x00ffffff | ( opacity/2) );
				rt += 1.25663704;
			}
		}
		else
		{
			int tx = (int)ox;
			int ty = (int)oy;
			if( tx >= 0 && ty >= 0 & tx < oc.width && ty < oc.height )
				olivec_blend_color( &OLIVEC_PIXEL(oc, tx, ty ), color );
		}
	}
}

float HatchTexture( float x, float y, float rscale )
{
	const float ca = cos( 1.25663704 );
	const float sa = sin( 1.25663704 );
	float ofcx = x * ca + y * sa * rscale;
	float ofcy =-x * sa * rscale + y * ca;

	ofcx += 10000;
	ofcy += 10000;
	ofcx *= .2;
	ofcy *= .2;
	ofcx -= (int)ofcx;
	ofcy -= (int)ofcy;

	ofcx = fabsf( ofcx - 0.5 ) * 2.0;
	ofcy = fabsf( ofcy - 0.5 ) * 2.0;

	if( ofcx < ofcy )
		return ofcx;
	else
		return ofcy;
}

void RenderBackground( Olivec_Canvas oc )
{
	int x, y;

	for( y = 0; y < oc.height; y++ )
	for( x = 0; x < oc.width; x++ )
	{
		//int intensity = 0x12;
		//if( HatchTexture( x, y, 1 ) < 0.5 ) // pi * 2 / 5
		//	intensity = 0x20;
		int intensity = 18 + HatchTexture( x, y, 1 ) * 14;

		uint32_t color = (intensity | (intensity<<8) | (intensity<<16) );
		OLIVEC_PIXEL(oc, x, y ) = 0xff000000 | color;
	}
}

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

float LerpSample( uint32_t * tsd, float place, int minp, int maxp )
{
	int plower = place;
	int pupper = plower+1;

	if( plower < minp ) plower = minp;
	if( pupper < minp ) pupper = minp;
	if( plower >= maxp ) plower = maxp-1;
	if( pupper >= maxp ) pupper = maxp-1;

	float fp = place - plower;

	if( fp < 0 ) fp = 0;
	if( fp > 1 ) fp = 1;

	uint32_t sample_lower = tsd[plower];
	uint32_t sample_upper = tsd[pupper];

	return ( sample_lower * ( 1.0 - fp ) ) + sample_upper * fp;
}

void DrawPrettyRect( Olivec_Canvas oc, int x, int y, float rw, float rh, uint32_t color, uint32_t * ts, uint32_t * tsd, int place, int maxp, int allow_spark )
{
	uint32_t upper = color;
	uint32_t lower = color;
	olivec_blend_color( &upper, 0x80ffffff );
	olivec_blend_color( &lower, 0x80000000 );

	//	olivec_rect( oc,  x+1, y+1, rw-2, rh-2, color );

	float rmpx = x + rw - 2;
	float rmpy = y + rh/2 - 2;

	int lx, ly;
	for( ly = y+1; ly <= y + rh - 2; ly++ )
	for( lx = x+1; lx <= x + rw - 2; lx++ )
	{
		if( lx < 0 || ly < 0 || lx >= oc.width || ly >= oc.height ) continue;

		int lcolor = 0xff000000;
		lcolor |= color;


		// Generate pretty samples over time showing boosting.
		float dx = (lx - rmpx);
		float dy = (ly - rmpy);
		float histpl = place + sqrtf( dx*dx + dy*dy );
		float opacity = LerpSample( tsd, histpl, 0, maxp );
		int out_opacity = sqrt( opacity ) * 32;  // Make a few stars have some impact, and more stars have more, but not linearly.
		if( out_opacity > 255 ) out_opacity = 255;
		if( out_opacity < 0 ) out_opacity = 0;
		int lcboost = ( ((int)out_opacity) << 24) | 0xffffff;

		// Allow a light hatching. Align to right edge.
		int hatchIntensity = 0x00 + HatchTexture( (lx - rw - place)*1.2, (ly-y)*1.2, -1 ) * 24;
		uint32_t hatchColor = (hatchIntensity << 24) | 0x000000;

		olivec_blend_color( &lcolor, hatchColor );
		olivec_blend_color( &lcolor, lcboost );

		olivec_blend_color( &OLIVEC_PIXEL(oc, lx, ly ), lcolor );
	}

	olivec_line( oc, x, y, x, y+rh-1, upper );
	olivec_line( oc, x, y, x+rw-1, y, upper );
	olivec_line( oc, x + rw-1, y, x+rw-1, y+rh-1, lower );
	olivec_line( oc, x, y + rh-1, x+rw-1, y+rh-1, lower );

	if( allow_spark && place <= maxp )
	{
		int se;
		for( se = 0; se < tsd[place]; se++ )
			Spark( rmpx, rmpy );
	}
}

int DrawGlyph( Olivec_Canvas oc, int c, float x, float y, float size, uint32_t color )
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
	color &= 0xffffff;

	if( fontchars )
	{
		x += fontcharsxo;
		y += fontcharsyo + size/2;

		int tx, ty;
		for( tx = 0; tx < w; tx++ )
		for( ty = 0; ty < h; ty++ )
		{
			int tc = (fontchars[w*ty+tx] << 24) | color;
			//CNFGColor( 0xffffffff ); //(tc<<24) | (tc<<16) | (tc<<8) | 0xff );
			//CNFGTackPixelBlend( oc, tx + x, ty + y, tc );
			int lx = (int32_t)x + tx;
			int ly = (int32_t)y + ty;
			if( lx < 0 || lx >= oc.width || ly < 0 || ly >= oc.height ) continue;
			olivec_blend_color( &OLIVEC_PIXEL(oc, lx, ly ), tc );
		}
		free( fontchars );
		w += fontcharsxo;
	}
	return ( w > 0 ) ? w : size*.5;
}

void DrawStr( Olivec_Canvas oc, const char * s, float x, float y, float size, uint32_t color )
{
	int i;
	int xofs = 0;
	int c;
	for( i = 0; c = s[i]; i++ )
	{
		xofs += DrawGlyph( oc, c, x + xofs, y, size, color );
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
