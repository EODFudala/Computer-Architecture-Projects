#include <stdio.h>
#include <stdlib.h>





////Register Blocking
//void dgemm(int m, int n, float *A, float *C) {
//    for (int j = 0; j < m; j++) {
//        for (int k = 0; k < n; k++) {
//
//            float tmp = A[j + k * m];
//
//            for (int i = 0; i < m; i++) {
//                C[i + j * m] += A[i + k * m] * tmp;
//            }
//        }
//    }
//}



////Blocking
//void dgemm(int m, int n, float *A, float *C) {
//
//    int size = m;
//
//
//    for(int i = 0; i < m; i += size)
//        for(int j = 0; j < m; j += size)
//            for(int k = 0; k < m; k += size)
//                for(int x = 0; x < size; x++)
//                    for(int y = 0; y < n; y++)
//                        for(int z = 0; z < size; z++)
//                            C[(i + z) + (j + x) * m] += A[(i + z) + (k + y) * m] * A[(j + x) + (k + y) * m];
//}

//
//
////Loop Unrolling
void dgemm( int m, int n, float *A, float *C ) {
//    int j, ExtraIt;
//    if(m % 2 == 0){
//        ExtraIt = m;
//    }
//    else {
//        ExtraIt = m-1;
//    }
//    for(int i = 0; i < m; i++){
//        for(int k = 0; k < n; k++){
//            for(j = 0; j < ExtraIt; j += 4){
//                C[i + j * m] += A[i + k * m] * A[j + k * m];
//                C[i + (j + 1) * m] += A[i + k * m] * A[(j + 1) + k * m];
//                C[i + (j + 2) * m] += A[i + k * m] * A[(j + 2) + k * m];
//                C[i + (j + 3) * m] += A[i + k * m] * A[(j + 3) + k * m];
//                }
//            if(ExtraIt % 2 != 0)
//                C[i + j * m] += A[i + k * m] * A[j + k * m];
//            }
//        }
//    }
    
    
//  int isEven;
//    int j;
//
//  if(m % 2 == 0) {
//    isEven = 1;
//  }
//
//  else if(m % 2 != 0) {
//    isEven = 0;
//  }
//
//  else {
//    printf("Value for m is not valid!");
//  }
//
//  for(int i = 0; i < m; i++) {
//    for(int k = 0; k < n; k++) {
//      if(isEven == 1) {
//        for(j = 0; j < m; j += 3) {
//          C[i + j * m] += A[i + k * m] * A[j + k * m];
//          C[i + (j + 1) * m] += A[i + k * m] * A[(j + 1) + k * m];
//            C[i + (j + 2) * m] += A[i + k * m] * A[(j + 2) + k * m];
//
//        }
//      } else {
//          C[i + j * m] += A[i + k * m] * A[j + k * m];
//        }
//      }
//    }
//  }


  int isEven;
    int x;

  if(m % 2 == 0) {
    isEven = 1;
  }

  else if(m % 2 != 0) {
    isEven = 0;
  }

  else {
    printf("Value for m is not valid!");
  }

  for(int i = 0; i < m; i++) {
    for(int k = 0; k < n; k++) {
        for(int j = 0; j < m - 2; j += 3) {
          C[i + j * m] += A[i + k * m] * A[j + k * m];
          C[i + (j + 1) * m] += A[i + k * m] * A[(j + 1) + k * m];
           C[i + (j + 2) * m] += A[i + k * m] * A[(j + 2) + k * m];
            x = j;
        }
        if (x < m) {
            for(int j = x; j < m; j++) {
            C[i + j * m] += A[i + k * m] * A[j + k * m];
      }

        }
    }
  }
}
