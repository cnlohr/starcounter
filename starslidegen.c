#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#define CNFG_IMPLEMENTATION
#include "rawdraw_sf.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define OLIVEC_IMPLEMENTATION
#include "olive.c"

#define WINDOWW 1920
#define WINDOWH 1080
uint32_t framebuffer[WINDOWW*WINDOWH];
uint32_t palette[1024];

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

#define CONTINUE_PAST_FRAMES 120
#define GLYPHH 36

#define RHEIGHT ((GLYPHH)+1)
#define XMARGIN 300
#define RIGHTMARGIN 100
#define TOPMARGIN 15
#define XOFS 10
#define YOFS 10
#define RWIDTH (w-XOFS*2 - XMARGIN - RIGHTMARGIN)
#define RMARGINA XOFS

#define TEXTYOFS (GLYPHH/4)
#define MAXGLYPHS 127

#define MAX_COLUMNS 6
uint32_t usercolors[MAX_COLUMNS] = { 0xff0000ff, 0xffff0000, 0xff00ff00, 0xffffff00, 0xffff00ff, 0xff00ffff };

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
	uint32_t * star_time_series;
	uint32_t * star_time_series_diff;
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


///////////////////////////////////////////////////////////////////////////////////
// Sound Effects

#define MAX_CHIRPS 1024

struct tick_stat
{
	int active;
	int column;
	float place;
	float foffset;
};

double tick_count_running[MAX_COLUMNS];
struct tick_stat ticks[MAX_CHIRPS];

float TickSFX( struct tick_stat * c )
{
	if( !c->active ) return 0;
	const int impact_length = 5000;

	// Creates a cute impulse.
	float attack = (c->place/300);
	if( attack > 1.0 ) attack = 1.0;
	float decay =  pow(1 - c->place / impact_length, 3);

	float fCore = c->foffset * (( 9 + c->column * 4 ) ) * c->place / ( c->place + 1000 );

	float ret = sin( 6.28 * fCore ) * decay * attack;  // Fundamental
	ret += sin( 3.0*6.28 *  fCore ) * decay * attack * .8;  // 3rd overtone
	ret += sin( 5.0*6.28 *  fCore ) * decay * attack * .6; // 5th overtone.
	ret += sin( 9.0*6.28 *  fCore ) * decay * attack * .5; // 9th overtone.
	ret += sin( 12.0*6.28 * fCore ) * decay * attack * .4; // 12th overtone.
	ret += sin( 15.0*6.28 * fCore ) * decay * attack * .3; // 15th overtone.
	ret += sin( 17.0*6.28 * fCore ) * decay * attack * .2; // 17th overtone.

	c->place++;

	if( c->place > impact_length ) c->active = 0;
	return ret;
}

void AddTick( int column )
{
	int i;
	for( i = 0; i < MAX_CHIRPS; i++ )
	{
		struct tick_stat * t = &ticks[i];
		if( t->active ) continue;
		t->active = 1;
		t->place = 0;
		t->foffset = ((rand()%1000)/1000.0)+1.0;
		t->column = column;
		break;
	}
}

void AddAudioFrames( FILE * fAudio, int nAudioSamples, int * diffs, int columns )
{
	int i;
	for( i = 0; i < columns; i++ )
	{
		tick_count_running[i] += diffs[i];
	}

	for( i = 0; i < nAudioSamples; i++ )
	{
		int j;

		for( j = 0; j < columns; j++ )
		{
			// Randomly speckle the pops.
			double pressure = log( tick_count_running[j] + 1 );
			if( pressure > (rand()%1000)/5.0 )
			{
				AddTick( j );
				tick_count_running[j]--;
			}
		}

		float fSample = 0.0;

		for( j = 0; j < MAX_CHIRPS; j++ )
		{
			if( ticks[j].active )
				fSample += TickSFX( &ticks[j] ) * .03;
		}

		if( fSample > 1.0 ) fSample = 1.0;
		if( fSample <-1.0 ) fSample =-1.0;
		fwrite( &fSample, 4, 1, fAudio );
	}
}

///////////////////////////////////////////////////////////////////////////////////

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
		palette[i] = (r<<16) | (g<<8) | (b<<0) | 0xff000000;
	}

    Olivec_Canvas oc = olivec_canvas( framebuffer, WINDOWW, WINDOWH, WINDOWW );

	CNFGSetup( "starcounter", WINDOWW, WINDOWH );

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
			if( !mid )
			{
				fprintf( stderr, "error: Repo %s does not have a user/account name\n", s );
				return -12;
			}
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


	int expframenumbers = (endtime - starttime) / frametime + 1;

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

	// Continue past the end, to let everything stabilize.
	endtime += CONTINUE_PAST_FRAMES*(frametime);


	for( i = 0; i < numreponames; i++ )
	{
		struct repoentry * e = &res[i];
		e->star_time_series = calloc( sizeof(uint32_t), expframenumbers + 1 );
		e->star_time_series_diff = calloc( sizeof(uint32_t), expframenumbers + 1 );
	}

	uint32_t * star_time_series[columns], * star_time_series_diff[columns];

	for( ttime = starttime; ttime <= endtime; ttime += frametime )
	{
		if( framenumber >= expframenumbers ) continue;

		for( i = 0; i < numstars; i++ )
		{
			struct allstarentry * e = allstars + i;
			if( e->startime <= ttime )
			{
				struct repoentry * r = &res[e->repoid];
				r->star_time_series[framenumber+1]++;
				int diff = r->star_time_series[framenumber+1] - r->star_time_series[framenumber];
				r->star_time_series_diff[framenumber] = diff;
			}
		}

		framenumber ++;;
	}

	for( i = 0; i < columns; i++ )
	{
		star_time_series[i] = calloc( sizeof(uint32_t), expframenumbers + 1 );
		star_time_series_diff[i] = calloc( sizeof(uint32_t), expframenumbers + 1 );
		int j;
		for( j = 0; j < expframenumbers; j++ )
		{
			int k;
			for( k = 0; k < numreponames; k++ )
			{
				if( strcmp( colnames[i], reponames_user[k] ) == 0 )
				{
					struct repoentry * r = &res[k];
					star_time_series[i][j] += r->star_time_series[j];
					star_time_series_diff[i][j] += r->star_time_series_diff[j];
				}
			}
		}
	}


	int datapoints[expframenumbers][columns];
	memset( datapoints, 0, sizeof( datapoints ) );


	FILE * fAudio = fopen( "audio.dat", "wb" );
	float audiolast = 0;
	double audiophase = 0;
	int laststars[columns];
	memset( laststars, 0, sizeof( laststars ) );

	framenumber = 0;

	for( ttime = starttime; ttime <= endtime; ttime += frametime )
	{
		// Allow user to exit.
		if( !CNFGHandleInput() ) break;

		int framenumber_limited = ( framenumber < expframenumbers ) ? framenumber : expframenumbers;

		struct repoentry_sort repsort[numreponames];
		char cts[1024]; // Temp string for sprintf'ing into.

		RenderBackground( oc );

		short tw = WINDOWW;
		short h = WINDOWH;
		short w = tw / columns;

		// Compute Max Stars (total)
		int maxstars = 0;
		int usercombmaxstars[columns];
		memset( usercombmaxstars, 0, sizeof( usercombmaxstars ) );
		int usermaxstars = 0;

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

				int ty0 = datapoints[lf-1][col] + (1 - col);
				int ty1 = datapoints[lf][col] + (1 - col );

				float x0 = lf    * (float)( graphendx - graphstartx ) / expframenumbers + graphstartx;
				float y0 = ty0   * (float)( graphendy - graphstarty ) / usermaxstars_total + graphstarty;
				float x1 = (lf+1)* (float)( graphendx - graphstartx ) / expframenumbers + graphstartx;
				float y1 = ty1   * (float)( graphendy - graphstarty ) / usermaxstars_total + graphstarty;

				int tx, ty;
				for( ty = 0; ty < 2; ty++ )
				for( tx = 0; tx < 2; tx++ )
					olivec_line( oc, x0+tx, y0+ty, x1+tx, y1+ty, usercolors[col] );
			}

			// Sort the repositories.
			qsort( repsort, numreponames, sizeof( repsort[0] ), (int(*)(const void*,const void*))repocompare );

			float strwidth = StrWidth( colnames[col], 0, 0, GLYPHH );
			DrawStr( oc, colnames[col], XMARGIN - strwidth + cxo, YOFS + TEXTYOFS + TOPMARGIN / 2, GLYPHH, usercolors[col] );

			if( usermaxstars )
			{
				float rw = (float)( RWIDTH ) * usercombmaxstars[col] / (usermaxstars);
				float rh = RHEIGHT - 4;

				float starswidth = StrWidth( "stars", 0, 0, GLYPHH );
				const int starsinblack = 
					//(columns == 1 );
					rw > ( starswidth + XMARGIN );

				if( starsinblack )
					sprintf( cts, "%d", tstars );
				else
					sprintf( cts, "%d stars", tstars );
				float tlen = StrWidth( cts, 0, 0, GLYPHH );
				float x = XOFS + XMARGIN + cxo;
				float y = YOFS + ( -1 * RHEIGHT + 0.5 ) + RHEIGHT + TOPMARGIN / 2;

				DrawStr( oc, cts, x + rw + RMARGINA, YOFS + TEXTYOFS + TOPMARGIN/2, GLYPHH, 0xffffffff );
				DrawPrettyRect( oc, x, y, ((int)rw), rh, 0xff222222, star_time_series[col], star_time_series_diff[col], framenumber, expframenumbers, 0 );

				if( starsinblack )
				{
					sprintf( cts, "stars" );
					float tx = x + rw + RMARGINA;
					float ty = YOFS + TEXTYOFS + TOPMARGIN/2;
					int strwidth = StrWidth( cts, XMARGIN + cxo, ty, GLYPHH );
					DrawStr( oc, cts, x + ((int)rw) - RMARGINA - strwidth, ty, GLYPHH, 0xffffffff );
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
				float y = YOFS + ( e->place * RHEIGHT + 0.5 ) + RHEIGHT + TOPMARGIN;
				float rw = (float)RWIDTH * e->count / (maxstars);
				float rh = RHEIGHT - 4;

				DrawPrettyRect( oc, x, y, rw, rh, palette[rid], e->star_time_series, e->star_time_series_diff, framenumber_limited, expframenumbers, 1 );

				//printf( "%d %d %d %d\n", x+1, y+1, x+rw-1, y+rh-1 );
				snprintf( cts, sizeof( cts ), "%d", e->count );
				int strwidth = StrWidth( reponames_repo[e->repid], XMARGIN + cxo, y + TEXTYOFS, GLYPHH );
				DrawStr( oc, reponames_repo[e->repid], XMARGIN - strwidth + cxo, y + TEXTYOFS, GLYPHH, 0xffffffff );
				DrawStr( oc, cts, x+rw+RMARGINA, y + TEXTYOFS, GLYPHH, 0xffffffff );
			}
		}


		time_t timer = ttime;
		static struct tm * tm_info;
		// Stop updating date at end.
		if( framenumber < expframenumbers )
			tm_info = gmtime(&timer);
		strftime( cts, sizeof( cts ), "%Y-%m-%d", tm_info);
		int tlen = StrWidth( cts, tw - 0 - RMARGINA, h - TEXTYOFS*4-RMARGINA, GLYPHH*2 );

		const int DATESY = 60;
		const int DATEMX = 187;
		int xo, yo;

		int centerx = tw - RMARGINA - DATEMX;

		// Add back shadow.
		for( yo = -1; yo < 2; yo++ )
		for( xo = -1; xo < 2; xo++ )
			DrawStr( oc, cts, centerx - tlen / 2 + xo, h - TEXTYOFS*4-RMARGINA + yo, GLYPHH*2, 0xff000000 );
		DrawStr( oc, cts, centerx - tlen / 2, h - TEXTYOFS*4-RMARGINA, GLYPHH*2, 0xffffffff );

		// Draw starcounter url, but shadowed.
		snprintf( cts, sizeof( cts ), "https://github.com/cnlohr/starcounter" );

		int github_len = StrWidth( cts, tw - 0 - RMARGINA, h - TEXTYOFS*4-RMARGINA, GLYPHH*.52 );

		for( yo = -1; yo < 2; yo++ )
		for( xo = -1; xo < 2; xo++ )
			DrawStr( oc, cts, centerx - github_len/2 + xo, h-TEXTYOFS-RMARGINA + yo - DATESY, GLYPHH*.52, 0xff000000 );
	        DrawStr( oc, cts, centerx - github_len/2,      h-TEXTYOFS-RMARGINA      - DATESY, GLYPHH*.52, 0xffffffff );


		RenderSparkles( oc );


		// Output bitmap (or can be png)
		for( i = 0; i < WINDOWW*WINDOWH; i++ )
			framebuffer[i] |= 0xff000000;
		sprintf( cts, "framedata/%06d.bmp", framenumber );
		stbi_write_bmp( cts, WINDOWW, WINDOWH, 4, framebuffer );

		for( i = 0; i < WINDOWW*WINDOWH; i++ )
			framebuffer[i] = framebuffer[i];
		CNFGBlitImage( framebuffer, 0, 0, WINDOWW, WINDOWH );

		// Swap window buffers
		CNFGSwapBuffers();


		framenumber++;

		int diff[columns];
		for( i = 0; i < columns; i++ )
		{
			diff[i] = usercombmaxstars[i] - laststars[i];
			laststars[i] = usercombmaxstars[i];
		}

		// 48000/60
		AddAudioFrames( fAudio, 48000/60, diff, columns );
	}


	return 0;
}
