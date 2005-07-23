/*  Felimage Noise Plugin for the GIMP
 *  Copyright (C) 2005 Guillermo Romero Franco <drirr_gato@users.sourceforge.net>
 *
 *  This file is part of the Felimage Noise Plugin for the GIMP
 *  
 *  Felimage Noise Plugin for the Gimp is free software; 
 *  you can redistribute it and/or modify it under the terms of 
 *  the GNU General Public License as published by the Free Software 
 *  Foundation; either version 2 of the License, or (at your option) 
 *  any later version.
 *
 *  Felimage Noise Plugin for the Gimp is distributed in the hope 
 *  that it will be useful, but WITHOUT ANY WARRANTY; without even 
 *  the implied warranty of MERCHANTABILITY or FITNESS FOR A 
 *  PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with fimg-noise; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
*/


unsigned Random();
void SetRandomSeed(guint32 seed);


#define RandomDbl()          ((double)Random() / 4294967296.0)

/* Must be <= 16 */
/* If this is changed, make sure to adjust the 5D lattice noise so that any assymetry
 * ends up in the 't' parameter (the phase) (see lnoise_5d.c)
 */
#define TABLE_SIZE_LOG  10

#define TABLE_SIZE (1<<TABLE_SIZE_LOG)

#define Hash1(A)          global_shuffle_table[ (A)                   &(TABLE_SIZE-1)]
#define Hash2(A,B)        global_shuffle_table[((A) + Hash1(B)       )&(TABLE_SIZE-1)]
#define Hash3(A,B,C)      global_shuffle_table[((A) + Hash2(B,C)     )&(TABLE_SIZE-1)]
#define Hash4(A,B,C,D)    global_shuffle_table[((A) + Hash3(B,C,D)   )&(TABLE_SIZE-1)]
#define Hash5(A,B,C,D,E)  global_shuffle_table[((A) + Hash4(B,C,D,E) )&(TABLE_SIZE-1)]


extern guint16 *global_shuffle_table;


void InitShuffleTable(guint32 seed);
void FinishShuffleTable();
