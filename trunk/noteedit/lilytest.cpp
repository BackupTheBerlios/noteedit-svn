/****************************************************************************************/
/*											*/
/* This program is free software; you can redistribute it and/or modify it under the	*/
/* terms of the GNU General Public License as published by the Free Software		*/
/* Foundation; either version 2 of the License, or (at your option) any later version.	*/
/*											*/
/* This program is distributed in the hope that it will be useful, but WITHOUT ANY	*/
/* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A	*/
/* PARTICULAR PURPOSE. See the GNU General Public License for more details.		*/
/*											*/
/* You should have received a copy of the GNU General Public License along with this	*/
/* program; (See "LICENSE.GPL"). If not, write to the Free Software Foundation, Inc.,	*/
/* 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.				*/
/*											*/
/*--------------------------------------------------------------------------------------*/
/*											*/
/*		Christian Fasshauer							*/
/*		mseacf@gmx.net								*/
/*											*/
/*											*/
/****************************************************************************************/


#include "config.h"
#if GCC_MAJ_VERS > 2
#include <fstream>
#else
#include <fstream.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>

#include "lilytest.h"
#include "resource.h"

using namespace std;

void lilytest::check() {
    printf( "LilyPond check: " );
    fflush( stdout );
    
    NResource::lilyProperties_.lilySemicolons = false;
    
    char tempDir[] = "/tmp/noteedit.XXXXXX";

    mkstemp( tempDir );
    char buf1[256];
    int ver[3];
    
    char *env = getenv( "PATH" );
    char *envp;
    const char *delim = ":";
    bool found = false;
    envp = strtok(env, delim);
    while (!found && envp) {
	    sprintf( buf1, "%s/lilypond", envp );
	    if( ! access( buf1, X_OK ) )
		found = true;
	    envp = strtok(0, delim);
    }
    if (found) {
	strcat( buf1, " --version > " );
	strcat( buf1, tempDir );
	system( buf1 );
	ifstream *is = new ifstream( tempDir, ios::in );
	char buf[50];
	is->getline( buf, 50 );
	is->close();
	delete is;
	remove( tempDir );
	if( sscanf( buf, "GNU LilyPond %i.%i.%i", &ver[0], &ver[1], &ver[2] ) != 3 ) {
		if (sscanf( buf, "lilypond (GNU LilyPond) %i.%i.%i", &ver[0], &ver[1], &ver[2] ) != 3 ) {
			printf( "detection not possible\n" );
			NResource::lilyProperties_.lilyAvailable = false;
			found = false;
		}
	}
    } else {
	printf("not available.\n");
	NResource::lilyProperties_.lilyAvailable = false;
    }
    if (!found) {
        printf("Setting version to 2.6.3\n");
        ver[0] = 2;
        ver[1] = 6;
        ver[2] = 3;
    }
    printf( "found version: %i.%i.%i\n", ver[0], ver[1], ver[2] );
    fflush(stdout);

    int whish[] = { 1, 3, 145 };
    NResource::lilyProperties_.lilySemicolons = ( !this->chkit( &ver[0], &whish[0] ) );

    int whish2[] = { 1, 5, 3 };
    NResource::lilyProperties_.lilyVarTrills = ( this->chkit( &ver[0], &whish2[0] ) );
    NResource::lilyProperties_.lilySluresInGraces = ( this->chkit( &ver[0], &whish2[0] ) );
    NResource::lilyProperties_.lilyAvailable = true;
	
    int whish3[] = { 2, 0, 0 };
	NResource::lilyProperties_.lilyVersion2 = ( this->chkit( &ver[0], &whish3[0] ) );

    int whish4[] = { 2, 2, 0};
	NResource::lilyProperties_.lilyProperties = ( !this->chkit( &ver[0], &whish4[0] ) );

    int whish5[] = { 2, 4, 0};
	NResource::lilyProperties_.lilyVersion24 = this->chkit( &ver[0], &whish5[0] );

    int whish6[] = { 2, 6, 0};
	NResource::lilyProperties_.lilyVersion26 = this->chkit( &ver[0], &whish6[0] );
}

bool lilytest::chkit( int ver[], int matrix[] ) {

    for( int i = 0; i < 3; i++ ) {
	if( ver[i] > matrix[i] )
	    return true;
	if( ver[i] < matrix[i] )
	    return false;
    }
    return true;
} 
    
