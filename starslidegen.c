#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define GIFENC_IMPLEMENTATION
#include "gifenc.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#define CNFG_IMPLEMENTATION
#include "rawdraw_sf.h"

void HandleKey( int keycode, int bDown ) { }
void HandleButton( int x, int y, int button, int bDown ) { }
void HandleMotion( int x, int y, int mask ) { }
int HandleDestroy() { return 0; }

char ** reponames = 0;
int numreponames = 0;

#define WINDOWW 960
#define WINDOWH 1080
uint32_t framebuffer[WINDOWW*WINDOWH];

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
	int i;
	char buff[1024];

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
			reponames[numreponames-1] = strdup( s );
		}
		numstars++;
		allstars = realloc( allstars, sizeof( allstars[0] ) * numstars );
		struct allstarentry * e = allstars + numstars - 1;
		e->reponame = reponames[i];
		e->repoid = i;
		e->startime = startime;
	}
	fclose( f );

	starttime = floor( starttime / (3600*24) ) * (3600*24);

	struct repoentry res[numreponames];
	memset( res, 0, sizeof( res ) );

	double ttime;
	for( ttime = starttime; ttime <= endtime; ttime += 3600*24 )
	{
		struct repoentry_sort repsort[numreponames];

		for( i = 0; i < numreponames; i++ )
		{
			res[i].count = 0;
			res[i].repid = i;
			repsort[i].count = 0;
			repsort[i].repid = i;
		}

		for( i = 0; i < numstars; i++ )
		{
			struct allstarentry * e = allstars + i;
			if( e->startime <= ttime )
			{
				res[e->repoid].count++;
				repsort[e->repoid].count++;
			}
		}

		qsort( repsort, numreponames, sizeof( repsort[0] ), (int(*)(const void*,const void*))repocompare );

		memset( framebuffer, 0, sizeof( framebuffer ) );

		int i;
		for( i = 0; i < numreponames; i++ )
		{
			struct repoentry * e = res + reposort[i].repid;
			e->targetplace = i;
			if( e->count == 0 && reposort[i].count != 0 )
				e->place = e->targetplace;
			e->count = reposort[i].count;
			double placediff = e->targetplace - e->place;
			e->place += placediff * 0.01;

			
		}

		
		printf( "\n" );
	}


	return 0;
}
