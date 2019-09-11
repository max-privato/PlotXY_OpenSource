/*
 * This file is part of MC's PlotXY.
 *
 * PlotXY was created during 1998, continuously maintained and upgraded up to current year
 * by Massimo Ceraolo from the University of Pisa.
 *
 * The Linux distribution has been built using Ceraolo's source code in 2018 by Perry
 * Clements from Canada.
 *
 * This program is free software: you can redistribute it under the terms of GNU Public
 * License version 3 as published by the Free Software Foundation.
 *
 * PLOTXY AND ALL THE RELATED MATERIAL INCLUDED IN THE DISTRIBUTION PLOTXY.ZIP FILE OR
 * AVAILABLE FROM GITHUB IS SUPPLIED "AS-IS" THE AUTHOR OFFERS NO WARRANTY OF ITS FITNESS
 * FOR ANY PURPOSE WHATSOEVER, AND ACCEPTS NO LIABILITY WHATSOEVER FOR ANY LOSS OR
 * DAMAGE INCURRED BY ITS USE.
 *
 */


/*Header del file Matrix.cpp.
  Questa coppia di file serve per l'allocazione e deallocazione dinamica di un vettore di float.
    Qualora dovessero servire funzioni analoghe per gli int o altri tipi, l'implementazione
    sarebbe banale.
    Sembrerebbe logico fare un'unica implementazione sfruttanto i "Templates"
    del C++, ma al momento (Dic '97) non sono riuscito ad ottenere questo risultato.
*/
#include <complex>

#define _abs(a)  (((a) > (0)) ? (a) : (-a))

float **CreateFMatrix(int numOfRows, int numOfCols);
int DeleteFMatrix(float **matrix);
float **ReallocFMatrix(float **m, long nr, long nc, long old_nr);
int **CreateIMatrix(long numRows, long numCols);
void DeleteIMatrix(int **Matrix);

char **CreateCMatrix(long NumRows, long NumCols);
void DeleteCMatrix(char **Matrix);
