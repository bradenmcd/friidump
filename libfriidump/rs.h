#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

int	modnn(int x);
void	generate_gf();
void	gen_poly();
void	rs_encode(unsigned char *data, unsigned char *bb);
int	rs_decode(unsigned char *data, int *eras_pos, int no_eras);
