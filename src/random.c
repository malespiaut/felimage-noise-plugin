/* Based on the multiply-with-carry generator in the 
 * Marsaglia's "Diehard" battery of statistical tests
 *
 * x(n) = RANDOM_FACTOR*x(n-1) + carry mod 2^32.
 * period = (RANDOM_FACTOR*2^31)-1
 */
 
#ifdef CALIBRATE
#include <glib.h>
#else
#include <libgimp/gimp.h>
#endif

#include "random.h"

#define RANDOM_FACTOR 1791398085

static guint32 rand_x, rand_c, rand_ah, rand_al;

guint16 *global_shuffle_table=NULL;

guint32 Random() {
   guint32 xh = rand_x>>16, xl = rand_x & 65535;

   rand_x = rand_x * RANDOM_FACTOR + rand_c;
   rand_c = xh*rand_ah + ((xh*rand_al) >> 16) + ((xl*rand_ah) >> 16);
   if (xl*rand_al >= ~rand_c + 1) rand_c++; 
   return rand_x;
}



void SetRandomSeed(guint32 seed) {
	rand_x=30903;
	rand_c = seed;
	rand_ah = RANDOM_FACTOR>>16;
	rand_al = RANDOM_FACTOR&65535;
}


void InitShuffleTable(guint32 seed) {
	guint32 i,j,t;
	int fi,bj,fj;
	guint16 *back_shuffle_table;

	SetRandomSeed(seed);
	
	back_shuffle_table = g_malloc(TABLE_SIZE * sizeof(guint16));
	
	if (!global_shuffle_table) {
		global_shuffle_table = g_malloc(TABLE_SIZE * sizeof(guint16));
	}
	
	for (i = 0; i < TABLE_SIZE; i++) {
		back_shuffle_table[i]   = (i + TABLE_SIZE - 1) & (TABLE_SIZE-1);
		global_shuffle_table[i] = (i+1)&(TABLE_SIZE-1);
	}
	
	for (i = 0; i < TABLE_SIZE; i++) {
		j = Random() >> (32 - TABLE_SIZE_LOG);
		bj = back_shuffle_table[j];
		fi = global_shuffle_table[i];
		fj = global_shuffle_table[j];
		
		t = global_shuffle_table[i];
		global_shuffle_table[i] = global_shuffle_table[bj];
		global_shuffle_table[bj] = global_shuffle_table[j];
		global_shuffle_table[j] = t;

		t = back_shuffle_table[fi];
		back_shuffle_table[fi] = back_shuffle_table[fj];
		back_shuffle_table[fj] = back_shuffle_table[j];
		back_shuffle_table[j] = t;
	}

	g_free(back_shuffle_table);

}

void FinishShuffleTable() {
	if (global_shuffle_table) {
		g_free(global_shuffle_table);
	}
	global_shuffle_table = NULL;
}
