#include "hot.h"
#include "skyline.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

df_chaleur null_dfc = {0};

int main(void){

	df_chaleur dfc = null_dfc;
	
	int nx = 100;
	double Lx = 1;
	
	init_df_chaleur(&dfc, nx, Lx);
	
	double tmax = 0.1;
	int ns = 100;
	compute_serie(&dfc, tmax, ns);
	
	double cfl = 0.3;
	double theta = 0.5;
	compute_df(&dfc, tmax, cfl, theta);
	
	plot_data(&dfc);

	return 0;
	
}
// static : objet.attribut
// pointeur : objet->attribut ==== (*objet).attribut
void init_df_chaleur(df_chaleur *dfc, int nx, double Lx){

	dfc->nx = nx;
	dfc->Lx = Lx;
	
	dfc->dx = Lx / nx;
	
	dfc->xx = malloc(nx * sizeof(double));
	
	dfc->un = malloc(nx * sizeof(double));
	dfc->unp1 = malloc(nx * sizeof(double));
	
	dfc->us = malloc(nx * sizeof(double));
	
	for(int i = 0; i < nx; i++){
		dfc->xx[i] = dfc->dx / 2 + i * dfc->dx;
		init_data(dfc->xx[i], dfc->un + i);
		dfc->unp1[i] = dfc->un[i];
		
	}
}

void init_data(double x, double *u){

	if (fabs(x-0.5) < 1. / 8)
		*u = 1;
	else
		*u = 0;
}

void plot_data(df_chaleur *dfc){

	FILE *plotfile;
	
	plotfile = fopen("plot.dat", "w");
	
	for(int i = 0; i < dfc->nx; i++){
		fprintf(plotfile, "%f %f %f\n", dfc->xx[i], dfc->un[i], dfc->us[i]);
	}
	
	fclose(plotfile);
	
	system("gnuplot plotcom");
}


void compute_serie(df_chaleur *dfc, double tmax, int ns){

	double alpha[ns+1];
	
	double L = dfc->Lx;
	
	alpha[0] = 2./8;
	for(int j = 1; j <= ns; j++)
		alpha[j] = 2 * L / j / M_PI *
					(sin(j * M_PI / L * 5./8) - sin(j * M_PI / L * 3./8));
	
	for( int i=0; i < dfc->nx; i++)
		dfc->us[i] = 0;
	
	for(int k = 0; k <= ns; k++){
		double coef = exp(-k*k * M_PI*M_PI * tmax / L/L);
		for(int i=0; i < dfc->nx; i++){
			dfc->us[i] += alpha[k] * coef *	cos(k * M_PI / L * dfc->xx[i]);
		}
	}
}

void compute_df(df_chaleur *dfc, double tmax, double cfl, double theta){

	double dx = dfc->dx;
	double dt = cfl * dx * dx;
	
	int nx = dfc->nx;
	
	dfc->dt = dt;
	dfc->tmax = tmax;
	dfc->tnow = 0;
	
	Skyline M;
	Skyline N;
	
	InitSkyline(&M, nx);
	InitSkyline(&N, nx);
	
	// On indique les coefficients non nuls…
	for(int i = 0; i < nx-1; i++){
		SwitchOn(&M, i, i+1);
		SwitchOn(&M, i+1, i);
		SwitchOn(&N, i, i+1);
		SwitchOn(&N, i+1, i);
	}
	// … Pour pouvoir allouer l'espace en mémoire
	AllocateSkyline(&M);
	AllocateSkyline(&N);
	
	// On initialise les coefficients non nuls
	for(int i = 0; i < nx-1; i++){
		SetSkyline(&M,i,i,1 + theta*cfl*2);
		SetSkyline(&N,i,i,1 - (1-theta)*cfl*2);
		
		SetSkyline(&M,i,i+1,-theta*cfl);
		SetSkyline(&N,i,i+1,(1-theta)*cfl);
		
		SetSkyline(&M,i+1,i,-theta*cfl);
		SetSkyline(&N,i+1,i,(1-theta)*cfl);
	}
	SetSkyline(&M,0,0,1 + theta*cfl*1);
	SetSkyline(&N,0,0,1 - (1-theta)*cfl*1);
	
	SetSkyline(&M,nx-1,nx-1,1 + theta*cfl*1);
	SetSkyline(&N,nx-1,nx-1,1 - (1-theta)*cfl*1);
	
	// On calcule la factorisation LU de la matrice qu'on veut inverser
	FactoLU(&M);
	
	while(dfc->tnow < tmax){
		dfc->tnow += dt;
		
		// On calcule unp1 à partir de un
		
		double v[nx];// Vecteur temporaire qui va contenir N*un
		MatVectSkyline(&N, dfc->un, v);
		
		FastSolveSkyline(&M, v, dfc->unp1);
		
		// Mise à jour de un
		for(int i = 0; i < nx; i++)
			dfc->un[i] = dfc->unp1[i];
	
	}
}

