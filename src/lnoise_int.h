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


#define Lerp(T,A,B) ((B-A)*T+A)

/* this curve is continuous up to the 5th derivative */
/*
#define Curve(A) (tmp =(1-cos(A*M_PI))*0.5, ((6*tmp-15)*tmp+10)*tmp*tmp*tmp)
*/
/*
#define Curve(A) (tmp =(1-cos(A*M_PI))*0.5, ((-2*tmp+3)*tmp*tmp))
*/
/*
#define Curve(A) (((6*A-15)*A+10)*A*A*A)
*/
/*
#define Curve(A) ((1-cos(A*M_PI))*0.5)
*/

#define Curve(A) (tmp = A*A, ((((-20.0 * A) + 70.0) * A - 84.0) * A + 35.0) * tmp*tmp)


