#include <cstdlib>
#include <cmath>
#include <iostream>
#include <string>
#include "trm_subs.h"

// These are declared extern in pzextr

double **d, *x;

/**
 * bsstep carries out a Burlisch-Stoer step with monitoring of 
 * local truncation error, based upon Numerical Recipes routine.
 * To use this one must express the problem as a serives of first
 * order ordinary differential equations, usually by defining extra
 * variables. 
 * \param y    nv y values
 * \param dydx nv derivatives
 * \param nv   number of equations
 * \param xx   value of x
 * \param htry ??
 * \param eps  ??
 * \param yscal scaling factors
 * \param hdid  ??
 * \param hnext ??
 * \param derivs computes derivatives
 */

void Subs::bsstep(double y[], double dydx[], int nv, double &xx, 
		  double htry, double eps, double yscal[], 
		  double &hdid, double &hnext, 
		  void (*derivs)(double, double [], double [])){

    const double SAFE1  = 0.25;
    const double SAFE2  = 0.7;
    const double REDMAX = 1.0e-5;
    const double REDMIN = 0.7;
    const double TINY   = 1.0e-30;
    const double SCALMX = 0.1;

    int i, iq, k, kk, km;
    static int first=1, kmax, kopt;
    static double epsold = -1.0, xnew;
    double eps1, errmax, fact, h, red = 0, scale = 0, work, wrkmin, xest;
    double yerr[nv], ysav[nv], yseq[nv];

    const int KMAXX = 8;
    const int IMAXX = KMAXX+1;
    double err[KMAXX]; 
    static double a[IMAXX];
    static double alf[KMAXX][KMAXX];
    static int nseq[IMAXX]={2,4,6,8,10,12,14,16,18};
    int reduct, exitflag=0;

    // Grab memory
    d = new double*[nv]; 
    for(i=0;i<nv;i++)
	d[i] = new double[KMAXX]; 
    x = new double[KMAXX];

    if(eps != epsold){
	hnext = xnew = -1.0e29;
	eps1  = SAFE1*eps;
	a[0]  = nseq[0] + 1;
	for(k=0;k<KMAXX;k++) a[k+1] = a[k]+nseq[k+1];
	for(iq=1;iq<KMAXX;iq++){
	    for(k=0;k<iq;k++)
		alf[k][iq]=pow(eps1,(a[k+1]-a[iq+1])/
			       ((a[iq+1]-a[0]+1.0)*(2*k+3)));
	}
	epsold = eps;
	for(kopt=1;kopt<KMAXX-1;kopt++)
	    if(a[kopt+1] > a[kopt]*alf[kopt-1][kopt]) break;
	kmax=kopt;
    }
    h=htry;
    for(i=0;i<nv;i++) ysav[i]=y[i];
    if(xx != xnew || h != hnext){
	first = 1;
	kopt  = kmax;
    }
    reduct = 0;
    for(;;){
	for(k=0;k<=kmax;k++){
	    xnew = xx+h;
	    if(xnew == xx){
		delete[] x;
		for(i=0;i<nv;i++)
		    delete[] d[i];
		delete[] d;
		throw Subs::Subs_Error("bsstep error: step size underflow");
	    }
      
	    mmid(ysav,dydx,nv,xx,h,nseq[k],yseq,derivs);
	    xest = Subs::sqr(h/nseq[k]);
	    pzextr(k,xest,yseq,y,yerr,nv);
	    if(k != 0){
		errmax = TINY;
		for(i=0;i<nv;i++) errmax = std::max(errmax, fabs(yerr[i]/yscal[i]));
		errmax /= eps;
		km = k-1;
		err[km] = pow(errmax/SAFE1,1.0/(2*km+3));
	    }
	    if(k != 0 && (k >= kopt-1 || first)){
	
		if(errmax < 1.0){
		    exitflag=1;
		    break;
		}
		if(k == kmax || k == kopt+1){
		    red=SAFE2/err[km];
		    break;
		}else if(k == kopt && alf[kopt-1][kopt] < err[km]){
		    red = 1.0/err[km];
		    break;
		}else if(kopt == kmax && alf[km][kmax-1] < err[km]){
		    red = alf[km][kmax-1]*SAFE2/err[km];
		    break;
		}else if(alf[km][kopt] < err[km]){
		    red = alf[km][kopt-1]/err[km];
		    break;
		}
	    }
	}
	if(exitflag) break;
	red = std::min(red,REDMIN);
	red = std::max(red,REDMAX);
	h *= red;
	reduct=1;
    }
    xx   = xnew;
    hdid = h;
    first = 0;
    wrkmin= 1.0e35;
    for(kk=0;kk<=km;kk++){
	fact = std::max(err[kk], SCALMX);
	work = fact*a[kk+1];
	if(work < wrkmin){
	    scale = fact;
	    wrkmin = work;
	    kopt=kk+1;
	}
    }
    hnext = h/scale;
    if(kopt >= k && kopt != kmax && !reduct){
	fact = std::max(scale/alf[kopt-1][kopt],SCALMX);
	if(a[kopt+1]*fact <= wrkmin){
	    hnext = h/fact;
	    kopt++;
	}
    }
    delete[] x;
    for(i=0;i<nv;i++)
	delete[] d[i];
    delete[] d;
}









