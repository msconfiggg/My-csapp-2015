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
void transpose_32x32(int M, int N, int A[N][M], int B[M][N]);
void transpose_64x64(int M, int N, int A[N][M], int B[M][N]);
void transpose_61x67(int M, int N, int A[N][M], int B[M][N]);

/*
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded.
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N]) {
  	if (M == 32 && N == 32) {
    	transpose_32x32(M, N, A, B);
  	} else if (M == 64 && N == 64) {
		transpose_64x64(M, N, A, B);
	} else if (M == 61 && N == 67) {
		transpose_61x67(M, N, A, B);
	}
}

/*
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started.
 */

/*
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N]) {
  	int i, j, tmp;

  	for (i = 0; i < N; i++) {
    	for (j = 0; j < M; j++) {
      	tmp = A[i][j];
      	B[j][i] = tmp;
    	}
  	}
}

char transpose_32x32_desc[] = "Transpose for 32x32 matrix";
void transpose_32x32(int M, int N, int A[N][M], int B[M][N]) {
  	int i, j, x, y;
	int a0, a1, a2, a3, a4, a5, a6, a7;

  	for (i = 0; i < N; i += 8) {
		for (j = 0; j < M; j += 8) {
			if (i == j) {
				a0 = A[i][j]; a1 = A[i][j + 1]; a2 = A[i][j + 2]; a3 = A[i][j + 3];
				a4 = A[i][j + 4]; a5 = A[i][j + 5]; a6 = A[i][j + 6]; a7 = A[i][j + 7];
				//将A的第i行暂存到B的第i行，这样就无需访问A的第i行，而B的第i行在第一次不命中后，后续的访问都是命中的
				B[i][j] = a0; B[i][j + 1] = a1; B[i][j + 2] = a2; B[i][j + 3] = a3;
				B[i][j + 4] = a4; B[i][j + 5] = a5; B[i][j + 6] = a6; B[i][j + 7] = a7;

				a0 = A[i + 1][j]; a1 = A[i + 1][j + 1]; a2 = A[i + 1][j + 2]; a3 = A[i + 1][j + 3];
				a4 = A[i + 1][j + 4]; a5 = A[i + 1][j + 5]; a6 = A[i + 1][j + 6]; a7 = A[i + 1][j + 7];
				B[i + 1][j] = B[i][j + 1];
				B[i][j + 1] = a0; B[i + 1][j + 1] = a1; B[i + 1][j + 2] = a2; B[i + 1][j + 3] = a3;
				B[i + 1][j + 4] = a4; B[i + 1][j + 5] = a5; B[i + 1][j + 6] = a6; B[i + 1][j + 7] = a7;

				a0 = A[i + 2][j]; a1 = A[i + 2][j + 1]; a2 = A[i + 2][j + 2]; a3 = A[i + 2][j + 3];
				a4 = A[i + 2][j + 4]; a5 = A[i + 2][j + 5]; a6 = A[i + 2][j + 6]; a7 = A[i + 2][j + 7];
				B[i + 2][j] = B[i][j + 2]; B[i + 2][j + 1] = B[i + 1][j + 2];
				B[i][j + 2] = a0; B[i + 1][j + 2] = a1; B[i + 2][j + 2] = a2; B[i + 2][j + 3] = a3;
				B[i + 2][j + 4] = a4; B[i + 2][j + 5] = a5; B[i + 2][j + 6] = a6; B[i + 2][j + 7] = a7;

				a0 = A[i + 3][j]; a1 = A[i + 3][j + 1]; a2 = A[i + 3][j + 2]; a3 = A[i + 3][j + 3];
				a4 = A[i + 3][j + 4]; a5 = A[i + 3][j + 5]; a6 = A[i + 3][j + 6]; a7 = A[i + 3][j + 7];
				B[i + 3][j] = B[i][j + 3]; B[i + 3][j + 1] = B[i + 1][j + 3]; B[i + 3][j + 2] = B[i + 2][j + 3];
				B[i][j + 3] = a0; B[i + 1][j + 3] = a1; B[i + 2][j + 3] = a2; B[i + 3][j + 3] = a3;
				B[i + 3][j + 4] = a4; B[i + 3][j + 5] = a5; B[i + 3][j + 6] = a6; B[i + 3][j + 7] = a7;

				a0 = A[i + 4][j]; a1 = A[i + 4][j + 1]; a2 = A[i + 4][j + 2]; a3 = A[i + 4][j + 3];
				a4 = A[i + 4][j + 4]; a5 = A[i + 4][j + 5]; a6 = A[i + 4][j + 6]; a7 = A[i + 4][j + 7];
				B[i + 4][j] = B[i][j + 4]; B[i + 4][j + 1] = B[i + 1][j + 4]; B[i + 4][j + 2] = B[i + 2][j + 4]; B[i + 4][j + 3] = B[i + 3][j + 4];
				B[i][j + 4] = a0; B[i + 1][j + 4] = a1; B[i + 2][j + 4] = a2; B[i + 3][j + 4] = a3;
				B[i + 4][j + 4] = a4; B[i + 4][j + 5] = a5; B[i + 4][j + 6] = a6; B[i + 4][j + 7] = a7;

				a0 = A[i + 5][j]; a1 = A[i + 5][j + 1]; a2 = A[i + 5][j + 2]; a3 = A[i + 5][j + 3];
				a4 = A[i + 5][j + 4]; a5 = A[i + 5][j + 5]; a6 = A[i + 5][j + 6]; a7 = A[i + 5][j + 7];
				B[i + 5][j] = B[i][j + 5]; B[i + 5][j + 1] = B[i + 1][j + 5];
				B[i + 5][j + 2] = B[i + 2][j + 5]; B[i + 5][j + 3] = B[i + 3][j + 5]; B[i + 5][j + 4] = B[i + 4][j + 5];
				B[i][j + 5] = a0; B[i + 1][j + 5] = a1; B[i + 2][j + 5] = a2; B[i + 3][j + 5] = a3;
				B[i + 4][j + 5] = a4; B[i + 5][j + 5] = a5; B[i + 5][j + 6] = a6; B[i + 5][j + 7] = a7;

				a0 = A[i + 6][j]; a1 = A[i + 6][j + 1]; a2 = A[i + 6][j + 2]; a3 = A[i + 6][j + 3];
				a4 = A[i + 6][j + 4]; a5 = A[i + 6][j + 5]; a6 = A[i + 6][j + 6]; a7 = A[i + 6][j + 7];
				B[i + 6][j] = B[i][j + 6]; B[i + 6][j + 1] = B[i + 1][j + 6]; B[i + 6][j + 2] = B[i + 2][j + 6];
				B[i + 6][j + 3] = B[i + 3][j + 6]; B[i + 6][j + 4] = B[i + 4][j + 6]; B[i + 6][j + 5] = B[i + 5][j + 6];
				B[i][j + 6] = a0; B[i + 1][j + 6] = a1; B[i + 2][j + 6] = a2; B[i + 3][j + 6] = a3;
				B[i + 4][j + 6] = a4; B[i + 5][j + 6] = a5; B[i + 6][j + 6] = a6; B[i + 6][j + 7] = a7;

				a0 = A[i + 7][j]; a1 = A[i + 7][j + 1]; a2 = A[i + 7][j + 2]; a3 = A[i + 7][j + 3];
				a4 = A[i + 7][j + 4]; a5 = A[i + 7][j + 5]; a6 = A[i + 7][j + 6]; a7 = A[i + 7][j + 7];
				B[i + 7][j] = B[i][j + 7]; B[i + 7][j + 1] = B[i + 1][j + 7]; B[i + 7][j + 2] = B[i + 2][j + 7];
				B[i + 7][j + 3] = B[i + 3][j + 7]; B[i + 7][j + 4] = B[i + 4][j + 7]; B[i + 7][j + 5] = B[i + 5][j + 7]; B[i + 7][i + 6] = B[i + 6][j + 7];
				B[i][j + 7] = a0; B[i + 1][j + 7] = a1; B[i + 2][j + 7] = a2; B[i + 3][j + 7] = a3;
				B[i + 4][j + 7] = a4; B[i + 5][j + 7] = a5; B[i + 6][j + 7] = a6; B[i + 7][j + 7] = a7;
			} else {
				for (x = i; x < i + 8; x++) {
					for (y = j; y < j + 8; y++) {
						B[y][x] = A[x][y];
					}
				}
			}
		}
  	}
}

char transpose_64x64_desc[] = "Transpose for 64x64 matrix";
void transpose_64x64(int M, int N, int A[N][M], int B[M][N]) {
	int i, j, x, y;
	int a0, a1, a2, a3, a4, a5, a6, a7;

	for (i = 0; i < N; i += 8) {
		j = (i == 0) ? 8 : 0;

		//把A对角线区块的下半区块存到B借用区块的上半区块，分左、右两块进行，移动的同时进行转置
		for (x = 0; x < 4; x++) {
			a0 = A[i + x + 4][i]; a1 = A[i + x + 4][i + 1]; a2 = A[i + x + 4][i + 2]; a3 = A[i + x + 4][i + 3];
			a4 = A[i + x + 4][i + 4]; a5 = A[i + x + 4][i + 5]; a6 = A[i + x + 4][i + 6]; a7 = A[i + x + 4][i + 7];

			B[i][j + x] = a0; B[i + 1][j + x] = a1; B[i + 2][j + x] = a2; B[i + 3][j + x] = a3;
			B[i][j + x + 4] = a4; B[i + 1][j + x + 4] = a5; B[i + 2][j + x + 4] = a6; B[i + 3][j + x + 4] = a7;
		}

		//把A对角线区块的上半区块直接移入B目标区块的上半区块
		for (x = i; x < i + 4; x++) {
			a0 = A[x][i]; a1 = A[x][i + 1]; a2 = A[x][i + 2]; a3 = A[x][i + 3];
			a4 = A[x][i + 4]; a5 = A[x][i + 5]; a6 = A[x][i + 6]; a7 = A[x][i + 7];

			B[x][i] = a0; B[x][i + 1] = a1; B[x][i + 2] = a2; B[x][i + 3] = a3;
			B[x][i + 4] = a4; B[x][i + 5] = a5; B[x][i + 6] = a6; B[x][i + 7] = a7;
		}

		//把B目标区块的上班区块分成左、右两个4x4区块分别转置
		for (x = i; x < i + 4; x++) {
			for (y = x + 1; y < i + 4; y++) {
				a0 = B[x][y];
				B[x][y] = B[y][x];
				B[y][x] = a0;

				a0 = B[x][y + 4];
				B[x][y + 4] = B[y][x + 4];
				B[y][x + 4] = a0;
			} 
		}

		//交换B目标区块上半区块的右半部分与B借用区块上半区块即原A对角线区块的下半区块的左半部分
		for (x = 0; x < 4; x++) {
			a0 = B[i + x][i + 4]; a1 = B[i + x][i + 5]; a2 = B[i + x][i + 6]; a3 = B[i + x][i + 7];
			B[i + x][i + 4] = B[i + x][j]; B[i + x][i + 5] = B[i + x][j + 1];
			B[i + x][i + 6] = B[i + x][j + 2]; B[i + x][i + 7] = B[i + x][j + 3];
			B[i + x][j] = a0; B[i + x][j + 1] = a1; B[i + x][j + 2] = a2; B[i + x][j + 3] = a3;
		}

		//把B借用区块的上半部分直接移入B目标区块的下半部分
		for (x = 0; x < 4; x++){
			B[i + x + 4][i] = B[i + x][j]; B[i + x + 4][i + 1] = B[i + x][j + 1];
			B[i + x + 4][i + 2] = B[i + x][j + 2]; B[i + x + 4][i + 3] = B[i + x][j + 3];
			B[i + x + 4][i + 4] = B[i + x][j + 4]; B[i + x + 4][i + 5] = B[i + x][j + 5];
			B[i + x + 4][i + 6] = B[i + x][j + 6]; B[i + x + 4][i + 7] = B[i + x][j + 7];
		}

		//处理非对角线区块
		for (j = 0; j < M; j += 8) {
			if (i == j) {
				continue;
			} else {
				//把A非对角线区块的上半区块移入B目标区块的上半区块，分成左、右两个部分，移动的同时进行转置
				for (x = j; x < j + 4; x++) {
					a0 = A[x][i]; a1 = A[x][i + 1]; a2 = A[x][i + 2]; a3 = A[x][i + 3];
					a4 = A[x][i + 4]; a5 = A[x][i + 5]; a6 = A[x][i + 6]; a7 = A[x][i + 7];

					B[i][x] = a0; B[i + 1][x] = a1; B[i + 2][x] = a2; B[i + 3][x] = a3;
					B[i][x + 4] = a4; B[i + 1][x + 4] = a5; B[i + 2][x + 4] = a6; B[i + 3][x + 4] = a7;
				}

				//把A非对角线区块的下半区块的左半区块转置覆盖B目标区块的上半区块的右半部分，并把原先的右半部分直接移入B目标区块的下半区块的左半部分
				for (y = i; y < i + 4; y++) {
					a0 = A[j + 4][y]; a1 = A[j + 5][y]; a2 = A[j + 6][y]; a3 = A[j + 7][y];
					a4 = B[y][j + 4]; a5 = B[y][j + 5]; a6 = B[y][j + 6]; a7 = B[y][j + 7];

					B[y][j + 4] = a0; B[y][j + 5] = a1; B[y][j + 6] = a2; B[y][j + 7] = a3;
					B[y + 4][j] = a4; B[y + 4][j + 1] = a5; B[y + 4][j + 2] = a6; B[y + 4][j + 3] = a7;
				}

				//把A非对角线区块的下半区块的右半部分转置移入B目标区块的下半区块的右半部分
				for (x = j + 4; x < j + 8; x++) {
					a0 = A[x][i + 4]; a1 = A[x][i + 5]; a2 = A[x][i + 6]; a3 = A[x][i + 7];
					B[i + 4][x] = a0; B[i + 5][x] = a1; B[i + 6][x] = a2; B[i + 7][x] = a3;
				}
			}
		}
	}
}

char transpose_61x67_desc[] = "Transpose for 61x67 matrix";
void transpose_61x67(int M, int N, int A[N][M], int B[M][N]) {
	int i, j, x, y;
	int a0, a1, a2, a3, a4, a5, a6, a7;

	for (i = 0; i < N; i += 8) {
		for (j = 0; j < M; j += 13) {
			if (i + 8 <= N && j + 13 <= M) {
				for (y = j; y < j + 13; y++) {
					a0 = A[i][y]; a1 = A[i + 1][y]; a2 = A[i + 2][y]; a3 = A[i + 3][y];
					a4 = A[i + 4][y]; a5 = A[i + 5][y]; a6 = A[i + 6][y]; a7 = A[i + 7][y];

					B[y][i] = a0; B[y][i + 1] = a1; B[y][i + 2] = a2; B[y][i + 3] = a3;
					B[y][i + 4] = a4; B[y][i + 5] = a5; B[y][i + 6] = a6; B[y][i + 7] = a7;
				}
			} else {
				for (x = i; x < N && x < i + 8; x++) {
					for (y = j; y < M && y < j + 13; y++) {
						B[y][x] = A[x][y];
					}
				}
			}
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
void registerFunctions() {
  	/* Register your solution function */
  	registerTransFunction(transpose_submit, transpose_submit_desc);

  	/* Register any additional transpose functions */
  	registerTransFunction(trans, trans_desc);
}

/*
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N]) {
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
