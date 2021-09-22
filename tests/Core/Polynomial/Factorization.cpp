#include "Core/Algebra/Set.hpp"	
#include "Core/Algebra/List.hpp"	
#include "Core/Algebra/Algebra.hpp"	
#include "Core/Polynomial/Polynomial.hpp"
#include "Core/Polynomial/Factorization.hpp"

#include <assert.h>

using namespace ast;
using namespace algebra;
using namespace polynomial;

void should_get_r_matrix() {

	AST* u = add({
		power(symbol("x"), integer(6)),
		power(symbol("x"), integer(5)),
		symbol("x"),
		integer(4)
	});

	AST* x = symbol("x");

	AST* n = integer(6);

	RMatrix(u, x, n, 5);

	// 0 0 1 0 1 0
	// 0 4 3 3 1 1
	// 0 0 1 1 2 0
	// 0 0 3 4 2 0
	// 0 0 2 4 1 0
	// 0 1 3 3 1 4

	assert(getRMatrixValue(0,0) == 0);
	assert(getRMatrixValue(0,1) == 0);
	assert(getRMatrixValue(0,2) == 1);
	assert(getRMatrixValue(0,3) == 0);
	assert(getRMatrixValue(0,4) == 1);
	assert(getRMatrixValue(0,5) == 0);

	assert(getRMatrixValue(1,0) == 0);
	assert(getRMatrixValue(1,1) == 4);
	assert(getRMatrixValue(1,2) == 3);
	assert(getRMatrixValue(1,3) == 3);
	assert(getRMatrixValue(1,4) == 1);
	assert(getRMatrixValue(1,5) == 1);

	assert(getRMatrixValue(2,0) == 0);
	assert(getRMatrixValue(2,1) == 0);
	assert(getRMatrixValue(2,2) == 1);
	assert(getRMatrixValue(2,3) == 1);
	assert(getRMatrixValue(2,4) == 2);
	assert(getRMatrixValue(2,5) == 0);

	assert(getRMatrixValue(3,0) == 0);
	assert(getRMatrixValue(3,1) == 0);
	assert(getRMatrixValue(3,2) == 3);
	assert(getRMatrixValue(3,3) == 4);
	assert(getRMatrixValue(3,4) == 2);
	assert(getRMatrixValue(3,5) == 0);

	assert(getRMatrixValue(4,0) == 0);
	assert(getRMatrixValue(4,1) == 0);
	assert(getRMatrixValue(4,2) == 2);
	assert(getRMatrixValue(4,3) == 4);
	assert(getRMatrixValue(4,4) == 1);
	assert(getRMatrixValue(4,5) == 0);

	assert(getRMatrixValue(5,0) == 0);
	assert(getRMatrixValue(5,1) == 1);
	assert(getRMatrixValue(5,2) == 3);
	assert(getRMatrixValue(5,3) == 3);
	assert(getRMatrixValue(5,4) == 1);
	assert(getRMatrixValue(5,5) == 4);

	destroyRMatrix(6);

	delete u;
	delete x;
	delete n;

}

void should_get_berlekamp_factors() {
	AST* u = add({
		power(symbol("x"), integer(6)),
		power(symbol("x"), integer(5)),
		symbol("x"),
		integer(4)
	});

	AST* x = symbol("x");
	AST* factors = berlekampFactor(u, x, 5);

	AST* F = set({
		add({
			integer(3), 
			mul({
				integer(2),
				symbol("x"),
			}),
			power(
				symbol("x"),
				integer(2)
			)
		}),
		add({
			integer(4), 
			mul({
				integer(3),
				symbol("x"),
			}),
			power(
				symbol("x"),
				integer(2)
			)
		}),
		add({
			integer(2), 
			symbol("x"),
			power(
				symbol("x"),
				integer(2)
			)
		}),
	});

	assert(factors->match(F));

	delete u;
	delete x;
	delete F;
	delete factors;
}

void should_gen_extended_sigma_p() {
	AST* F0 = add({
		symbol("x"),
		integer(1)
	});

	AST* V0 = list({
		symbol("x"),
		sub({ symbol("x"), integer(1) }),
	});

	AST* x = symbol("x");

	AST* S0 = genExtendSigmaP(V0, x, 3);

	AST* k0 = list({
		integer(-1),
		integer(1),
	});

	AST* R0 = genExtendRP(V0, S0, F0, x, 3);

	AST* t0 = list({
		integer(-1),
		integer(-1),
	});

	assert(S0->match(k0));
	assert(R0->match(t0));

	AST* F1 = add({
		mul({integer(-4), power(symbol("x"), integer(4))}),
		power(symbol("x"), integer(3)),
		mul({integer(-4), power(symbol("x"), integer(2))}),
		mul({integer(-3), symbol("x")}),
		integer(4)
	});

	AST* V1 = list({
		add({
			power(symbol("x"), integer(2)),
			mul({integer(-2), symbol("x")}),
			integer(4)
		}),

		add({
			symbol("x"),
			integer(5)
		}),

		add({
			symbol("x"),
			integer(-5)
		}),

		add({
			symbol("x"),
			integer(-2)
		})
	});

	AST* S1 = genExtendSigmaP(V1, x, 11);
	AST* R1 = genExtendRP(V1, S1, F1, x, 11);
	
	AST* t1 = list({
		add({
			integer(4),
			mul({ integer(-2), symbol("x") })
		}),
		integer(0),
		integer(0),
		integer(-2),
	});

	AST* k1 = list({
		add({
			integer(2),
			mul({ integer(3), symbol("x") }),
			mul({ integer(-5), power(symbol("x"), integer(2)) }),
			mul({ integer(-4), power(symbol("x"), integer(3)) }),
			mul({ integer(-2), power(symbol("x"), integer(4)) }),
			mul({ integer(-2), power(symbol("x"), integer(5)) }),
			mul({ integer(-2), power(symbol("x"), integer(6)) }),
		}),
		add({
			integer(5),
			mul({ integer(-2), symbol("x") }),
			mul({ integer(-1), power(symbol("x"), integer(2)) }),
			mul({ integer(4), power(symbol("x"), integer(3)) }),
			mul({ integer(5), power(symbol("x"), integer(4)) }),
			mul({ integer(2), power(symbol("x"), integer(5)) }),
		}),
		add({
			integer(3),
			mul({ integer(-1), symbol("x") }),
			mul({ integer(-1), power(symbol("x"), integer(3)) }),
		}),
		integer(3),
	});

	assert(S1->match(k1));
	assert(R1->match(t1));
	
	delete x;
	delete F0;
	delete F1;
	delete V0;
	delete V1;
	delete k0;
	delete k1;
	delete t0;
	delete t1;
	delete S0;
	delete S1;
	delete R0;
	delete R1;
}

void should_get_square_free_factorization()
{
	AST* ax = add({
		power(symbol("x"), integer(8)),
		mul({
			integer(-2),
			power(symbol("x"), integer(6))
		}),
		mul({
			integer(2),
			power(symbol("x"), integer(2))
		}),
		integer(-1)
	});

	AST* x = symbol("x");

	AST* sf_ax0 = squareFreeFactorization(ax, x);

	AST* sf_ax_r = mul({
		power(
			add({
				integer(-1),
				power(symbol("x"), integer(2))
			}),
			integer(3)
		),
		add({
			integer(1),
			power(symbol("x"), integer(2))
		})
	});

	assert(sf_ax0->match(sf_ax_r));

	AST* sf_ax1 = squareFreeFactorization2(ax, x);

	assert(sf_ax1->match(sf_ax_r));

	AST* bx = add({
		power(symbol("x"), integer(11)),
		mul({integer(2), power(symbol("x"), integer(9))}),
		mul({integer(2), power(symbol("x"), integer(8))}),
		power(symbol("x"), integer(6)),
		power(symbol("x"), integer(5)),
		mul({integer(2), power(symbol("x"), integer(3))}),
		mul({integer(2), power(symbol("x"), integer(2))}),
		integer(1)
	});

	AST* q = power(integer(3), integer(1));

	AST* sf_bx0 = squareFreeFactorizationFiniteField(bx, x, q);
	AST* sf_bx_r = mul({
		add({symbol("x"), integer(1)}),
		power(add({power(symbol("x"), integer(2)), integer(1)}), integer(3)),
		power(add({symbol("x"), integer(2)}), integer(4)),
	}); 

	assert(sf_bx0->match(sf_bx_r));

	delete x;
	delete q;
	delete ax;
	delete bx;
	delete sf_bx0;
	delete sf_ax0;
	delete sf_ax1;
	delete sf_ax_r;
	delete sf_bx_r;
}

void should_formQMatrices()
{
	AST* ax = add({
		power(
			symbol("x"),
			integer(6)
		),
		mul({
			integer(-3),
			power(
				symbol("x"),
				integer(5)
			)
		}),
		power(
			symbol("x"),
			integer(4)
		),
		mul({
			integer(-3),
			power(
				symbol("x"),
				integer(3)
			)
		}),
		mul({
			integer(-1),
			power(
				symbol("x"),
				integer(2)
			)
		}),
		mul({
			integer(-3),
			symbol("x"),
		}),
		integer(1)
	});

	AST* x = symbol("x");
	AST* q = integer(11);

	AST* Q = formMatrixQ(ax, x, q);

	assert(Q->operand(0)->operand(0)->kind() == Kind::Integer);
	assert(Q->operand(0)->operand(0)->value() == 1);
	assert(Q->operand(0)->operand(1)->kind() == Kind::Integer);
	assert(Q->operand(0)->operand(1)->value() == 0);
	assert(Q->operand(0)->operand(2)->kind() == Kind::Integer);
	assert(Q->operand(0)->operand(2)->value() == 0);
	assert(Q->operand(0)->operand(3)->kind() == Kind::Integer);
	assert(Q->operand(0)->operand(3)->value() == 0);
	assert(Q->operand(0)->operand(4)->kind() == Kind::Integer);
	assert(Q->operand(0)->operand(4)->value() == 0);
	assert(Q->operand(0)->operand(5)->kind() == Kind::Integer);
	assert(Q->operand(0)->operand(5)->value() == 0);
	
	assert(Q->operand(1)->operand(0)->kind() == Kind::Integer);
	assert(Q->operand(1)->operand(0)->value() == 3);
	assert(Q->operand(1)->operand(1)->kind() == Kind::Integer);
	assert(Q->operand(1)->operand(1)->value() == 5);
	assert(Q->operand(1)->operand(2)->kind() == Kind::Integer);
	assert(Q->operand(1)->operand(2)->value() == -3);
	assert(Q->operand(1)->operand(3)->kind() == Kind::Integer);
	assert(Q->operand(1)->operand(3)->value() == -3);
	assert(Q->operand(1)->operand(4)->kind() == Kind::Integer);
	assert(Q->operand(1)->operand(4)->value() == -5);
	assert(Q->operand(1)->operand(5)->kind() == Kind::Integer);
	assert(Q->operand(1)->operand(5)->value() == 5);
	
	assert(Q->operand(2)->operand(0)->kind() == Kind::Integer);
	assert(Q->operand(2)->operand(0)->value() == 3);
	assert(Q->operand(2)->operand(1)->kind() == Kind::Integer);
	assert(Q->operand(2)->operand(1)->value() == -5);
	assert(Q->operand(2)->operand(2)->kind() == Kind::Integer);
	assert(Q->operand(2)->operand(2)->value() == -5);
	assert(Q->operand(2)->operand(3)->kind() == Kind::Integer);
	assert(Q->operand(2)->operand(3)->value() == 1);
	assert(Q->operand(2)->operand(4)->kind() == Kind::Integer);
	assert(Q->operand(2)->operand(4)->value() == -1);
	assert(Q->operand(2)->operand(5)->kind() == Kind::Integer);
	assert(Q->operand(2)->operand(5)->value() == 0);
	
	assert(Q->operand(3)->operand(0)->kind() == Kind::Integer);
	assert(Q->operand(3)->operand(0)->value() == -2);
	assert(Q->operand(3)->operand(1)->kind() == Kind::Integer);
	assert(Q->operand(3)->operand(1)->value() == 4);
	assert(Q->operand(3)->operand(2)->kind() == Kind::Integer);
	assert(Q->operand(3)->operand(2)->value() == -1);
	assert(Q->operand(3)->operand(3)->kind() == Kind::Integer);
	assert(Q->operand(3)->operand(3)->value() == 3);
	assert(Q->operand(3)->operand(4)->kind() == Kind::Integer);
	assert(Q->operand(3)->operand(4)->value() == -4);
	assert(Q->operand(3)->operand(5)->kind() == Kind::Integer);
	assert(Q->operand(3)->operand(5)->value() == -2);

	assert(Q->operand(4)->operand(0)->kind() == Kind::Integer);
	assert(Q->operand(4)->operand(0)->value() == -4);
	assert(Q->operand(4)->operand(1)->kind() == Kind::Integer);
	assert(Q->operand(4)->operand(1)->value() == -3);
	assert(Q->operand(4)->operand(2)->kind() == Kind::Integer);
	assert(Q->operand(4)->operand(2)->value() == -1);
	assert(Q->operand(4)->operand(3)->kind() == Kind::Integer);
	assert(Q->operand(4)->operand(3)->value() == 0);
	assert(Q->operand(4)->operand(4)->kind() == Kind::Integer);
	assert(Q->operand(4)->operand(4)->value() == 0);
	assert(Q->operand(4)->operand(5)->kind() == Kind::Integer);
	assert(Q->operand(4)->operand(5)->value() == -3);
	
	assert(Q->operand(5)->operand(0)->kind() == Kind::Integer);
	assert(Q->operand(5)->operand(0)->value() == -3);
	assert(Q->operand(5)->operand(1)->kind() == Kind::Integer);
	assert(Q->operand(5)->operand(1)->value() == -1);
	assert(Q->operand(5)->operand(2)->kind() == Kind::Integer);
	assert(Q->operand(5)->operand(2)->value() == -4);
	assert(Q->operand(5)->operand(3)->kind() == Kind::Integer);
	assert(Q->operand(5)->operand(3)->value() == -3);
	assert(Q->operand(5)->operand(4)->kind() == Kind::Integer);
	assert(Q->operand(5)->operand(4)->value() == -1);
	assert(Q->operand(5)->operand(5)->kind() == Kind::Integer);
	assert(Q->operand(5)->operand(5)->value() == -3);
	
	printf("%s\n", Q->toString().c_str());
	
	AST* T = formMatrixQBinary(ax, x, q);
	
	printf("%s\n", T->toString().c_str());
	
	delete ax;
	delete x;
	delete q;
	delete Q;
}

int main() {
	
	should_get_r_matrix();
	should_get_berlekamp_factors();
	should_gen_extended_sigma_p();
	should_get_square_free_factorization();
	should_formQMatrices();

	return 0;
}
