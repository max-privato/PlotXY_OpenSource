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

#include "SuppFunctions.h"
#define max(a, b)  (((a) > (b)) ? (a) : (b))
#define min(a, b)  (((a) < (b)) ? (a) : (b))

/*
Questo file contiene funzioni di supporto a PlotXY:
1) funzioni di allocazione e disallocazione delle matrici
2) funzione per il tracciamento di numeri entro stringhe con la corretta considerazione del numero di cifre significative.*/

int **CreateIMatrix(long numRows, long numCols){
    long i;
    int  **Matrix;
    //Allocaz. vettore puntatori alle righe:
    Matrix=new int *[numRows];
    if(Matrix==nullptr)
        return nullptr;
  //Allocaz. matrice:
    Matrix[0]=new int[numRows*numCols];
    for(i=1; i<numRows; i++)
        Matrix[i]=Matrix[0]+i*numCols;
    return Matrix;
}

void DeleteIMatrix(int  **Matrix){
    if(Matrix==nullptr) return;
    delete[] Matrix[0];
    delete[] Matrix;
}

double **CreateDMatrix(int numOfRows, int numOfCols){
/* Con questa funzione si alloca un unico spazio contiguo alla matrice, ma le righe ricevono dei puntatori specifici.
Quindi si deve allocare anche spazio per i puntatori alle righe.*/
    int i;
    double **matrix;
    //Allocaz. vettore puntatori alle righe:
    matrix=new double*[numOfRows];
    if(matrix==nullptr)
      return nullptr;
    if(matrix==nullptr)
      return nullptr;
    //Allocaz. matrice:
    matrix[0]=new double[numOfRows*numOfCols];
    if(matrix[0]==nullptr)
      return nullptr;
    for(i=1; i<numOfRows; i++)
        matrix[i]=matrix[0]+i*numOfCols;
    return matrix;
}


float **CreateFMatrix(int numOfRows, int numOfCols){
/* Con questa funzione si alloca un unico spazio contiguo alla matrice, ma le righe ricevono dei puntatori specifici.
Quindi si deve allocare anche spazio per i puntatori alle righe.*/
    int i;
    float **matrix;
    //Allocaz. vettore puntatori alle righe:
    matrix=new float*[numOfRows];
    if(matrix==nullptr)
      return nullptr;
    if(matrix==nullptr)
      return nullptr;
    //Allocaz. matrice:
    matrix[0]=new float[numOfRows*numOfCols];
    if(matrix[0]==nullptr)
      return nullptr;
    for(i=1; i<numOfRows; i++)
        matrix[i]=matrix[0]+i*numOfCols;
    return matrix;
}

int DeleteCMatrix(float **matrix){
    if(matrix==nullptr)
      return 1;
    delete[] matrix[0];
    delete[] matrix;
    return 0;
}


int DeleteFMatrix(float **matrix){
    if(matrix==nullptr)
      return 1;
    delete[] matrix[0];
    delete[] matrix;
    return 0;
}



float* **CreateFPMatrix(int NumRows, int NumCols){
    //Versione per matrici di puntatori a float
    int i;
    float* **Matrix;
    //Allocaz. vettore puntatori alle righe:
    Matrix=new float* *[NumRows];
    if(Matrix==nullptr)
        return nullptr;
  //Allocaz. matrice:
    Matrix[0]=new float*[NumRows*NumCols];
    if(Matrix[0]==nullptr)
        return nullptr;
    for(i=1; i<NumRows; i++)
        Matrix[i]=Matrix[0]+i*NumCols;
    return Matrix;
}


int DeleteFPMatrix(float* **Matrix){
    //Versione per matrici di puntatori a float
    if(Matrix==nullptr)
        return 1;
    delete[] Matrix[0];
    delete[] Matrix;
    return 0;
}


char **CreateCMatrix(long NumRows, long NumCols){
    long i;
    char **Matrix;
    //Allocaz. vettore puntatori alle righe:
    Matrix=new char*[NumRows];
    if(Matrix==nullptr)
        return nullptr;
  //Allocaz. matrice:
    Matrix[0]=new char[NumRows*NumCols];
    for(i=1; i<NumRows; i++)
        Matrix[i]=Matrix[0]+i*NumCols;
    return Matrix;
}

void DeleteCMatrix(char **Matrix){
    if(Matrix==nullptr)
        return;
    delete[] Matrix[0];
    delete[] Matrix;
}


QString smartSetNum(float num, int prec){
    /*  Funzione PURA che scrive su stringa i numeri con un numero prefissato di cifre
     * significative nella versione più compatta possibile, ma senza perdita di informazioni.
     *  E' stato necessario implementarla
     * ache se esiste la funzione di stringa setNum, in quanto quest'ultima taglia gli zeri
     * finali. Pertanto, ad es. 0.1200  (o 0.12) con 4 cifre significative in setNum viene
     * 0.12, con perdita di informazione sugli ultimi due digit, mentre nella presente
     * routine, più correttamente, viene 0.1200
     *
    */
    QChar sep;
    QString out, prepS;
    //Prima di tutto scrivo il numero nella stringa da ritornare in formato esponenziale, e successivamente verifico se esiste la possibilità di una scrittura più compatta che metto nuovamente in out. Se non esisterà questa possibilità, la stringa di uscita sarà out così come derivante dalla seguente riga:
    out.setNum(num,'e',prec-1);
    //Prima di tutto devo capire quante cifre ho dopo l'esponente, se 1 o 2 I due casi, testati rispettivamente con Qt 5.2 (Mac-air) e Qt 5.7 (Fisso Home) sono e+## (qualunque sia l'esponente e formato differenziato a seconda se sia sufficiente un esponente a un dicit (nel qual caso ho E+#) o servano due digit nel qual caso ho E+##). In sostanza mi basta calcolare expSize e vedere sse è di 3 caratteri (E+#) o di 4 (E+##)
    int expSize=out.size()-out.indexOf('e');
    //Se l'esponente è 0 intanto mi riparmio le ultime 4 cifre:
    int exp;
    exp=out.right(expSize-1).toInt();
    //Se l'esponente è zero lo tolgo e via:
    if(exp==0){
      out.chop(expSize);
      return out;
    }

    //nelle seguenti analisi devo maneggiare il separatore decimale sep, che può essere punto o virgola. Faccio un codice che sia robusto in tal senso:
    if(out.contains('.'))
        sep='.';
    else
        sep=',';
    // la seguente prepS è la stringa da "prependere" se il numero è in modulo minore di 1 ovviamente nel caso in cui sia possibile rinunciare alla notazione esponenziale
    prepS="0"+QString(sep);
    //Il seguente loop è valido solo per esponenti positivi, e compatibili con il livello di precisione richiesto. Se il numero è troppo grande per essere rappresentato in utte le sue cifre in formato floating point non verrà processato e resterà attiva la notazione esponenziale precedentemente usata per la scrittura in out
    for (int i=1; i<prec; i++)
      if(exp==i &&prec>i){
        out.remove(sep);
        out.chop(expSize);
        //Adesso inserisco il puntino nella posizione giusta. Però poi lo ritolgo se diviene l'ultimo carattere.
        if(out[0]=='-')
          out.insert(i+1+1,sep);
        else
          out.insert(i+1,sep);
        if(out[out.length()-1]=='.')
            out.chop(1);
        break;
        if(i+1!=out.length())
          out.insert(i+1+(int)(out[0]=='-'),sep);
        break;
       }
// Rimangono da trattare solo i casi di esponente negativo. Vengono considerati solo esponenti fino a -2 in quanto al più voglio avere due zeri dopo la virgola. Oltre questo è più leggibile la notazione esponenziale:
    if(exp==-1 &&prec>1){
      out.remove(sep);
      if(out[0]=='-')
         out.insert(1,prepS);
      else
         out.insert(0,prepS);
      out.chop(expSize);
     }
    if(exp==-2 &&prec>2){
      if(out[0]=='-'){
        out.insert(1,"0");
        out.remove(sep);
        out.insert(1,prepS);
      }else{
        out.insert(0,"0");
        out.remove(sep);
        out.insert(0,prepS);
      }
      out.chop(expSize);
     }
    if(exp==-3 &&prec>3){
      if(out[0]=='-'){
        out.insert(1,"00");
        out.remove(sep);
        out.insert(1,prepS);
      }else{
        out.insert(0,"00");
        out.remove(sep);
        out.insert(0,prepS);
      }
      out.chop(expSize);
     }
    //Il seguente ulteriore caso comporta numeri in floatin point particolarmente lunghi, più lunghi del caso di forma esponenziale ed esponente a 3 digit. Pertanto ha senso mantenerlo solo se l'esponente è a 4 digit:
    if(expSize==4)
      if(exp==-4 &&prec>4){
        if(out[0]=='-'){
          out.insert(1,"000");
          out.remove(sep);
          out.insert(1,prepS);
        }else{
          out.insert(0,"000");
          out.remove(sep);
          out.insert(0,prepS);
        }
        out.chop(expSize);
       }

    return out;
}
