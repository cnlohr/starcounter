#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#define CNFGRASTERIZER
#define CNFG_IMPLEMENTATION
#include "rawdraw_sf.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define EXTRADRAWUTILS_IMPLEMENTATION
#include "extradrawutils.h"

void HandleKey( int keycode, int bDown ) { }
void HandleButton( int x, int y, int button, int bDown ) { }
void HandleMotion( int x, int y, int mask ) { }
int HandleDestroy() { return 0; }

char ** reponames = 0;
char ** reponames_user = 0;
char ** reponames_repo = 0;
int numreponames = 0;

#define WINDOWW 1920
#define WINDOWH 1080

#define CONTINUE_PAST_FRAMES 200
#define GLYPHH 36

#define RHEIGHT ((GLYPHH)+1)
#define XMARGIN 300
#define RIGHTMARGIN 100
#define XOFS 10
#define YOFS 10
#define RWIDTH (w-XOFS*2 - XMARGIN - RIGHTMARGIN)
#define RMARGINA XOFS

#define TEXTYOFS (GLYPHH/4)
#define MAXGLYPHS 127

#define MAX_COLUMNS 6
uint32_t usercolors[MAX_COLUMNS] = { 0xff0000ff, 0x0000ffff, 0x00ff00ff, 0xffff00ff, 0xff00ffff, 0x00ffffff };

uint32_t framebuffer[WINDOWW*WINDOWH];

uint32_t palette[1024];

struct allstarentry
{
	const char * reponame;
	int repoid;
	double startime;
} * allstars = 0;
int numstars = 0;

struct repoentry
{
	int repid;
	int count;
	double place;
	int targetplace;
};

struct repoentry_sort
{
	int count;
	int repid;
};

int repocompare( const struct repoentry_sort * a, struct repoentry_sort * b )
{
	if( a->count > b->count ) return -1;
	if( a->count < b->count ) return 1;
	return strcmp( reponames[a->repid], reponames[b->repid] );
}

int main( int argc, const char ** argv )
{
	double frametime = 24*3600;

	int i;
	char buff[1024];

	int r = LoadFont();

	if( r < 0 ) return r;

	mkdir( "framedata", 0777 );

	// Make some random palette colors.
	srand( 1 );
	for( i = 0; i < sizeof(palette)/sizeof(palette[0]); i++ )
	{
		int r = rand()%8192;
		int g = rand()%8192;
		int b = rand()%8192;
		int mc = r;
		if( mc > g ) mc = g;
		if( mc > b ) mc = b;
		r -= mc; g -= mc; b -= mc;
		float rm = 255.0 / sqrtf(r*r+g*g+b*b);
		r *= rm; g *= rm; b *= rm;
		palette[i] = (r<<24) | (g<<16) | (b<<8) | 0xff;
	}

	CNFGSetup( "starcounter", WINDOWW, WINDOWH );

	CNFGBGColor = 0x121212ff;

	if( argc < 2 )
	{
		fprintf( stderr, "Error: usage: starslidegen allstarlist.txt\n" );
		return -5;
	}

	const char * asl = argv[1];

	FILE * f = fopen( asl, "r" );
	if( !f )
	{
		fprintf( stderr, "Error can't open %s\n", asl );
		return -6;
	}

	double starttime = 0;
	double endtime = 0;

	// Read in data.
	while( !feof( f ) )
	{
		char * s = fgets( buff, sizeof(buff), f );
		if( !s ) break;
		char * comma = strchr( s, ',' );
		if( !comma ) continue;
		*comma  = 0;
		double startime = atof( comma + 1 );
		if( starttime == 0 || startime < starttime )
			starttime = startime;
		if( endtime == 0 || startime > endtime )
			endtime = startime;

		for( i = 0; i < numreponames; i++ )
		{
			if( strcmp( s, reponames[i] ) == 0 ) break;
		}
		if( i == numreponames )
		{
			numreponames++;
			reponames = realloc( reponames, sizeof( reponames[0] ) * numreponames );
			reponames_user = realloc( reponames_user, sizeof( reponames_user[0] ) * numreponames );
			reponames_repo = realloc( reponames_repo, sizeof( reponames_user[0] ) * numreponames );
			reponames[numreponames-1] = strdup( s );
			char * mid = strchr( s, '/' );
			*mid = 0;
			mid++;
			reponames_user[numreponames-1] = strdup( s );
			reponames_repo[numreponames-1] = strdup( mid );
		}
		numstars++;
		allstars = realloc( allstars, sizeof( allstars[0] ) * numstars );
		struct allstarentry * e = allstars + numstars - 1;
		e->reponame = reponames[i];
		e->repoid = i;
		e->startime = startime;
	}
	fclose( f );

	starttime = floor( starttime / (frametime) ) * (frametime);

	struct repoentry res[numreponames];
	memset( res, 0, sizeof( res ) );

	double ttime;
	int framenumber = 0;
	int columns = 1;
	const char * colnames[MAX_COLUMNS];

	colnames[0] = reponames_user[0];

	// Develop column names
	for( i = 1; i < numreponames; i++ )
	{
		const char * user = reponames_user[i];
		int j;
		for( j = 0; j < columns; j++ )
		{
			if( strcmp( user, colnames[j] ) == 0 )
				break;
		}

		if( j == columns )
		{
			if( columns+1 >= MAX_COLUMNS )
			{
				fprintf( stderr, "Error: too many users\n" );
				exit( -15 );
			}
			colnames[columns++] = user;
		}
	}

	// Figure out overall user max.
	int usercombmaxstars_total[columns];
	memset( usercombmaxstars_total, 0, sizeof( usercombmaxstars_total ) );
	int usermaxstars_total = 0;
	for( i = 0; i < numstars; i++ )
	{
		struct allstarentry * e = allstars + i;
		res[e->repoid].count++;
		int col;
		for( col = 0; col < columns; col++ )
		{
			if( strcmp( reponames_user[e->repoid], colnames[col] ) == 0 )
			{
				usercombmaxstars_total[col]++;
				if( usercombmaxstars_total[col] > usermaxstars_total )
					usermaxstars_total = usercombmaxstars_total[col];
			}
		}
	}

	int expframenumbers = (endtime - starttime) / frametime + 1;
	int datapoints[expframenumbers][columns];
	memset( datapoints, 0, sizeof( datapoints ) );

	// Continue past the end, to let everything stabilize.
	endtime += CONTINUE_PAST_FRAMES*(frametime);

	FILE * fAudio = fopen( "audio.dat", "wb" );
	float audiolast = 0;
	double audiophase = 0;
	double audioticksaccum = 0;
	int laststars = 0;

	for( ttime = starttime; ttime <= endtime; ttime += frametime )
	{
		// Allow user to exit.
		if( !CNFGHandleInput() ) break;

		struct repoentry_sort repsort[numreponames];
		short tw,h;
		char cts[1024]; // Temp string for sprintf'ing into.

		CNFGGetDimensions( &tw, &h );
		CNFGClearFrame();
		short w = tw / columns;

		// Compute Max Stars (total)
		int maxstars = 0;
		int usercombmaxstars[columns];
		memset( usercombmaxstars, 0, sizeof( usercombmaxstars ) );
		int usermaxstars = 0;
		int current_total_stars = 0;

		for( i = 0; i < numreponames; i++ )
		{
			res[i].count = 0;
			res[i].repid = i;
		}

		for( i = 0; i < numstars; i++ )
		{
			struct allstarentry * e = allstars + i;
			if( e->startime <= ttime )
			{
				res[e->repoid].count++;
				int col;
				for( col = 0; col < columns; col++ )
				{
					if( strcmp( reponames_user[e->repoid], colnames[col] ) == 0 )
					{
						usercombmaxstars[col]++;
						if( usercombmaxstars[col] > usermaxstars )
							usermaxstars = usercombmaxstars[col];
					}
				}
				current_total_stars++;
			}
		}

		for( i = 0; i < numreponames; i++ )
		{
			if( res[i].count > maxstars ) maxstars = res[i].count;
		}

		// Iterate per user.
		int col;
		for( col = 0; col < columns; col++ )
		{
			int cxo = col * w;

			for( i = 0; i < numreponames; i++ )
			{
				res[i].count = 0;
				res[i].repid = i;
				repsort[i].count = 0;
				repsort[i].repid = i;
			}

			int tstars = 0;

			for( i = 0; i < numstars; i++ )
			{
				struct allstarentry * e = allstars + i;
				if( strcmp( reponames_user[e->repoid], colnames[col] ) != 0 )
				{
					continue;
				}
				if( e->startime <= ttime )
				{
					res[e->repoid].count++;
					repsort[e->repoid].count++;
					tstars++;
				}
			}

			// Only compute up to the expected end.
			if( framenumber < expframenumbers )
				datapoints[framenumber][col] = tstars;

			// Draw Graph
			float graphstartx = tw/3 + 10 + XOFS;
			float graphendx   = tw - XOFS;
			float graphstarty = h - 100;
			float graphendy   = h * 1 / 3;
			int lf = 0;
			for( lf = 1; lf < framenumber; lf++ )
			{
				if( lf >= expframenumbers ) break;

				CNFGColor( usercolors[col] );
				int ty0 = datapoints[lf-1][col] + (1 - col);
				int ty1 = datapoints[lf][col] + (1 - col );

				float x0 = lf    * (float)( graphendx - graphstartx ) / expframenumbers + graphstartx;
				float y0 = ty0   * (float)( graphendy - graphstarty ) / usermaxstars_total + graphstarty;
				float x1 = (lf+1)* (float)( graphendx - graphstartx ) / expframenumbers + graphstartx;
				float y1 = ty1   * (float)( graphendy - graphstarty ) / usermaxstars_total + graphstarty;
				
				CNFGTackSegment( x0, y0, x1, y1 );
				CNFGTackSegment( x0, y0+1, x1, y1+1 );
				CNFGTackSegment( x0+1, y0, x1+1, y1 );
				CNFGTackSegment( x0+1, y0+1, x1+1, y1+1 );
			}

			// Sort the repositories.
			qsort( repsort, numreponames, sizeof( repsort[0] ), (int(*)(const void*,const void*))repocompare );

			memset( framebuffer, 0, sizeof( framebuffer ) );

			float strwidth = StrWidth( colnames[col], 0, 0, GLYPHH );
			DrawStr( colnames[col], XMARGIN - strwidth + cxo, YOFS + TEXTYOFS, GLYPHH );

			if( usermaxstars )
			{
				if( columns == 1 )
					sprintf( cts, "%d", tstars );
				else
					sprintf( cts, "%d stars", tstars );
				float tlen = StrWidth( cts, 0, 0, GLYPHH );
				float x = XOFS + XMARGIN + cxo;
				float y = YOFS + ( -1 * RHEIGHT + 0.5 ) + RHEIGHT;
				float rw = (float)( RWIDTH ) * usercombmaxstars[col] / (usermaxstars);
				float rh = RHEIGHT - 4;

				CNFGColor( 0xffffffff );
				DrawStr( cts, x + rw + RMARGINA, YOFS + TEXTYOFS, GLYPHH );

				CNFGTackRectangle( x, y, x+rw, y+rh );
				CNFGColor( usercolors[col] );
				CNFGTackRectangle( x+1, y+1, x+rw-1, y+rh-1 );

				if( columns == 1 )
				{
					sprintf( cts, "stars" );
					CNFGColor( 0x000000ff );
					float tx = x + rw + RMARGINA;
					float ty = YOFS + TEXTYOFS;
					int strwidth = StrWidth( cts, XMARGIN + cxo, ty, GLYPHH );
					DrawStr( cts, x + rw - RMARGINA - strwidth, ty, GLYPHH );
				}
			}

			int validpl = 0;
			for( i = 0; i < numreponames; i++ )
			{
				int rid = repsort[i].repid;
				struct repoentry * e = res + rid;
				if( strcmp( reponames_user[e->repid], colnames[col] ) != 0 )
				{
					continue;
				}
				e->targetplace = validpl;
				if( e->count == 0 && repsort[i].count != 0 )
					e->place = e->targetplace;
				e->count = repsort[i].count;
				double placediff = e->targetplace - e->place;
				e->place += placediff * 0.06;

				if( e->count == 0 ) continue;
				validpl++;

				float x = XOFS + XMARGIN + cxo;
				float y = YOFS + ( e->place * RHEIGHT + 0.5 ) + RHEIGHT;
				float rw = (float)RWIDTH * e->count / (maxstars);
				float rh = RHEIGHT - 4;

				CNFGColor( 0xffffffff );
				CNFGTackRectangle( x, y, x+rw, y+rh );
				CNFGColor( palette[rid] );
				CNFGTackRectangle( x+1, y+1, x+rw-1, y+rh-1 );
				//printf( "%d %d %d %d\n", x+1, y+1, x+rw-1, y+rh-1 );
				snprintf( cts, sizeof( cts ), "%d", e->count );
				int strwidth = StrWidth( reponames_repo[e->repid], XMARGIN + cxo, y + TEXTYOFS, GLYPHH );
				CNFGColor( 0xffffffff );
				DrawStr( reponames_repo[e->repid], XMARGIN - strwidth + cxo, y + TEXTYOFS, GLYPHH );
				DrawStr( cts, x+rw+RMARGINA, y + TEXTYOFS, GLYPHH );
			}
		}


		CNFGColor( 0xffffffff );
		time_t timer = ttime;
		static struct tm * tm_info;
		// Stop updating date at end.
		if( framenumber < expframenumbers )
			tm_info = gmtime(&timer);
		strftime( cts, sizeof( cts ), "%Y-%m-%d", tm_info);
		int tlen = StrWidth( cts, tw - 0 - RMARGINA, h - TEXTYOFS*4-RMARGINA, GLYPHH*2 );
		DrawStr( cts, tw - tlen - RMARGINA, h - TEXTYOFS*4-RMARGINA, GLYPHH*2 );

		// Draw starcounter url, but shadowed.
		snprintf( cts, sizeof( cts ), "https://github.com/cnlohr/starcounter" );
		int xo, yo;
		CNFGColor( 0x000000ff );
		for( yo = -1; yo < 2; yo++ )
		for( xo = -1; xo < 2; xo++ )
			DrawStr( cts, RMARGINA + xo, h-TEXTYOFS-RMARGINA + yo, GLYPHH/2 );
		CNFGColor( 0xffffffff );
		DrawStr( cts, RMARGINA, h-TEXTYOFS-RMARGINA, GLYPHH/2 );


		// Swap window buffers
		CNFGSwapBuffers();

		// Output bitmap (or can be png)
		for( i = 0; i < tw*h; i++ )
			CNFGBuffer[i] |= 0xff000000;
		sprintf( cts, "framedata/%06d.bmp", framenumber );
		stbi_write_bmp( cts, tw, h, 4, CNFGBuffer);

		framenumber++;

		int diff = current_total_stars - laststars;
		laststars = current_total_stars;
		audioticksaccum += diff;
		for( i = 0; i < 800; i++ )
		{
			audiophase += audioticksaccum * 0.0002;
			if( audiophase >= 1.0 )
			{
				audiophase--;
				audioticksaccum--;
			}
			audiolast = audiophase * 0.05 - 0.025;
			fwrite( &audiolast, 4, 1, fAudio );
		}
	}


	return 0;
}
