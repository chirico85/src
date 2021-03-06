/* 1-D finite-difference wave extrapolation */
/*
  Copyright (C) 2008 University of Texas at Austin
  
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#include <rsf.h>
#include <math.h>
int main(int argc, char* argv[]) 
{
    int nx, nt, ix, it, what, nb, nbx;
    float dt, dx, w, g, a0, a1, a2, a3, a4, a5, pi=3.14159;
    float *old, *nxt, *cur, *sig, *v, *vx, **a; 
    sf_file inp, out, vel, grad;

    sf_init(argc,argv);
    inp = sf_input("in");
    out = sf_output("out");
    vel = sf_input("vel");   /* velocity */
    grad = sf_input("grad"); /* velocity gradient */

    if (SF_FLOAT != sf_gettype(inp)) sf_error("Need float input");
    if (!sf_histint(vel,"n1",&nx)) sf_error("No n1= in input");
    if (!sf_histfloat(vel,"d1",&dx)) sf_error("No d1= in input");
    if (!sf_histint(inp,"n1",&nt)) sf_error("No n2= in input");
    if (!sf_histfloat(inp,"d1",&dt)) sf_error("No d2= in input");

    sf_putint(out,"n1",nx);
    sf_putfloat(out,"d1",dx);
    sf_putint(out,"n2",nt);
    sf_putfloat(out,"d2",dt);

    if (!sf_getint("what",&what)) what=2; /* 2nd or 4th order for FD*/
    if (!sf_getint("nb",&nb)) nb=10; /* 2nd or 4th order for FD*/
    
    nbx = nb + nx;
    sig = sf_floatalloc(nt);
    old = sf_floatalloc(nbx);
    nxt = sf_floatalloc(nbx);
    cur = sf_floatalloc(nbx);
    v = sf_floatalloc(nbx);
    vx = sf_floatalloc(nbx);

    sf_floatread(v,nx,vel);
    sf_floatread(vx,nx,grad);
    
    for (ix= nbx-1; ix > nb-1; ix--) {
        v[ix] = v[ix-nb];
        vx[ix] = vx[ix-nb];
    } 
    
    for (ix=0; ix < nb; ix++){
        v[ix] = v[nb];
        vx[ix] = vx[nb];
    }

    sf_fileclose(vel);
    sf_fileclose(grad);

    sf_floatread(sig,nt,inp);		
    
    switch(what) {
	case 2:
	    /* 2nd order FD*/
	    a = sf_floatalloc2(3,nbx);
	    for (ix=0; ix < nbx; ix++) {
		/* dimensionless velocity */
		w = v[ix] * dt/dx;
		/* dimensionless gradient */
		g = 0.5 * vx[ix] * dt;

		a1 = w*w * (1.0 + g*g);
		a2= g*w;
	
		a[ix][0] = a1+a2;
		a[ix][1] = -2*a1;
		a[ix][2] = a1-a2;

		/* initial conditions */
		cur[ix] = 0.;
		nxt[ix] = 0.;
	    }

	    free(v);
	    free(vx);

	    /* propagation in time */
	    for (it=0; it < nt; it++) {


		for (ix=0; ix < nbx; ix++) {
		    old[ix] = cur[ix];
		    cur[ix] = nxt[ix];
		}   

		/* Stencil */
		nxt[0] = 0;
		for (ix=1; ix < nbx-1; ix++) {
		    nxt[ix] = cur[ix]*a[ix][1] + cur[ix+1]*a[ix][0] + cur[ix-1]*a[ix][2];
		}
		nxt[nbx-1] = cur[nbx-1]*a[nbx-1][1] + cur[nbx-1]*a[nbx-1][0] + cur[nbx-2]*a[nbx-1][2];
	
		for (ix=0; ix < nbx; ix++) {
		    nxt[ix] += 2*cur[ix] - old[ix];
		    if (ix < nb) nxt[ix] *= sin(ix*pi/nb/2); 
		} 
         
		if (it < 500) {nxt[nb] = sig[it];} 
		sf_floatwrite(nxt+nb,nx,out);
	    }
	    break;
        
        case 4:
	    /* 4th order FD*/
	    a = sf_floatalloc2(5,nbx);
     
	    for (ix=0; ix < nbx; ix++) {
		/* dimensionless velocity */
		w = v[ix] * dt/dx;
		/* dimensionless gradient */
		g = vx[ix] * dt;

		a1 = w*w * (4.0 + g*g);
		a2 = w*w*w*w*(16.0+24.0*g*g+g*g*g*g);
		a3 = w*g;
		a4 = w*w*w*g*(12.0+g*g);	
		a[ix][0] = -a1/48.0+a2/192.0-a3/12.0+a4/48.0;
		a[ix][1] = a1/3.0-a2/48.0+2.0*a3/3.0-a4/24.0; 
		a[ix][2] = -5.0*a1/8.0+a2/32.0; 
		a[ix][3] = a1/3.0-a2/48.0-2.0*a3/3.0+a4/24.0; 
		a[ix][4] = -a1/48.0+a2/192.0+a3/12.0-a4/48.0;

		/* initial conditions */
		cur[ix] = 0.;
		nxt[ix] = 0.;
	    }

	    free(v);
	    free(vx);

	    /* propagation in time */
	    for (it=0; it < nt; it++) {

		for (ix=0; ix < nbx; ix++) {
		    old[ix] = cur[ix];
		    cur[ix] = nxt[ix];
		}  

		/* Stencil */
		nxt[0] = 0.0;
		nxt[1] = cur[0]*a[1][4] +cur[0]*a[1][3] +cur[1]*a[1][2] 
		    + cur[2]*a[1][1] + cur[3]*a[1][0];
		for (ix=2; ix < nbx-2; ix++) {
		    nxt[ix] = cur[ix+2]*a[ix][0] + cur[ix+1]*a[ix][1] + cur[ix]*a[ix][2]
			+ cur[ix-1]*a[ix][3] + cur[ix-2]*a[ix][4];
		}
		nxt[nbx-2] = cur[nbx-4]*a[nbx-2][4] + cur[nbx-3]*a[nbx-2][3] + cur[nbx-2]*a[nbx-2][2]
	            + cur[nbx-1]*a[nbx-2][1] + cur[nbx-1]*a[nbx-2][0];
		nxt[nbx-1] = cur[nbx-3]*a[nbx-1][4] + cur[nbx-2]*a[nbx-1][3] + cur[nbx-1]*a[nbx-1][2]
	            + cur[nbx-1]*a[nbx-1][1] + cur[nbx-1]*a[nbx-1][0];
		for (ix=0; ix < nbx; ix++) {
		    nxt[ix] += 2*cur[ix] - old[ix];
		    if (ix < nb) nxt[ix] *= sin(ix*pi/nb/2); 
		}  

		if (it < 500) nxt[nb] = sig[it]; 
		sf_floatwrite(nxt+nb,nx,out);
	    } 
	    break;
 
        case 8:
	    /* 4th order FD*/
	    a = sf_floatalloc2(7,nbx);
     
	    for (ix=0; ix < nbx; ix++) {
		/* dimensionless velocity */
		w = v[ix] * dt/dx;
		/* dimensionless gradient */
		g = vx[ix] * dt;
		a0 = w*g;
		a1 = w*w*(4.0 + g*g);
		a2 = w*w*w*g*(12.0+g*g);	
		a3 = pow(w,4)*(16.0+24.0*g*g+pow(g,4));
		a4 = pow(w,5)*g*(80.0+40.0*g*g+pow(g,4));
		a5 = pow(w,6)*(64.0+240.0*g*g+60.0*pow(g,4)+pow(g,6));

		a[ix][0] = (384.0*a0+64.0*a1-120.0*a2-20.0*a3+6.0*a4+a5)/23040.0;  
		a[ix][1] = -(576.0*a0+144.0*a1-160.0*a2-40.0*a3+4.0*a4+a5)/3840.0;
		a[ix][2] = (1152.0*a0+576.0*a1-104.0*a2-52.0*a3+2.0*a4+a5)/1536.0;
		a[ix][3] = -(784.0*a1-56.0*a3+a5)/1152.0;
		a[ix][4] = (-1152.0*a0+576.0*a1+104.0*a2-52.0*a3-2.0*a4+a5)/1536.0;
		a[ix][5] = -(-576.0*a0+144.0*a1+160.0*a2-40.0*a3-4.0*a4+a5)/3840.0;
		a[ix][6] = (-384.0*a0+64.0*a1+120.0*a2-20.0*a3-6.0*a4+a5)/23040.0;  

		/* initial conditions */
		cur[ix] = 0.;
		nxt[ix] = 0.;
	    }

	    free(v);
	    free(vx);


	    /* propagation in time */
	    for (it=0; it < nt; it++) {

		for (ix=0; ix < nbx; ix++) {
		    old[ix] = cur[ix];
		    cur[ix] = nxt[ix];
		}  

		/* Stencil */
		nxt[0] = 0.0;
		nxt[1] = cur[0]*a[1][6] +cur[0]*a[1][5] +cur[0]*a[1][4] +cur[1]*a[1][3] +cur[2]*a[1][2] 
		    + cur[3]*a[1][1] + cur[4]*a[1][0];
		nxt[2] = cur[0]*a[1][6] +cur[0]*a[1][5] +cur[1]*a[1][4] +cur[2]*a[1][3] +cur[3]*a[1][2] 
		    + cur[4]*a[1][1] + cur[5]*a[1][0];
		for (ix=3; ix < nbx-3; ix++) {
		    nxt[ix] = cur[ix+3]*a[ix][0] +cur[ix+2]*a[ix][1] + cur[ix+1]*a[ix][2] + cur[ix]*a[ix][3]
			+ cur[ix-1]*a[ix][4] + cur[ix-2]*a[ix][5]+ cur[ix-3]*a[ix][6];

		}
		nxt[nbx-3] = cur[nbx-6]*a[nbx-3][6] + cur[nbx-5]*a[nbx-3][5] + cur[nbx-4]*a[nbx-3][4] + cur[nbx-3]*a[nbx-3][3]
	            + cur[nbx-2]*a[nbx-3][2]+ cur[nbx-1]*a[nbx-3][1] + cur[nbx-1]*a[nbx-3][0];
		nxt[nbx-2] = cur[nbx-5]*a[nbx-2][6] + cur[nbx-4]*a[nbx-2][5] + cur[nbx-3]*a[nbx-2][4] + cur[nbx-2]*a[nbx-2][3]
	            + cur[nbx-1]*a[nbx-2][2] + cur[nbx-1]*a[nbx-2][1] + cur[nbx-1]*a[nbx-2][0];
		nxt[nbx-1] = cur[nbx-4]*a[nbx-1][6] + cur[nbx-3]*a[nbx-1][5] + cur[nbx-2]*a[nbx-1][4] + cur[nbx-1]*a[nbx-1][3]
	            + cur[nbx-1]*a[nbx-1][2] + cur[nbx-1]*a[nbx-1][1] + cur[nbx-1]*a[nbx-1][0];
		for (ix=0; ix < nbx; ix++) {
		    nxt[ix] += 2*cur[ix] - old[ix];
		    if (ix < nb) nxt[ix] *= sin(ix*pi/nb/2); 
		}  

		if (it < 500) nxt[nb] = sig[it]; 
		sf_floatwrite(nxt+nb,nx,out);
	    } 



    }         


    exit(0); 
}           
           
