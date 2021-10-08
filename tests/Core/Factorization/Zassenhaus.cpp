#include <assert.h>

#include "Core/Algebra/List.hpp"
#include "Core/Algebra/Algebra.hpp"
#include "Core/Polynomial/Polynomial.hpp"
#include "Core/Polynomial/Factorization.hpp"
#include "Core/Factorization/Zassenhaus.hpp"
#include "Core/Factorization/Berlekamp.hpp"

using namespace ast;
using namespace algebra;
using namespace polynomial;
using namespace factorization;

void should_factorize_zassenhaus()
{
	AST* f = add({
		power(symbol("x"), integer(4)),
		integer(-1)
	});
	
	AST* g = add({
		mul({integer(6), power(symbol("x"), integer(4))}),
		mul({integer(5), power(symbol("x"), integer(3))}),
		mul({integer(15), power(symbol("x"), integer(2))}),
		mul({integer(5), symbol("x")}),
		integer(4)
	});
	
	AST* L = list({symbol("x")});

	printf("%s\n", berlekampFactors(g, L->operand(0), integer(6473))->toString().c_str());

	AST* K = symbol("Z");

	// AST* z = zassenhaus(f, L, K);
	AST* r = zassenhaus(g, L, K);

	// printf("%s\n", z->toString().c_str());
	printf("%s\n", r->toString().c_str());

	delete f;
	delete L;
	delete K;
	// delete z;
	delete r;
}

void should_distinct_degree_factorize()
{
	AST* f = add({
		power(symbol("x"), integer(8)),
		power(symbol("x"), integer(7)),
		mul({integer(-1), power(symbol("x"), integer(6))}),
		power(symbol("x"), integer(5)),
		mul({integer(-1), power(symbol("x"), integer(3))}),
		mul({integer(-1), power(symbol("x"), integer(2))}),
		mul({integer(-1), symbol("x") }),
	});

	AST* q = integer(3);
	AST* L = list({symbol("x")});
	AST* K = symbol("Z");

	AST* d = distinctDegreeFactorization(f, L, K, q);

	printf("----> %s\n", d->toString().c_str());

	// delete f;
	// delete q;
	// delete L;
	// delete K;
	// delete d;
}

int main()
{
	should_distinct_degree_factorize();
	// should_factorize_zassenhaus();
	return 0;	
}
