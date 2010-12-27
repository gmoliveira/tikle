#include <stdio.h>

#define HOSTS 3
#define EVENTOS 2

int vetor[HOSTS*EVENTOS][EVENTOS*4+3];

int verifica(int foo) {

	int i,j;
	vetor[foo][0] = 1;
	for (i=0;i<HOSTS*EVENTOS;i++) {
		if (vetor[i][0] == 0) {
			printf("verificando %c e %c com linha %d\n", vetor[foo][1], vetor[foo][2], i);
			if ((vetor[i][1] == vetor[foo][2]) && (vetor[i][2] == vetor[foo][1])) {
				printf("verdadeiro\n");
				vetor[i][0] = 1; 
				for (j=4;j<EVENTOS*4+3;j+=4) {
					vetor[foo][j-1] = 1;
					vetor[i][j-1] = 1;
					printf("EVENTO %d: analisando linha %d (in/out: %d/%d) com linha %d (in/out: %d/%d)\n",
							vetor[i][j], foo, vetor[foo][j+1], vetor[foo][j+2], i, vetor[i][j+1], vetor[i][j+2]);
				}
				return 1;
			}
		}
	}
	return 0;
}
/*
 * ##############################################################################
 * 	falta verificar se estavam na mesma partição durante o evento X	
 * ##############################################################################
 */
int main(void)
{

	int i = 0, j = 0, foo, k, l;

	vetor[0][0] = 0;
	vetor[0][1] = 65;
	vetor[0][2] = 66;
	vetor[0][3] = 0;
	vetor[0][4] = 0;
	vetor[0][5] = 20;
	vetor[0][6] = 13;
	vetor[0][7] = 0;
	vetor[0][8] = 1;
	vetor[0][9] = 3;
	vetor[0][10] = 8;

	vetor[1][0] = 0;
	vetor[1][1] = 65;
	vetor[1][2] = 67;
	vetor[1][3] = 0;
	vetor[1][4] = 0;
	vetor[1][5] = 10;
	vetor[1][6] = 15;
	vetor[1][7] = 0;
	vetor[1][8] = 1;
	vetor[1][9] = 5;
	vetor[1][10] = 7;

	vetor[2][0] = 0;
	vetor[2][1] = 66;
	vetor[2][2] = 65;
	vetor[2][3] = 0;
	vetor[2][4] = 0;
	vetor[2][5] = 13;
	vetor[2][6] = 20;
	vetor[2][7] = 0;
	vetor[2][8] = 1;
	vetor[2][9] = 8;
	vetor[2][10] = 3;

	vetor[3][0] = 0;
	vetor[3][1] = 66;
	vetor[3][2] = 67;
	vetor[3][3] = 0;
	vetor[3][4] = 0;
	vetor[3][5] = 12;
	vetor[3][6] = 15;
	vetor[3][7] = 0;
	vetor[3][8] = 1;
	vetor[3][9] = 10;
	vetor[3][10] = 7;

	vetor[4][0] = 0;
	vetor[4][1] = 67;
	vetor[4][2] = 65;
	vetor[4][3] = 0;
	vetor[4][4] = 0;
	vetor[4][5] = 15;
	vetor[4][6] = 10;
	vetor[4][7] = 0;
	vetor[4][8] = 1;
	vetor[4][9] = 7;
	vetor[4][10] = 5;

	vetor[5][0] = 0;
	vetor[5][1] = 67;
	vetor[5][2] = 66;
	vetor[5][3] = 0;
	vetor[5][4] = 0;
	vetor[5][5] = 15;
	vetor[5][6] = 12;
	vetor[5][7] = 0;
	vetor[5][8] = 1;
	vetor[5][9] = 7;
	vetor[5][10] = 10;
	
	for (i=0;i<HOSTS*EVENTOS;i++) {
		for (j=0;j<EVENTOS*4+3;j++) {
			if (j == 1 || j == 2)
				printf("- %c ",vetor[i][j]);
			else
				printf("- %d ",vetor[i][j]);
		}
		printf("\n");
	}

	for (i=0;i<HOSTS*EVENTOS;i++) {
		if (vetor[i][0] == 0) {
			printf("enviando %c e %c\n\n", vetor[i][1], vetor[i][2]);
			verifica(i);
			for (k=0;k<HOSTS*EVENTOS;k++) {
				for (l=0;l<EVENTOS*4+3;l++) {
					if (l == 1 || l == 2)
						printf("- %c ",vetor[k][l]);
					else
						printf("- %d ",vetor[k][l]);
				}
				printf("\n");
			}

		}
	}
	return 1;
}
