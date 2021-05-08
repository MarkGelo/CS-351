/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */

char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    // at most 12 local variables of type int on stack
    // cannot modify array A
    // cannot define any arrays

    if((M == 32 && N == 32) || (M == 61 && N == 67)){ // for 32x32 and 61x67
      /* 6 local vars
       * N Block Size: 32 x 32 | 64 x 64 | 61 x 67
       * 1 Block Size: 695 Misses | 4723 Misses | 4424
       * 4 Block Size: 439 Misses | 1795 Misses | 2424 Misses
       * 8 Block Size: 287 Misses | 4635 Misses | 2215 Misses 
       * 16 Block Size: 1141 Misses | 4651 Misses | 1989 Misses
       * */

      // with respect to A, row is N, col is M
      int i, j, row, col; //i j for blocks and row col similar ot how other lang does it
      int dgnV; // var for diagonal value
      int bs; //block size -- Best for 32x32, 64x64, 61x67 are 8, 4, 16 respectively
      if(M == 32 && N == 32){ // 287 misses -- good enough
        bs = 8;
      }else if(M == 64 && N == 64){ // 1795 misses -- bad
        bs = 4;
      }else if(M == 61 && N == 67){ // 1989 misses -- good enough
        bs = 16;
      }
      for(row = 0; row < N; row += bs){
        for(col = 0; col < M; col += bs){
          for(i = row; i < row + bs && i < N; i++){ // i < N for non square 61x67
            for(j = col; j < col + bs && j < M; j++){ // j < M for non square 6x67
              if(i == j){
                dgnV = A[j][j];
              }else{
                B[j][i] = A[i][j];
              }
            }
            if(row == col){
              B[i][i] = dgnV;
            }
          }
        }
      }
    }else if(M == 64 && N == 64){ // for 64x64 since too many misses using the above so optimized it
      /* with 8 local variables
       * Block size of 8 with 1387 misses -- meh
       *
       * with 12 local variables
       * Block size of 8 with 1275 misses -- good
       * */
      int row, col, i, j;
      int temp1, temp2, temp3, temp4;
      int temp5, temp6, temp7, temp8; // more vars for reducing misses
      for(row = 0; row < N; row += 8){
        for(col = 0; col < M; col += 8){
          for(i = 0; i < 4; i++){
            // temp vars to reduce misses
            temp1 = A[row + i][col];
            temp2 = A[row + i][col + 1]; 
            temp3 = A[row + i][col + 2]; 
            temp4 = A[row + i][col + 3]; 
            temp5 = A[row + i][col + 4]; 
            temp6 = A[row + i][col + 5]; 
            temp7 = A[row + i][col + 6]; 
            temp8 = A[row + i][col + 7];
          
            B[col][row + i] = temp1;
            B[col + 1][row + i] = temp2;
            B[col + 2][row + i] = temp3;
            B[col + 3][row + i] = temp4;

            B[col][row + i + 4] = temp5;
            B[col + 1][row + i + 4] = temp6;
            B[col + 2][row + i + 4] = temp7;
            B[col + 3][row + i + 4] = temp8;   
          }
          for(i = 0; i < 4; i++){
            temp1 = B[col + i][row + 4];
            temp2 = B[col + i][row + 5];
            temp3 = B[col + i][row + 6];
            temp4 = B[col + i][row + 7];

            B[col + i][row + 4] = A[row + 4][col + i];
            B[col + i][row + 5] = A[row + 5][col + i];
            B[col + i][row + 6] = A[row + 6][col + i];
            B[col + i][row + 7] = A[row + 7][col + i];

            B[col + 4 + i][row] = temp1;
            B[col + 4 + i][row + 1] = temp2;
            B[col + 4 + i][row + 2] = temp3;
            B[col + 4 + i][row + 3] = temp4;
          }
          for(i = 4; i < 8; i++){
            for(j = 4; j < 8; j++){
              B[col + j][row + i] = A[row + i][col + j];
            }
          }
        }
      }
    }
}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

char trans_all_desc[] = "trying all size transpose";
void trans_all(int M, int N, int A[N][M], int B[M][N])
{
/*
 * 6 local vars
 * N Block Size: 32 x 32 | 64 x 64 | 61 x 67
 * 1 Block Size: 695 Misses | 4723 Misses | 4424
 * 4 Block Size: 439 Misses | 1795 Misses | 2424 Misses
 * 8 Block Size: 287 Misses | 4635 Misses | 2215 Misses 
 * 16 Block Size: 1141 Misses | 4651 Misses | 1989 Misses
 * */

    // with respect to A, row is N, col is M
    int i, j, row, col; //i j for blocks and row col similar ot how other lang does it
    int dgnV; // var for diagonal value
    int bs; //block size -- Best for 32x32, 64x64, 61x67 are 8, 4, 16 respectively
    if(M == 32 && N == 32){ // 287 misses -- good enough
      bs = 8;
    }else if(M == 64 && N == 64){
      bs = 4;
    }else if(M == 61 && N == 67){ // 1989 misses -- good enough
      bs = 16;
    }
    for(row = 0; row < N; row += bs){
      for(col = 0; col < M; col += bs){
        for(i = row; i < row + bs && i < N; i++){ // i < N for non square 61x67
          for(j = col; j < col + bs && j < M; j++){ // j < M for non square 6x67
            if(i == j){
              dgnV = A[j][j];
            }else{
              B[j][i] = A[i][j];
            }
          }
          if(row == col){
            B[i][i] = dgnV;
          }
        }
      }
    }
    //printf("%d", is_transpose(M, N, A, B));
}

char trans_64x64_desc[] = "64 x 64 transpose";
void trans_64x64(int M, int N, int A[N][M], int B[M][N]){
    /* 
     * 8 local variables
     * Block size of 8 with 1387 misses -- meh
     *
     * 12 local variables
     * Block size of 8 with 1275 misses -- good
     * */
    int row, col, i, j;
    int temp1, temp2, temp3, temp4;
    int temp5, temp6, temp7, temp8; // more vars for reducing misses
    for(row = 0; row < N; row += 8){
      for(col = 0; col < M; col += 8){
        for(i = 0; i < 4; i++){
          // temp vars to reduce misses
          temp1 = A[row + i][col];
          temp2 = A[row + i][col + 1]; 
          temp3 = A[row + i][col + 2]; 
          temp4 = A[row + i][col + 3]; 
          temp5 = A[row + i][col + 4]; 
          temp6 = A[row + i][col + 5]; 
          temp7 = A[row + i][col + 6]; 
          temp8 = A[row + i][col + 7];
          
          B[col][row + i] = temp1;
          B[col + 1][row + i] = temp2;
          B[col + 2][row + i] = temp3;
          B[col + 3][row + i] = temp4;

          B[col][row + i + 4] = temp5;
          B[col + 1][row + i + 4] = temp6;
          B[col + 2][row + i + 4] = temp7;
          B[col + 3][row + i + 4] = temp8;   
        }
        for(i = 0; i < 4; i++){
          temp1 = B[col + i][row + 4];
          temp2 = B[col + i][row + 5];
          temp3 = B[col + i][row + 6];
          temp4 = B[col + i][row + 7];

          B[col + i][row + 4] = A[row + 4][col + i];
          B[col + i][row + 5] = A[row + 5][col + i];
          B[col + i][row + 6] = A[row + 6][col + i];
          B[col + i][row + 7] = A[row + 7][col + i];

          B[col + 4 + i][row] = temp1;
          B[col + 4 + i][row + 1] = temp2;
          B[col + 4 + i][row + 2] = temp3;
          B[col + 4 + i][row + 3] = temp4;
        }
        for(i = 4; i < 8; i++){
          for(j = 4; j < 8; j++){
            B[col + j][row + i] = A[row + i][col + j];
          }
        }
      }
    }
}

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc); 
    registerTransFunction(trans_all, trans_all_desc);
    registerTransFunction(trans_64x64, trans_64x64_desc);
}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

