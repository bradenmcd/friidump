#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#define mm  8           /* RS code over GF(2**mm) - change to suit */
#define n   256   	    /* n = size of the field */
#define nn  182         /* nn=2**mm -1   length of codeword */
#define kk  172         /* kk = nn-2*tt  */ /* Degree of g(x) = 2*tt */

//#define	NN		n-1
//#define	FCR		0
//#define	PRIM	1
#define	_NROOTS	nn-kk
//#define	PAD		NN-nn
//#define	A0		NN
//#define	IPRIM	1

const int	NN     = n-1;
const int	FCR    = 0;
const int	PRIM   = 1;
const int	NROOTS = nn-kk;
const int	PAD    = (n-1)-nn;
const int	A0     = n-1;
const int	IPRIM  = 1;


#ifndef min
#define	min(a,b)	((a) < (b) ? (a) : (b))
#endif

/**** Primitive polynomial ****/
int pp [mm+1] = { 1, 0, 1, 1, 1, 0, 0, 0, 1}; /* 1+x^2+x^3+x^4+x^8 */

/* generator polynomial, tables for Galois field */
int alpha_to[n], index_of[n], gg[nn-kk+1];

int b0 = 1;

/* data[] is the info vector, bb[] is the parity vector, recd[] is the 
  noise corrupted received vector  */
int recd[nn], data[kk], bb[nn-kk];

int modnn(int x){
  while (x >= 0xff) {
    x -= 0xff;
    x = (x >> 0xff) + (x & 0xff);
  }
  return x;
}


void generate_gf()
 {
	register int i, mask ;

  mask = 1 ;
  alpha_to[mm] = 0 ;
  for (i=0; i<mm; i++)
   { alpha_to[i] = mask ;
     index_of[alpha_to[i]] = i ;
     if (pp[i]!=0) /* If pp[i] == 1 then, term @^i occurs in poly-repr of @^mm */
       alpha_to[mm] ^= mask ;  /* Bit-wise EXOR operation */
     mask <<= 1 ; /* single left-shift */
   }
  index_of[alpha_to[mm]] = mm ;
  /* Have obtained poly-repr of @^mm. Poly-repr of @^(i+1) is given by 
     poly-repr of @^i shifted left one-bit and accounting for any @^mm 
     term that may occur when poly-repr of @^i is shifted. */
  mask >>= 1 ;
  for (i=mm+1; i<255; i++)
   { if (alpha_to[i-1] >= mask)
        alpha_to[i] = alpha_to[mm] ^ ((alpha_to[i-1]^mask)<<1) ;
     else alpha_to[i] = alpha_to[i-1]<<1 ;
     index_of[alpha_to[i]] = i ;
   }
  index_of[0] = A0 ;//-1
 }


void gen_poly()
/* Obtain the generator polynomial of the tt-error correcting, length */
 {
	register int i, j, root;

	gg[0] = 1;

	for (i = 0,root=0*1; i < nn-kk; i++,root += 1) {
		gg[i+1] = 1;

		for (j = i; j > 0; j--){
			if (gg[j] != 0)
				gg[j] = gg[j-1] ^ alpha_to[modnn(index_of[gg[j]] + root)];
			else
				gg[j] = gg[j-1];
		}

		gg[0] = alpha_to[modnn(index_of[gg[0]] + root)];
	}
	for (i=0; i <= nn-kk; i++) {
		gg[i] = index_of[gg[i]];
	}
 }


void rs_encode(unsigned char *data, unsigned char *bb)
 {
	register int i,j ;
	int feedback;

	for (i=0; i<NROOTS; i++)   bb[i] = 0; //nullify result

	for(i=0;i<NN-NROOTS-PAD;i++){
		feedback = index_of[data[i] ^ bb[0]];

		if(feedback != A0){      /* feedback term is non-zero */
			for(j=1;j<NROOTS;j++) {
				bb[j] ^= alpha_to[modnn(feedback + gg[NROOTS-j])];
			}
		}
		/* Shift */
		memmove(&bb[0],&bb[1], NROOTS-1);
		//for (j=0; j<NROOTS-1; j++)   bb[j] = bb[j+1];

		if(feedback != A0)
			bb[NROOTS-1] = alpha_to[modnn(feedback + gg[0])];
		else
			bb[NROOTS-1] = 0;
	}
 }
///*
int rs_decode(unsigned char *data, int *eras_pos, int no_eras){
  int deg_lambda, el, deg_omega;
  int i, j, r,k;
  unsigned char u,q,tmp,num1,num2,den,discr_r;
  unsigned char lambda[_NROOTS+1], s[_NROOTS];	
  unsigned char b[_NROOTS+1], t[_NROOTS+1], omega[_NROOTS+1];
  unsigned char root[_NROOTS], reg[_NROOTS+1], loc[_NROOTS];
  int syn_error, count;


  // form the syndromes; i.e., evaluate data(x) at roots of g(x)
  for(i=0;i<NROOTS;i++)
    s[i] = data[0];

  for(j=1;j<NN-PAD;j++){
    for(i=0;i<NROOTS;i++){
      if(s[i] == 0){
	s[i] = data[j];
      } else {
	s[i] = data[j] ^ alpha_to[modnn(index_of[s[i]] + (FCR+i)*PRIM)];
      }
    }
  }

  // Convert syndromes to index form, checking for nonzero condition
  syn_error = 0;
  for(i=0;i<NROOTS;i++){
    syn_error |= s[i];
    s[i] = index_of[s[i]];
  }

  if (!syn_error) {
    // if syndrome is zero, data[] is a codeword and there are no
    // errors to correct. So return data[] unmodified
    //
    count = 0;
    goto finish;
  }
  memset(&lambda[1],0,NROOTS*sizeof(lambda[0]));
  lambda[0] = 1;

  if (no_eras > 0) {
    /* Init lambda to be the erasure locator polynomial */
    lambda[1] = alpha_to[modnn(PRIM*(NN-1-eras_pos[0]))];
    for (i = 1; i < no_eras; i++) {
      u = modnn(PRIM*(NN-1-eras_pos[i]));
      for (j = i+1; j > 0; j--) {
	tmp = index_of[lambda[j - 1]];
	if(tmp != A0)
	  lambda[j] ^= alpha_to[modnn(u + tmp)];
      }
    }
  }
  for(i=0;i<NROOTS+1;i++)
    b[i] = index_of[lambda[i]];

  /*
   * Begin Berlekamp-Massey algorithm to determine error+erasure
   * locator polynomial
   */
  r = no_eras;
  el = no_eras;

  while (++r <= NROOTS) {	/* r is the step number */
    /* Compute discrepancy at the r-th step in poly-form */
    discr_r = 0;
    for (i = 0; i < r; i++){
      if ((lambda[i] != 0) && (s[r-i-1] != A0)) {
	discr_r ^= alpha_to[modnn(index_of[lambda[i]] + s[r-i-1])];
      }
    }
    discr_r = index_of[discr_r];	/* Index form */
    if (discr_r == A0) {
      /* 2 lines below: B(x) <-- x*B(x) */
      memmove(&b[1],b,NROOTS*sizeof(b[0]));
      b[0] = A0;
    } else {
      /* 7 lines below: T(x) <-- lambda(x) - discr_r*x*b(x) */
      t[0] = lambda[0];
      for (i = 0 ; i < NROOTS; i++) {
	if(b[i] != A0)
	  t[i+1] = lambda[i+1] ^ alpha_to[modnn(discr_r + b[i])];
	else
	  t[i+1] = lambda[i+1];
      }
      if (2 * el <= r + no_eras - 1) {
	el = r + no_eras - el;
	/*
	 * 2 lines below: B(x) <-- inv(discr_r) *
	 * lambda(x)
	 */
	for (i = 0; i <= NROOTS; i++)
	  b[i] = (lambda[i] == 0) ? A0 : modnn(index_of[lambda[i]] - discr_r + NN);
      } else {
	/* 2 lines below: B(x) <-- x*B(x) */
	memmove(&b[1],b,NROOTS*sizeof(b[0]));
	b[0] = A0;
      }
      memcpy(lambda,t,(NROOTS+1)*sizeof(t[0]));
    }
  }

  /* Convert lambda to index form and compute deg(lambda(x)) */
  deg_lambda = 0;
  for(i=0;i<NROOTS+1;i++){
    lambda[i] = index_of[lambda[i]];
    if(lambda[i] != A0)
      deg_lambda = i;
  }
  /* Find roots of the error+erasure locator polynomial by Chien search */
  memcpy(&reg[1],&lambda[1],NROOTS*sizeof(reg[0]));
  count = 0;		/* Number of roots of lambda(x) */
  for (i = 1,k=IPRIM-1; i <= NN; i++,k = modnn(k+IPRIM)) {
    q = 1; /* lambda[0] is always 0 */
    for (j = deg_lambda; j > 0; j--){
      if (reg[j] != A0) {
	reg[j] = modnn(reg[j] + j);
	q ^= alpha_to[reg[j]];
      }
    }
    if (q != 0)
      continue; /* Not a root */
    /* store root (index-form) and error location number */
    root[count] = i;
    loc[count] = k;
    /* If we've already found max possible roots,
     * abort the search to save time
     */
    if(++count == deg_lambda)
      break;
  }

  if (deg_lambda != count) {
    /*
     * deg(lambda) unequal to number of roots => uncorrectable
     * error detected
     */
    count = -1;
    goto finish;
  }
  /*
   * Compute err+eras evaluator poly omega(x) = s(x)*lambda(x) (modulo
   * x**NROOTS). in index form. Also find deg(omega).
   */
  deg_omega = deg_lambda-1;
  for (i = 0; i <= deg_omega;i++){
    tmp = 0;
    for(j=i;j >= 0; j--){
      if ((s[i - j] != A0) && (lambda[j] != A0))
	tmp ^= alpha_to[modnn(s[i - j] + lambda[j])];
    }
    omega[i] = index_of[tmp];
  }

  /*
   * Compute error values in poly-form. num1 = omega(inv(X(l))), num2 =
   * inv(X(l))**(FCR-1) and den = lambda_pr(inv(X(l))) all in poly-form
   */
  for (j = count-1; j >=0; j--) {
    num1 = 0;
    for (i = deg_omega; i >= 0; i--) {
      if (omega[i] != A0)
	num1  ^= alpha_to[modnn(omega[i] + i * root[j])];
    }
    num2 = alpha_to[modnn(root[j] * (FCR - 1) + NN)];
    den = 0;

    /* lambda[i+1] for i even is the formal derivative lambda_pr of lambda[i] */
    for (i = min(deg_lambda,NROOTS-1) & ~1; i >= 0; i -=2) {
      if(lambda[i+1] != A0)
	den ^= alpha_to[modnn(lambda[i+1] + i * root[j])];
    }
    /* Apply error to data */
    if (num1 != 0 && loc[j] >= PAD) {
      data[loc[j]-PAD] ^= alpha_to[modnn(index_of[num1] + index_of[num2] + NN - index_of[den])];
    }
  }

 finish:
  if(eras_pos != NULL){
    for(i=0;i<count;i++)
      eras_pos[i] = loc[i];
  }
  return count;
}
