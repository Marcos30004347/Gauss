#include "AST3.hpp"
#include "Core/AST/Integer.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <math.h>
#include <random>
#include <string>

namespace ast_teste {

ast *ast_create(ast::kind kind, size_t reserve) {
  ast *a = (ast *)malloc(sizeof(ast));

  a->ref_count = 1;

  a->ast_kind = kind;

  a->ast_childs = (ast **)(calloc(sizeof(void (ast::*)()), reserve));

  a->ast_format = ast::default_format;

  a->ast_size = 0;

  a->ast_reserved_size = reserve;

  return a;
}

ast *ast_create(ast::kind kind) {
  ast *a = (ast *)malloc(sizeof(ast));

  a->ref_count = 1;

  a->ast_kind = kind;

  a->ast_childs =
      (ast **)(calloc(sizeof(void (ast::*)()), ast::childs_margin + 1));

  a->ast_format = ast::default_format;

  a->ast_size = 0;

  a->ast_reserved_size = ast::childs_margin + 1;

  return a;
}

ast *ast_create(ast::kind kind, std::initializer_list<ast *> l) {
  ast *a = (ast *)malloc(sizeof(ast));

  a->ref_count = 1;

  a->ast_kind = kind;

  a->ast_format = ast::default_format;

  a->ast_size = l.size();

  a->ast_reserved_size = a->ast_size;

  a->ast_childs = (ast **)(malloc(sizeof(void (ast::*)()) * l.size()));

  memcpy(a->ast_childs, l.begin(), (size_t)l.end() - (size_t)l.begin());

  return a;
}

void ast_set_kind(ast *a, ast::kind kind) { a->ast_kind = kind; }

ast *ast_set_operand(ast *a, ast *v, size_t i) {
  a->ast_childs[i] = v;
  return v;
}

void ast_set_size(ast *a, size_t s) {
  assert(a->ast_reserved_size >= s);
  a->ast_size = s;
}

void ast_delete_operands(ast *a) {
  if (a == 0)
    return;

  for (size_t i = 0; i < ast_size(a); i++) {
    ast_delete(ast_operand(a, i));
  }

  if (a->ast_childs) {
    free(a->ast_childs);
  }

  a->ast_size = 0;
  a->ast_reserved_size = 0;
  a->ast_childs = 0;
}

void ast_delete_metadata(ast *a) {
  if (ast_is_kind(a, ast::symbol)) {
    free(a->ast_sym);
  }

  if (ast_is_kind(a, ast::integer)) {
    delete a->ast_int;
  }
}

void ast_delete(ast *a) {
  if (a == 0)
    return;

  a = ast_dec_ref(a);

  if (a->ref_count > 0)
    return;

  ast_delete_operands(a);
  ast_delete_metadata(a);

  free(a);
}

ast *ast_symbol(const char *id) {
  ast *a = ast_create(ast::symbol);

  a->ast_sym = strdup(id);

  return a;
}

ast *ast_integer(Int value) {
  ast *a = ast_create(ast::integer);

  a->ast_int = new Int(value);

  return a;
}

ast *ast_fraction(Int num, Int den) {
  return ast_create(ast::fraction, {ast_integer(num), ast_integer(den)});
}

inline ast *ast_modifiable_ref(ast *a) {
  return a->ref_count > 1 ? ast_copy(a) : a;
}

inline void ast_assign_modifiable_ref(ast *a, ast **b) {
  ast *t = ast_modifiable_ref(a);

  if (t != a)
    ast_dec_ref(a);

  *b = t;
}

inline void ast_assign(ast** b, ast* a) {
	if(*b) ast_delete(*b);
	*b = a;
}

void ast_insert(ast *a, ast *b, size_t idx) {
  assert(a->ast_size >= idx);

  const size_t ast_size = sizeof(void (ast::*)());

  if (a->ast_reserved_size == a->ast_size) {

    ast **childs = (ast **)calloc(ast_size, (a->ast_size + ast::childs_margin));

    a->ast_reserved_size = a->ast_size + ast::childs_margin;

    memcpy(childs, a->ast_childs, ast_size * idx);

    childs[idx] = b;

    memcpy(childs + idx + 1, a->ast_childs + idx,
           ast_size * (a->ast_size - idx));

    free(a->ast_childs);

    a->ast_childs = childs;
  } else {
    if (idx == a->ast_size) {
      a->ast_childs[idx] = b;
    } else {
      memcpy(a->ast_childs + idx + 1, a->ast_childs + idx,
             ast_size * (a->ast_size - idx));
      a->ast_childs[idx] = b;
    }
  }

  a->ast_size += 1;
}

void ast_remove(ast *a, size_t idx) {
  if (ast_operand(a, idx)) {
    ast_delete(ast_operand(a, idx));
  }

  if (idx != a->ast_size - 1) {
    memcpy(a->ast_childs + idx, a->ast_childs + idx + 1,
           sizeof(void (ast::*)()) * (a->ast_size - idx - 1));
  }

  a->ast_size -= 1;

  if (a->ast_size - a->ast_reserved_size > ast::childs_margin) {
    ast **childs = (ast **)calloc(sizeof(void (ast::*)()),
                                  (a->ast_size + ast::childs_margin));

    memcpy(childs, a->ast_childs, sizeof(void (ast::*)()) * a->ast_size);

    free(a->ast_childs);

    a->ast_childs = childs;
  }
}

int ast_cmp_consts(ast *a, ast *b) {
  assert(ast_is_kind(a, ast::constant) && ast_is_kind(b, ast::constant));

  if (ast_is_kind(a, ast::integer) && ast_is_kind(b, ast::integer)) {
    if (ast_value(a) == ast_value(b)) {
      return 0;
    }

    return ast_value(a) > ast_value(b) ? 1 : -1;
  }

  if (ast_is_kind(a, ast::fraction) && ast_is_kind(b, ast::fraction)) {
    Int na = ast_value(a->ast_childs[0]);
    Int da = ast_value(a->ast_childs[1]);
    Int nb = ast_value(b->ast_childs[0]);
    Int db = ast_value(b->ast_childs[1]);

    if (na * db == nb * da)
      return 0;

    return na * db - nb * da > 0 ? 1 : -1;
  }

  if (ast_is_kind(a, ast::integer) && ast_is_kind(b, ast::fraction)) {
    Int na = ast_value(a);
    Int nb = ast_value(b->ast_childs[0]);
    Int db = ast_value(b->ast_childs[1]);

    Int ct = na * db;

    if (ct == nb) {
      return 0;
    }

    return ct > nb ? 1 : -1;
  }

  Int nb = ast_value(b);
  Int na = ast_value(a->ast_childs[0]);
  Int da = ast_value(a->ast_childs[1]);

  Int ct = nb * da;

  if (ct == na) {
    return 0;
  }

  return na > ct ? 1 : -1;
}

bool should_revert_idx(ast::kind ctx) { return ctx & (ast::add); }

bool ast_is_zero(ast *a) {
  return (a == nullptr) || (ast_is_kind(a, ast::integer) && ast_value(a) == 0);
}

inline int ast_op_cmp(ast *a, ast *b, ast::kind ctx) {
  long m = ast_size(a);
  long n = ast_size(b);

  long l = std::min(ast_size(a), ast_size(b));

  m = m - 1;
  n = n - 1;

  if (ast_is_kind(a, ast::constant) && ast_is_kind(b, ast::constant)) {
    return ast_cmp_consts(a, b);
  }

  if (ctx == ast::add) {

    if (ast_is_kind(a, ast::mul) && ast_is_kind(b, ast::mul)) {

      if (std::abs(m - n) > 1) {
        return n - m;
      }

      for (long i = 0; i < l; i++) {
        int order =
            ast_kind(ast_operand(a, m - i)) - ast_kind(ast_operand(b, n - i));

        if (order)
          return order;
      }

      for (long i = 0; i < l; i++) {
        int order = ast_cmp(ast_operand(a, m - i), ast_operand(b, n - i), ctx);

        if (order)
          return order;
      }
    }
  }

  for (long i = 0; i < l; i++) {
    int order =
        ast_kind(ast_operand(b, n - i)) - ast_kind(ast_operand(a, m - i));

    if (order)
      return order;
  }

  for (long i = 0; i < l; i++) {
    int order = ast_cmp(ast_operand(a, m - i), ast_operand(b, n - i), ctx);

    if (order)
      return order;
  }

  return (ctx & ast::add) ? m - n : n - m;
}

inline int ast_cmp_idents(ast *a, ast *b) {
  return strcmp(ast_id(a), ast_id(b));
}

std::string ast_to_string(ast *tree) {
  if (!tree)
    return "null";

  if (ast_is_kind(tree, ast::integer)) {
    return tree->ast_int->to_string();
  }

  if (ast_is_kind(tree, ast::symbol)) {
    return std::string(tree->ast_sym);
  }

  if (ast_is_kind(tree, ast::undefined)) {
    return "undefined";
  }

  if (ast_is_kind(tree, ast::fail)) {
    return "fail";
  }

  if (ast_is_kind(tree, ast::infinity)) {
    return "inf";
  }

  if (ast_is_kind(tree, ast::negative_infinity)) {
    return "-inf";
  }

  if (ast_is_kind(tree, ast::fraction)) {
    return ast_to_string(ast_operand(tree, 0)) + "/" +
           ast_to_string(ast_operand(tree, 1));
  }

  if (ast_is_kind(tree, ast::fraction)) {
    return "sqrt(" + ast_to_string(ast_operand(tree, 0)) + ")";
  }

  if (ast_is_kind(tree, ast::funcall)) {
    std::string r = std::string(ast_funname(tree)) + "(";

    if (ast_size(tree) > 0) {
      for (size_t i = 0; i < ast_size(tree) - 1; i++) {
        r += ast_to_string(ast_operand(tree, i));
        r += ",";
      }

      r += ast_to_string(ast_operand(tree, ast_size(tree) - 1));
    }

    r += ")";

    return r;
  }

  if (ast_is_kind(tree, ast::pow)) {
    std::string r = "";

    if (ast_operand(tree, 0) &&
        ast_is_kind(ast_operand(tree, 0),
                    ast::sub | ast::add | ast::mul | ast::div)) {
      r += "(";
    }

    r += ast_to_string(ast_operand(tree, 0));

    if (ast_operand(tree, 0) &&
        ast_is_kind(ast_operand(tree, 0),
                    ast::sub | ast::add | ast::mul | ast::div)) {
      r += ")";
    }

    r += "^";

    if (ast_operand(tree, 1) &&
        ast_is_kind(ast_operand(tree, 1),
                    ast::sub | ast::add | ast::mul | ast::div)) {
      r += "(";
    }

    r += ast_to_string(ast_operand(tree, 1));

    if (ast_operand(tree, 1) &&
        ast_is_kind(ast_operand(tree, 1),
                    ast::sub | ast::add | ast::mul | ast::div)) {
      r += ")";
    }

    return r;
  }

  if (ast_is_kind(tree, ast::div)) {
    std::string r = "";

    if (ast_is_kind(ast_operand(tree, 0),
                    ast::sub | ast::add | ast::mul | ast::div)) {
      r += "(";
    }

    r += ast_to_string(ast_operand(tree, 0));

    if (ast_is_kind(ast_operand(tree, 0),
                    ast::sub | ast::add | ast::mul | ast::div)) {
      r += ")";
    }

    r += " ÷ ";

    if (ast_is_kind(ast_operand(tree, 1),
                    ast::sub | ast::add | ast::mul | ast::div)) {
      r += "(";
    }

    r += ast_to_string(ast_operand(tree, 1));

    if (ast_is_kind(ast_operand(tree, 1),
                    ast::sub | ast::add | ast::mul | ast::div)) {
      r += ")";
    }

    return r;
  }

  if (ast_is_kind(tree, ast::add)) {
    std::string r = "";

    for (size_t i = 0; i < ast_size(tree); i++) {
      if (ast_operand(tree, i) &&
          ast_is_kind(ast_operand(tree, i), ast::sub | ast::add | ast::mul)) {

        r += "(";
      }

      r += ast_to_string(ast_operand(tree, i));

      if (ast_operand(tree, i) &&
          ast_is_kind(ast_operand(tree, i), ast::sub | ast::add | ast::mul)) {
        r += ")";
      }

      if (i < ast_size(tree) - 1) {
        r += " + ";
      }
    }

    return r;
  }

  if (ast_is_kind(tree, ast::sub)) {
    std::string r = "";

    for (size_t i = 0; i < ast_size(tree) - 1; i++) {
      if (ast_operand(tree, i) &&
          ast_is_kind(ast_operand(tree, i), ast::sub | ast::add | ast::mul)) {
        r += "(";
      }

      r += ast_to_string(ast_operand(tree, i));

      if (ast_operand(tree, i) &&
          ast_is_kind(ast_operand(tree, i), ast::sub | ast::add | ast::mul)) {
        r += ")";
      }

      if (i != ast_size(tree) - 1) {
        r += " - ";
      }
    }

    return r;
  }

  if (ast_is_kind(tree, ast::mul)) {
    std::string r = "";

    for (size_t i = 0; i < ast_size(tree); i++) {

      if (ast_operand(tree, i) == nullptr) {
        continue;
      }

      if (ast_is_kind(ast_operand(tree, i), ast::sub | ast::add | ast::mul)) {
        r += "(";
      }

      r += ast_to_string(ast_operand(tree, i));

      if (ast_is_kind(ast_operand(tree, i), ast::sub | ast::add | ast::mul)) {
        r += ")";
      }

      if (i < ast_size(tree) - 1) {
        r += "⋅";
      }
    }

    return r;
  }

  if (ast_is_kind(tree, ast::fact)) {
    return ast_to_string(ast_operand(tree, 0)) + "!";
  }

  return "to_string_not_implemented";
}

std::string ast_kind_id(ast *a) {
  switch (ast_kind(a)) {

  case ast::integer: {
    return "integer";
  }
  case ast::symbol: {
    return "symbol";
  }
  case ast::funcall: {
    return "funcall";
  }
  case ast::fact: {
    return "fact";
  }
  case ast::pow: {
    return "pow";
  }
  case ast::mul: {
    return "mul";
  }
  case ast::add: {
    return "add";
  }
  case ast::sub: {
    return "div";
  }
  case ast::sqrt: {
    return "sqrt";
  }
  case ast::infinity: {
    return "infinity";
  }
  case ast::negative_infinity: {
    return "negative infinity";
  }

  case ast::undefined: {
    return "undefined";
  }

  case ast::fail: {
    return "fail";
  }

  case ast::fraction: {
    return "fraction";
  }

  case ast::div: {
    return "div";
  }

  default:
    return "";
  }
}

void ast_print(ast *a, int tabs) {
  printf("%*c<ast ", tabs, ' ');
  printf("ref_count=\"%lli\" ", a->ref_count);
  printf("address=\"%p\" ", a);
  printf("kind=\"%s\"", ast_kind_id(a).c_str());

  if (ast_kind(a) == ast::integer) {
    printf(" value=\"%s\"", ast_value(a).to_string().c_str());
  }

  if (ast_kind(a) == ast::symbol) {
    printf(" id=\"%s\"", ast_id(a));
  }

  if (ast_size(a)) {
    printf(">\n");
    // printf("\n%*c  childs: [\n", tabs, ' ');

    for (size_t i = 0; i < ast_size(a); i++) {
      ast_print(ast_operand(a, i), tabs + 3);
    }
    // printf("%*c  ];", tabs, ' ');
    printf("%*c</ast>\n", tabs, ' ');
  } else {
    printf(">\n");
  }
}

int ast_cmp(ast *a, ast *b, ast::kind ctx) {
  if (a == b)
    return 0;

  if (ctx & ast::mul) {
    if (ast_is_kind(a, ast::constant)) {
      return -1;
    }

    if (ast_is_kind(b, ast::constant)) {
      return +1;
    }

    if (ast_is_kind(a, ast::pow) && ast_is_kind(b, ast::pow)) {
      return ast_cmp(ast_operand(a, 0), ast_operand(b, 0), ctx);
    }

    // if(ast_is_kind(a, ast::add) && ast_is_kind(b, ast::pow)) {
    // 	int order = ast_cmp(a, ast_operand(b, 0), ast::mul);

    // 	if(order == 0) {
    // 		return should_revert_idx(ctx)
    // 			? ast_kind(a) - ast_kind(b)
    // 			: ast_kind(b) - ast_kind(a);
    // 	}

    // 	return order;
    // }

    // if(ast_is_kind(b, ast::add) && ast_is_kind(a, ast::pow)) {
    // 	int order = ast_cmp(ast_operand(a, 0), b, ast::mul);

    // 	if(order == 0) {

    // 		return should_revert_idx(ctx)
    // 			? ast_kind(a) - ast_kind(b)
    // 			: ast_kind(b) - ast_kind(a);
    // 	}

    // 	return order;
    // 	// if(ast_cmp(ast_operand(a, 0), b, ast::mul) == 0) {
    // 	// 	return 0;
    // 	// return should_revert_idx(ctx)
    // 	// 	? ast_kind(a) - ast_kind(b)
    // 	// 	: ast_kind(b) - ast_kind(a);
    // 	// }
    // }

    if (ast_is_kind(a, ast::symbol | ast::add) && ast_is_kind(b, ast::pow)) {

      int order = ast_cmp(a, ast_operand(b, 0), ast::mul);

      if (order == 0) {
        return ast_kind(a) - ast_kind(b);
      }

      return order;
    }

    if (ast_is_kind(b, ast::symbol | ast::add) && ast_is_kind(a, ast::pow)) {
      int order = ast_cmp(ast_operand(a, 0), b, ast::mul);

      if (order == 0) {
        return ast_kind(a) - ast_kind(b);
      }

      return order;
    }

    if (ast_is_kind(a, ast::funcall) && ast_is_kind(b, ast::funcall)) {
      return strcmp(ast_funname(a), ast_funname(b));
    }

    if (ast_is_kind(a, ast::pow) && ast_is_kind(b, ast::funcall)) {
      return ast_cmp(ast_operand(a, 0), b, ast::mul);
    }

    if (ast_is_kind(b, ast::pow) && ast_is_kind(a, ast::funcall)) {
      return ast_cmp(b, ast_operand(a, 0), ast::mul);
    }

    if (ast_is_kind(a, ast::mul) &&
        ast_is_kind(b, ast::pow | ast::symbol | ast::funcall)) {
      long k = ast_size(a);

      for (long i = k - 1; i >= 0; i--) {
        int order = ast_cmp(ast_operand(a, i), b, ast::mul);

        if (order == 0) {
          return 0;
        }

        if (order < 0)
          break;
      }

      return +k - 1;
    }

    if (ast_is_kind(b, ast::mul) &&
        ast_is_kind(a, ast::pow | ast::symbol | ast::funcall)) {
      long k = ast_size(b);

      for (long i = k - 1; i >= 0; i--) {
        int order = ast_cmp(ast_operand(b, i), a, ast::mul);

        if (order == 0) {
          return 0;
        }

        if (order < 0)
          break;
      }

      return -k + 1;
    }
  }

  if (ctx & ast::add) {

    if (ast_is_kind(a, ast::constant) && ast_is_kind(b, ast::constant)) {
      return ast_cmp_consts(b, a);
    }

    if (ast_is_kind(a, ast::constant)) {
      return +1;
    }

    if (ast_is_kind(b, ast::constant)) {
      return -1;
    }

    if (ast_is_kind(a, ast::pow) && ast_is_kind(b, ast::pow)) {
      int i = ast_cmp(ast_operand(a, 1), ast_operand(b, 1), ctx);

      if (i != 0) {
        return i;
      }

      return ast_cmp(ast_operand(a, 0), ast_operand(b, 0), ctx);
    }

    if (ast_is_kind(a, ast::funcall) && ast_is_kind(b, ast::funcall)) {
      return strcmp(ast_funname(a), ast_funname(b));
    }

    if (ast_is_kind(a, ast::add) && ast_is_kind(b, ast::symbol)) {
      return +1;
    }

    if (ast_is_kind(a, ast::symbol) && ast_is_kind(b, ast::add)) {
      return -1;
    }

    if (ast_is_kind(a, ast::mul) && ast_is_kind(b, ast::symbol)) {
      long k = ast_size(a);

      if (k > 2)
        return -k;

      int order = ast_cmp(ast_operand(a, ast_size(a) - 1), b, ctx);

      if (order == 0) {
        return -1;
      }

      return k > 2 ? -1 : order;
    }

    if (ast_is_kind(b, ast::mul) && ast_is_kind(a, ast::symbol)) {
      long k = ast_size(b);

      if (k > 2) {
        return +k;
      }

      int order = ast_cmp(a, ast_operand(b, ast_size(b) - 1), ctx);

      if (order == 0) {
        return +1;
      }

      return k > 2 ? +1 : order;
    }

    if (ast_is_kind(a, ast::mul) && ast_is_kind(b, ast::pow)) {
      long k = ast_size(a);

      if (k > 2)
        return -k;

      int order = ast_cmp(ast_operand(a, 0), b, ctx);

      if (order == 0)
        return -1;

      return k > 2 ? -1 : order;
    }

    if (ast_is_kind(b, ast::mul) && ast_is_kind(a, ast::pow)) {
      long k = ast_size(b);

      if (k > 2)
        return +k;

      int order = ast_cmp(a, ast_operand(b, 0), ctx);

      if (order == 0)
        return +1;

      return k > 2 ? +1 : order;
    }
  }

  if (ast_is_kind(a, ast::funcall) && ast_is_kind(b, ast::funcall)) {
    return strcmp(ast_funname(a), ast_funname(b));
  }

  if (ast_is_kind(a, ast::constant) && ast_is_kind(b, ast::constant)) {
    return ast_cmp_consts(a, b);
  }

  if (ast_is_kind(a, ast::symbol) && ast_is_kind(b, ast::symbol)) {
    return ast_cmp_idents(a, b);
  }

  if (ast_is_kind(a, ast::add) && ast_is_kind(b, ast::add)) {
    return ast_op_cmp(a, b, ctx);
  }

  if (ast_is_kind(a, ast::mul) && ast_is_kind(b, ast::mul)) {
    return ast_op_cmp(a, b, ctx);
  }

  if (ast_is_kind(a, ast::pow) && ast_is_kind(b, ast::pow)) {
    return ast_cmp(ast_operand(a, 0), ast_operand(b, 0), ctx) ||
           ast_cmp(ast_operand(a, 1), ast_operand(b, 1), ctx);
  }

  if (ast_is_kind(a, ast::div) && ast_is_kind(b, ast::div)) {
    return ast_cmp(ast_operand(a, 0), ast_operand(b, 0), ctx) ||
           ast_cmp(ast_operand(a, 1), ast_operand(b, 1), ctx);
  }

  return should_revert_idx(ctx) ? ast_kind(a) - ast_kind(b)
                                : ast_kind(b) - ast_kind(a);
}

long int ast_sort_split(ast *a, long l, long r) {
  long int i = l - 1;

  ast *p = a->ast_childs[r];

  for (long int j = l; j < r; j++) {
    if (ast_cmp(a->ast_childs[j], p, ast_kind(a)) < 0) {
      i = i + 1;

      // swap i and j
      ast *t = a->ast_childs[i];

      a->ast_childs[i] = a->ast_childs[j];
      a->ast_childs[j] = t;
    }
  }

  ast *t = a->ast_childs[i + 1];

  a->ast_childs[i + 1] = a->ast_childs[r];
  a->ast_childs[r] = t;

  return i + 1;
}

ast *ast_sort_childs(ast *a, long int l, long int r) {
  ast_assign_modifiable_ref(a, &a);

  if (l < r) {
    long int m = ast_sort_split(a, l, r);

    a = ast_sort_childs(a, l, m - 1);
    a = ast_sort_childs(a, m + 1, r);
  }

  return a;
}

ast *ast_sort(ast *a) {
  ast_assign_modifiable_ref(a, &a);

  if (ast_is_kind(a, ast::terminal)) {
    return a;
  }

  size_t i = ast_is_kind(a, ast::sub) ? 1 : 0;

  for (; i < a->ast_size; i++) {
    a = ast_sort(ast_operand(a, i));
  }

  if (ast_is_kind(a, ast::ordered)) {
    return a;
  }

  a = ast_sort_childs(a, 0, ast_size(a) - 1);

  return a;
}

ast *ast_copy(ast *a) {
  ast *b = (ast *)calloc(sizeof(ast), 1);

  b->ref_count = 1;

  b->ast_kind = a->ast_kind;
  b->ast_size = a->ast_size;
  b->ast_reserved_size = a->ast_reserved_size;
  b->ast_format = a->ast_format;

  if (ast_is_kind(a, ast::integer)) {
    b->ast_int = new Int(*a->ast_int);
    return b;
  }

  if (ast_is_kind(a, ast::symbol)) {
    b->ast_sym = strdup(a->ast_sym);
    return b;
  }

  b->ast_childs = (ast **)calloc(sizeof(void (ast::*)()), b->ast_reserved_size);

  for (size_t i = 0; i < ast_size(a); i++) {
    b->ast_childs[i] = ast_inc_ref(ast_operand(a, i));
  }

  return b;
}

inline ast *ast_set_to_undefined(ast *a) {
  ast_assign_modifiable_ref(a, &a);

  ast_delete_operands(a);
  ast_delete_metadata(a);

  ast_set_kind(a, ast::undefined);

  return a;
}

inline ast *ast_set_to_fail(ast *a) {
  ast_assign_modifiable_ref(a, &a);

  ast_delete_operands(a);
  ast_delete_metadata(a);

  ast_set_kind(a, ast::fail);

  return a;
}

inline ast *ast_set_to_int(ast *a, Int v) {
  ast_assign_modifiable_ref(a, &a);

  ast_delete_operands(a);
  ast_delete_metadata(a);

  ast_set_kind(a, ast::integer);

  a->ast_int = new Int(v);

  return a;
}

inline ast *ast_replace_operand(ast *a, ast *v, size_t i) {
  ast_assign_modifiable_ref(a, &a);

  ast_set_operand(a, v, i);

  return a;
}

inline ast *ast_set_op_to_int(ast *a, size_t i, Int v) {
  ast_assign_modifiable_ref(a, &a);

  ast *t = ast_operand(a, i);

  t = ast_set_to_int(t, v);

  return ast_replace_operand(a, t, i);
}

inline ast *ast_set_to_fra(ast *a, Int u, Int v) {
  ast_assign_modifiable_ref(a, &a);

  ast_delete_operands(a);
  ast_delete_metadata(a);

  ast_set_kind(a, ast::fraction);

  ast_insert(a, ast_integer(u), 0);
  ast_insert(a, ast_integer(v), 1);

  return a;
}

inline ast *ast_set_op_to_fra(ast *a, size_t i, Int u, Int v) {
  ast_assign_modifiable_ref(a, &a);

  ast *t = ast_operand(a, i);

  t = ast_set_to_fra(t, u, v);

  return ast_replace_operand(a, t, i);
}

inline ast *ast_set_to_sym(ast *a, const char *s) {
  ast_assign_modifiable_ref(a, &a);

  ast_delete_operands(a);

  ast_delete_metadata(a);

  ast_set_kind(a, ast::symbol);

  a->ast_sym = strdup(s);

  return a;
}

inline ast *ast_set_op_to_sym(ast *a, size_t i, const char *s) {
  ast_assign_modifiable_ref(a, &a);

  ast *t = ast_operand(a, i);

  t = ast_set_to_sym(t, s);

  return ast_replace_operand(a, t, i);
}

// a = a + b
inline ast *ast_set_inplace_add_consts(ast *a, ast *b) {
  assert(ast_is_kind(a, ast::constant));
  assert(ast_is_kind(b, ast::constant));

  ast_assign_modifiable_ref(a, &a);

  if (ast_is_kind(a, ast::integer) && ast_is_kind(b, ast::integer)) {
    Int x = ast_value(a);
    Int y = ast_value(b);

    return ast_set_to_int(a, x + y);
  }

  if (ast_is_kind(a, ast::fraction) && ast_is_kind(b, ast::fraction)) {
    Int x = ast_value(ast_operand(a, 0));
    Int y = ast_value(ast_operand(a, 1));
    Int w = ast_value(ast_operand(b, 0));
    Int z = ast_value(ast_operand(b, 1));

    Int e = x * z + w * y;
    Int k = y * z;

    if (e % k == 0) {
      a = ast_set_to_int(a, e / k);
    } else {

      Int g = abs(gcd(e, k));

      a = ast_set_to_fra(a, e / g, k / g);
    }

    return a;
  }

  if (ast_is_kind(a, ast::fraction) && ast_is_kind(b, ast::integer)) {
    Int x = ast_value(ast_operand(a, 0));
    Int y = ast_value(ast_operand(a, 1));
    Int w = ast_value(b);

    Int e = x + w * y;
    Int k = y;

    if (e % k == 0) {
      a = ast_set_to_int(a, e / k);
    } else {

      Int g = abs(gcd(e, k));

      a = ast_set_to_fra(a, e / g, k / g);
    }

    return a;
  }

  if (ast_is_kind(b, ast::fraction) && ast_is_kind(a, ast::integer)) {
    Int x = ast_value(ast_operand(b, 0));
    Int y = ast_value(ast_operand(b, 1));
    Int w = ast_value(a);

    Int e = x + w * y;
    Int k = y;

    if (e % k == 0) {
      a = ast_set_to_int(a, e / k);
    } else {

      Int g = abs(gcd(e, k));

      a = ast_set_to_fra(a, e / g, k / g);
    }

    return a;
  }

  return a;
}

inline ast *ast_set_inplace_add_consts(ast *a, Int b) {
  assert(ast_is_kind(a, ast::constant));
  ast_assign_modifiable_ref(a, &a);

  if (ast_is_kind(a, ast::integer)) {
    Int x = ast_value(a);

    return ast_set_to_int(a, x + b);
  }

  if (ast_is_kind(a, ast::fraction)) {
    Int x = ast_value(ast_operand(a, 0));
    Int y = ast_value(ast_operand(a, 1));

    Int e = x + b * y;

    if (e % y == 0) {
      a = ast_set_to_int(a, e / y);
    } else {

      Int g = abs(gcd(e, y));

      a = ast_set_to_fra(a, e / g, y / g);
    }

    return a;
  }

  return a;
}

inline ast *ast_set_op_inplace_add_consts(ast *a, size_t i, ast *b) {
  ast_assign_modifiable_ref(a, &a);

  ast *t = ast_operand(a, i);

  t = ast_set_inplace_add_consts(t, b);

  return ast_replace_operand(a, t, i);
}

inline ast *ast_set_op_inplace_add_consts(ast *a, size_t i, Int b) {
  ast_assign_modifiable_ref(a, &a);

  ast *t = ast_operand(a, i);

  t = ast_set_inplace_add_consts(t, b);

  return ast_replace_operand(a, t, i);
}

inline ast *ast_set_inplace_mul_consts(ast *a, ast *b) {
  assert(ast_is_kind(a, ast::constant));
  assert(ast_is_kind(b, ast::constant));

  ast_assign_modifiable_ref(a, &a);

  if (ast_is_kind(a, ast::integer) && ast_is_kind(b, ast::integer)) {
    Int x = ast_value(a);
    Int y = ast_value(b);

    return ast_set_to_int(a, x * y);
  }

  if (ast_is_kind(a, ast::fraction) && ast_is_kind(b, ast::fraction)) {
    Int x = ast_value(ast_operand(a, 0));
    Int y = ast_value(ast_operand(a, 1));
    Int w = ast_value(ast_operand(b, 0));
    Int z = ast_value(ast_operand(b, 1));

    Int e = x * w;
    Int k = y * z;

    if (e % k == 0) {
      a = ast_set_to_int(a, e / k);
    } else {

      Int g = abs(gcd(e, k));

      a = ast_set_to_fra(a, e / g, k / g);
    }

    return a;
  }

  if (ast_is_kind(a, ast::fraction) && ast_is_kind(b, ast::integer)) {
    Int x = ast_value(ast_operand(a, 0));
    Int y = ast_value(ast_operand(a, 1));
    Int w = ast_value(b);

    Int e = x * w;
    Int k = y;

    if (e % k == 0) {
      a = ast_set_to_int(a, e / k);
    } else {

      Int g = abs(gcd(e, k));

      a = ast_set_to_fra(a, e / g, k / g);
    }

    return a;
  }

  if (ast_is_kind(b, ast::fraction) && ast_is_kind(a, ast::integer)) {
    Int x = ast_value(ast_operand(b, 0));
    Int y = ast_value(ast_operand(b, 1));
    Int w = ast_value(a);

    Int e = x * w;
    Int k = y;

    if (e % k == 0) {
      a = ast_set_to_int(a, e / k);
    } else {

      Int g = abs(gcd(e, k));

      a = ast_set_to_fra(a, e / g, k / g);
    }

    return a;
  }

  return a;
}

inline ast *ast_set_inplace_mul_consts(ast *a, Int b) {
  assert(ast_is_kind(a, ast::constant));

  ast_assign_modifiable_ref(a, &a);

  if (ast_is_kind(a, ast::integer)) {
    Int x = ast_value(a);

    return ast_set_to_int(a, x * b);
  }

  if (ast_is_kind(a, ast::fraction)) {
    Int x = ast_value(ast_operand(a, 0));
    Int y = ast_value(ast_operand(a, 1));

    Int e = x * b;

    if (e % y == 0) {
      a = ast_set_to_int(a, e / y);
    } else {

      Int g = abs(gcd(e, y));

      a = ast_set_to_fra(a, e / g, y / g);
    }

    return a;
  }
  return a;
}

inline ast *ast_set_op_inplace_mul_consts(ast *a, size_t i, ast *b) {
  ast_assign_modifiable_ref(a, &a);

  ast *t = ast_operand(a, i);

  t = ast_set_inplace_mul_consts(t, b);

  return ast_replace_operand(a, t, i);
}

inline ast *ast_set_op_inplace_mul_consts(ast *a, size_t i, Int b) {
  ast_assign_modifiable_ref(a, &a);

  ast *t = ast_operand(a, i);

  t = ast_set_inplace_mul_consts(t, b);

  return ast_replace_operand(a, t, i);
}

inline ast *ast_set_to_mul(Int v, ast *a) {
  ast_assign_modifiable_ref(a, &a);

  if (ast_is_kind(a, ast::mul)) {
    ast_insert(a, ast_integer(v), 0);
  } else {
    a = ast_create(ast::mul, {ast_integer(v), a});
  }

  return a;
}

inline ast *ast_set_to_mul(ast *a, ast *b) {
  ast_assign_modifiable_ref(a, &a);

  if (ast_is_kind(a, ast::mul)) {
    ast_insert(a, b, 0);
  } else {
    a = ast_create(ast::mul, {a, b});
  }

  return a;
}

inline ast *ast_set_op_to_mul(ast *a, size_t i, Int v) {
  ast_assign_modifiable_ref(a, &a);

  ast *t = ast_operand(a, i);

  t = ast_set_to_mul(v, t);

  return ast_replace_operand(a, t, i);
}

inline ast *ast_set_op_to_mul(ast *a, size_t i, ast *v) {
  ast_assign_modifiable_ref(a, &a);

  ast *t = ast_operand(a, i);

  t = ast_set_to_mul(v, t);

  return ast_replace_operand(a, t, i);
}

inline ast *ast_set_to_pow(ast *a, Int e) {
  ast_assign_modifiable_ref(a, &a);

  // if(ast_is_kind(a, ast::pow)) {
  // 	ast_delete(ast_operand(a, 1));

  // 	ast_set_operand(a, ast_integer(e), 1);

  // 	return a;
  // }

  return ast_create(ast::pow, {a, ast_integer(e)});
}

inline ast *ast_set_to_add(ast *a, ast *e) {
  ast_assign_modifiable_ref(a, &a);

  if (ast_is_kind(a, ast::add)) {
    ast_insert(a, e, ast_size(a));

    return a;
  }

  return ast_create(ast::add, {a, e});
}

inline ast *ast_set_op_to_add(ast *a, size_t i, ast *v) {
  ast_assign_modifiable_ref(a, &a);

  ast *t = ast_operand(a, i);

  t = ast_set_to_add(t, v);

  return ast_replace_operand(a, t, i);
}

inline ast *ast_set_op_pow_add_to_deg(ast *a, size_t i, ast *e) {
  assert(ast_is_kind(ast_operand(a, i), ast::pow));

  ast_assign_modifiable_ref(a, &a);

  ast *b = ast_operand(a, i);

  ast *p = ast_create(
      ast::pow, {ast_inc_ref(ast_operand(b, 0)),
                 ast_create(ast::add, {ast_inc_ref(ast_operand(b, 1)), e})});

  ast_delete(b);

  ast_set_operand(a, p, i);

  return a;
}

inline ast *ast_set_op_to_pow(ast *a, size_t i, Int v) {
  ast_assign_modifiable_ref(a, &a);

  ast *t = ast_create(ast::pow, {ast_operand(a, i), ast_integer(v)});

  return ast_replace_operand(a, t, i);
}

inline ast *ast_set_op_to_pow(ast *a, size_t i, ast *v) {
  ast_assign_modifiable_ref(a, &a);

  ast *t = ast_create(ast::pow, {ast_operand(a, i), v});

  return ast_replace_operand(a, t, i);
}

inline ast *ast_detatch_operand(ast *a, size_t i) {
  ast *b = ast_operand(a, i);

  ast_set_operand(a, 0, i);

  ast_remove(a, i);

  return b;
}

inline ast *eval_add_consts(ast *u, size_t i, ast *v, size_t j) {
  ast *a = ast_operand(u, i);
  ast *b = ast_operand(v, j);

  if (a == 0 || b == 0) {
    return u;
  }

  return ast_set_op_inplace_add_consts(u, i, b);
}

inline ast *eval_mul_consts(ast *u, size_t i, ast *v, size_t j) {
  ast *a = ast_operand(u, i);
  ast *b = ast_operand(v, j);

  if (a == 0 || b == 0) {
    return u;
  }

  return ast_set_op_inplace_mul_consts(u, i, b);
}

inline ast *eval_add_int(ast *a, Int b) {
  if (ast_is_kind(a, ast::integer)) {
    return ast_set_to_int(a, ast_value(a) + b);
  }

  assert(ast_is_kind(a, ast::fraction));

  Int num = ast_value(ast_operand(a, 0));
  Int den = ast_value(ast_operand(a, 1));

  num = b * den + num;

  Int cff = abs(gcd(num, den));

  if (den / cff == 1) {
    return ast_set_to_int(a, num / cff);
  }

  return ast_set_to_fra(a, num / cff, den / cff);
}

inline ast *eval_add_nconst(ast *u, size_t i, ast *v, size_t j) {
  assert(ast_is_kind(u, ast::add) && ast_is_kind(v, ast::add));

  assert(ast_is_kind(ast_operand(u, i), ast::summable));
  assert(ast_is_kind(ast_operand(v, j), ast::summable));

  ast *a = ast_operand(u, i);
  ast *b = ast_operand(v, j);

  if (a == 0 || b == 0) {
    return 0;
  }

  if (ast_is_kind(a, ast::pow) && ast_is_kind(b, ast::symbol)) {
    return 0;
  }

  if (ast_is_kind(a, ast::symbol) && ast_is_kind(b, ast::pow)) {
    return 0;
  }

  int kind = ast_kind(a) & ast_kind(b);

  if (kind & (ast::symbol | ast::pow)) {
    if (ast_cmp(a, b, ast::add) == 0) {
      return ast_set_op_to_mul(u, i, 2);
    }

    return 0;
  }

  long size_a = ast_size(a);

  if (kind & ast::mul) {
    long size_b = ast_size(b);

    ast *c;

    long size_c = -1;

    if (ast_is_kind(ast_operand(a, 0), ast::integer) &&
        ast_is_kind(ast_operand(b, 0), ast::integer) &&
        std::abs(size_a - size_b) != 0) {
      return 0;
    }
    if (size_b > size_a) {
      c = a;
      a = b;
      b = c;

      size_c = size_b;
      size_b = size_a;
      size_a = size_c;
    }

    assert(size_a == size_b || size_a == size_b + 1);

    long size =
        size_b - (ast_is_kind(ast_operand(b, 0), ast::constant) ? 1 : 0);

    for (long x = 0; x < size; x++) {
      if (ast_cmp(ast_operand(a, size_a - x - 1),
                  ast_operand(b, size_b - x - 1), ast::add) != 0) {
        return 0;
      }
    }

    int ka = ast_kind(ast_operand(a, 0));
    int kb = ast_kind(ast_operand(b, 0));

    // NOTE: sinse we are getting a modifiable reference
    // from the ast_eval_add method, the following operations
    // are valid.
    assert(u->ref_count == 1);

    ast_assign_modifiable_ref(u, &u);

    assert(ast_operand(u, i)->ref_count == 1);

    ast_assign_modifiable_ref(ast_operand(u, i), &a);

    if ((ka & ast::constant) && (kb & ast::constant)) {
      a = ast_set_op_inplace_add_consts(a, 0, ast_operand(b, 0));
    } else if (ka & ast::constant) {
      a = ast_set_op_inplace_add_consts(a, 0, 1);
    } else {
      a = ast_set_to_mul(2, a);
    }

    ast_set_operand(u, a, i);

    if (size_c != -1) {
      ast_set_operand(u, a, i);
      ast_set_operand(v, b, j);
    }

    return u;
  }

  if (ast_is_kind(b, ast::mul)) {
    return 0;
  }

  // a is a mul and b is a sym or a pow
  assert(ast_is_kind(a, ast::mul));
  assert(ast_is_kind(b, ast::symbol | ast::pow));

  if (size_a > 2) {
    return 0;
  }

  ast *a0 = ast_operand(a, 0);
  ast *a1 = ast_operand(a, 1);

  long ki = ast_kind(a1) & ast_kind(b);

  if (!ast_is_kind(a0, ast::constant) || !ki) {
    return 0;
  }

  if (ast_cmp(b, a1, ast::mul) == 0) {
    // NOTE: sinse we are getting a modifiable reference
    // from the ast_eval_add method, the following operations
    // are valid.

    assert(u->ref_count == 1);

    ast_assign_modifiable_ref(u, &u);

    assert(ast_operand(u, i)->ref_count == 1);

    ast_assign_modifiable_ref(ast_operand(u, i), &a);

    a = ast_set_op_inplace_add_consts(a, 0, 1);

    ast_set_operand(u, a, i);

    return u;
  }

  return 0;
}

inline ast *eval_mul_nconst(ast *u, size_t i, ast *v, size_t j) {
  assert(ast_is_kind(u, ast::mul) && ast_is_kind(v, ast::mul));

  assert(ast_is_kind(ast_operand(u, i), ast::multiplicable));
  assert(ast_is_kind(ast_operand(v, j), ast::multiplicable));

  ast *a = ast_operand(u, i);
  ast *b = ast_operand(v, j);

  if (a == 0 || b == 0) {
    return 0;
  }

  if (ast_is_kind(a, ast::add) && ast_is_kind(b, ast::add)) {

    if (ast_cmp(a, b, ast::mul) == 0) {
      return ast_set_op_to_pow(u, i, 2);
    }

    return 0;
  }

  // printf("--------> %s   %%%%%%%%   %s\n", ast_to_string(a).c_str(),
  // ast_to_string(b).c_str());
  if (ast_is_kind(a, ast::add) && ast_is_kind(b, ast::pow)) {

    // printf("--------> %s   *****   %s\n", ast_to_string(a).c_str(),
    // ast_to_string(b).c_str());

    if (ast_cmp(a, ast_operand(b, 0), ast::mul) == 0) {
      ast *e = ast_create(ast::add,
                          {ast_integer(1), ast_inc_ref(ast_operand(b, 1))});

      u = ast_set_op_to_pow(u, i, e);
      ast_set_operand(u, ast_eval(ast_operand(u, i)), i);
      return u;
    }

    return 0;
  }

  if (ast_is_kind(a, ast::pow) && ast_is_kind(b, ast::add)) {
    // printf("--------> %s   *****   %s\n", ast_to_string(a).c_str(),
    // ast_to_string(b).c_str());

    if (ast_cmp(b, ast_operand(a, 0), ast::mul) == 0) {
      u = ast_set_op_pow_add_to_deg(u, i, ast_integer(1));
      ast_set_operand(u, ast_eval(ast_operand(u, i)), i);
      return u;
    }

    return 0;

    // if(ast_cmp(a, b, ast::mul) == 0) {
    // 	return ast_set_op_to_pow(u, i, 2);
    // }

    return 0;
  }

  if (ast_is_kind(a, ast::pow) && ast_is_kind(b, ast::pow)) {
    if (ast_cmp(ast_operand(a, 0), ast_operand(b, 0), ast::mul) == 0) {
      ast *k = ast_inc_ref(ast_operand(b, 1));

      u = ast_set_op_pow_add_to_deg(u, i, k);
      k = ast_eval(ast_operand(u, i));

      if (ast_operand(u, i) != k) {
        ast_delete(ast_operand(u, i));
        ast_set_operand(u, k, i);
      }

      return u;
    }

    return 0;
  }

  if (ast_is_kind(a, ast::symbol | ast::funcall) &&
      ast_is_kind(b, ast::symbol | ast::funcall)) {
    if (ast_cmp(a, b, ast::mul) == 0) {
      // printf("B\n");
      ast *t = ast_set_op_to_pow(u, i, 2);

      // ast_print(t);

      return t;
    }

    return 0;
  }

  if (ast_is_kind(a, ast::pow) && ast_is_kind(b, ast::symbol | ast::funcall)) {
    if (ast_cmp(ast_operand(a, 0), b, ast::mul) == 0) {
      ast *k = ast_integer(1);

      u = ast_set_op_pow_add_to_deg(u, i, k);

      k = ast_eval(ast_operand(u, i));

      if (ast_operand(u, i) != k) {
        ast_delete(ast_operand(u, i));
      }
      // printf("C\n");
      // ast_print(u);

      return ast_replace_operand(u, k, i);
    }

    return 0;
  }

  return 0;
}

ast *eval_add_add(ast *a, ast *b) {
  assert(ast_is_kind(a, ast::add));
  assert(ast_is_kind(b, ast::add));

  size_t j = 0;
  size_t i = 0;

  ast *u = 0;
  ast *v = 0;

  while (i < ast_size(a) && j < ast_size(b)) {
    assert(!ast_is_kind(ast_operand(b, j), ast::add));

    u = ast_operand(a, i);
    v = ast_operand(b, j);

    if (u == 0) {
      i++;
      continue;
    }

    if (v == 0) {
      j++;
      continue;
    }

    ast *tmp = 0;

    if (ast_is_kind(u, ast::constant) && ast_is_kind(v, ast::constant)) {
      tmp = eval_add_consts(a, i, b, j);
    } else if (ast_is_kind(u, ast::summable) && ast_is_kind(v, ast::summable)) {
      tmp = eval_add_nconst(a, i, b, j);
    }

    if (tmp) {
      a = tmp;

      i = i + 1;
      j = j + 1;
    } else {
      int order = ast_cmp(u, v, ast::add);

      if (order < 0) {
        i = i + 1;
      } else {
        ast_insert(a, ast_inc_ref(v), i++);
        j = j + 1;
      }
    }
  }

  while (j < ast_size(b)) {
    if (i >= ast_size(a)) {
      v = ast_operand(b, j++);
      ast_insert(a, ast_inc_ref(v), ast_size(a));
    } else {
      u = ast_operand(a, i);
      v = ast_operand(b, j);

      if (v == 0) {
        j++;
      } else if (u == 0 || ast_cmp(u, v, ast::add) < 0) {
        i++;
      } else {
        ast_insert(a, ast_inc_ref(v), i++);
        j = j + 1;
      }
    }
  }

  return a;
}

inline ast *eval_mul_mul(ast *a, ast *b) {
  assert(ast_is_kind(a, ast::mul));
  assert(ast_is_kind(b, ast::mul));

  size_t j = 0;
  size_t i = 0;

  ast *u = 0;
  ast *v = 0;

  while (i < ast_size(a) && j < ast_size(b)) {

    assert(!ast_is_kind(ast_operand(b, j), ast::mul));

    u = ast_operand(a, i);
    v = ast_operand(b, j);

    if (u == 0) {
      i++;
      continue;
    }

    if (v == 0) {
      j++;
      continue;
    }

    ast *tmp = 0;

    if (ast_is_kind(u, ast::constant) && ast_is_kind(v, ast::constant)) {
      tmp = eval_mul_consts(a, i, b, j);
      a = tmp ? tmp : a;
    } else if (ast_is_kind(u, ast::multiplicable) &&
               ast_is_kind(v, ast::multiplicable)) {
      tmp = eval_mul_nconst(a, i, b, j);
      a = tmp ? tmp : a;
    }

    if (tmp) {
      i = i + 1;
      j = j + 1;
    } else {
      int order = ast_cmp(u, v, ast::mul);

      if (order < 0) {
        i = i + 1;
      } else {
        ast_insert(a, ast_inc_ref(v), i++);
        j = j + 1;
      }
    }
  }
  // printf(">> %s\n", ast_to_string(a).c_str());
  // printf("(%li, %li)\n", i, j);
  while (j < ast_size(b)) {
    if (i >= ast_size(a)) {
      v = ast_operand(b, j++);
      ast_insert(a, ast_inc_ref(v), ast_size(a));
    } else {
      u = ast_operand(a, i);
      v = ast_operand(b, j);

      if (v == 0) {
        j++;
      } else if (u == 0 || ast_cmp(u, v, ast::mul) < 0) {
        i++;
      } else {
        ast_insert(a, ast_inc_ref(v), i++);
        j = j + 1;
      }
    }
  }

  return a;
}

ast *eval_mul_int(ast *u, size_t i, Int v) {
  ast *a = ast_operand(u, i);

  if (ast_is_kind(a, ast::integer)) {
    return ast_set_op_inplace_mul_consts(u, i, v);
  }

  if (ast_is_kind(a, ast::add | ast::sub)) {
    assert(u->ref_count == 1);

    ast_assign_modifiable_ref(u, &u);

    assert(ast_operand(u, i)->ref_count == 1);

    ast_assign_modifiable_ref(ast_operand(a, i), &a);

    for (size_t j = 0; j < ast_size(a); j++) {
      a = ast_set_op_inplace_mul_consts(a, j, v);
    }

    ast_set_operand(u, a, i);

    return u;
  }

  if (ast_is_kind(a, ast::mul)) {
    return ast_set_op_to_mul(u, i, v);
  }

  if (ast_is_kind(a, ast::div)) {
    return eval_mul_int(a, 0, v);
  }

  if (ast_is_kind(a, ast::sqrt | ast::pow | ast::fact | ast::funcall |
                         ast::symbol)) {
    return ast_set_op_inplace_mul_consts(u, i, v);
  }

  assert(ast_is_kind(a, ast::undefined | ast::fail));

  return u;
}

ast *ast_raise_to_first_op(ast *a) {
  ast_assign_modifiable_ref(a, &a);

  if (ast_is_kind(ast_operand(a, 0), ast::integer)) {
    return ast_set_to_int(a, ast_value(ast_operand(a, 0)));
  }

  if (ast_is_kind(ast_operand(a, 0), ast::symbol)) {
    return ast_set_to_sym(a, ast_id(ast_operand(a, 0)));
  }

  if (ast_is_kind(ast_operand(a, 0), ast::funcall)) {
    // TODO: ast_set_to_funcall
    a->ast_sym = strdup(ast_id(a));

    ast_set_kind(a, ast::funcall);

    ast *t = ast_operand(a, 0);

    ast **t_childs = a->ast_childs;
    size_t t_size = a->ast_size;
    size_t t_rsize = a->ast_reserved_size;

    a->ast_childs = t->ast_childs;
    a->ast_size = t->ast_size;
    a->ast_reserved_size = t->ast_reserved_size;

    t->ast_childs = t_childs;
    t->ast_size = t_size;
    t->ast_reserved_size = t_rsize;

    ast_delete(t);

    return a;
  }

  ast *t = ast_operand(a, 0);

  ast_set_operand(a, 0, 0);

  ast::kind k = ast_kind(a);

  ast_set_kind(a, ast_kind(t));
  ast_set_kind(t, k);

  ast **t_childs = a->ast_childs;
  size_t t_size = a->ast_size;
  size_t t_rsize = a->ast_reserved_size;

  a->ast_childs = t->ast_childs;
  a->ast_size = t->ast_size;
  a->ast_reserved_size = t->ast_reserved_size;

  t->ast_childs = t_childs;
  t->ast_size = t_size;
  t->ast_reserved_size = t_rsize;

  ast_delete(t);

  return a;
}

ast *ast_eval_add(ast *a, bool print, ast *p) {
  ast_assign_modifiable_ref(a, &a);
  p = p ? p : a;

  ast *t = 0;

  for (size_t i = 0; i < ast_size(a); i++) {
    t = ast_eval(ast_operand(a, i), print, p);
    a = ast_replace_operand(a, t, i);
  }

  a = ast_sort_childs(a, 0, ast_size(a) - 1);

  if (ast_is_kind(ast_operand(a, 0), ast::add)) {
    t = ast_detatch_operand(a, 0);

    eval_add_add(a, t);

    ast_delete(t);
  }

  size_t j = 0;

  for (long i = 1; i < (long)ast_size(a); i++) {

    ast *aj = ast_operand(a, j);
    ast *ai = ast_operand(a, i);

    t = 0;

    if (ast_is_kind(ai, ast::fail) || ast_is_kind(aj, ast::fail)) {
      return ast_set_to_fail(a);
    } else if (ast_is_kind(ai, ast::undefined) ||
               ast_is_kind(aj, ast::undefined)) {
      return ast_set_to_undefined(a);
    } else if (ast_is_zero(ast_operand(a, j))) {
      ast_remove(a, j);
    } else if (ast_is_kind(ai, ast::add)) {
      ai = ast_detatch_operand(a, i--);
      a = eval_add_add(a, ai);
      ast_delete(ai);
    } else if (ast_is_kind(aj, ast::constant) &&
               ast_is_kind(ai, ast::constant)) {

      t = eval_add_consts(a, j, a, i);

      if (t) {
        a = t;
        ast_remove(a, i--);
      }

    } else if (ast_is_kind(aj, ast::summable) &&
               ast_is_kind(ai, ast::summable)) {
      t = eval_add_nconst(a, j, a, i);

      if (t) {
        a = t;
        ast_remove(a, i--);
      }
    }

    if (!t)
      j = i;

    // // TODO: code like this could be used to return the history of operations
    // if (tmp && print) {
    //   printf("%s\n", ast_to_string(p).c_str());
    // }
  }

  if (ast_size(a) == 0) {
    ast_delete_operands(a);
    ast_delete_metadata(a);
    ast_set_kind(a, ast::integer);

    a->ast_int = new Int(0);

  } else if (ast_size(a) == 1) {
    a = ast_raise_to_first_op(a);
  }

  return a;
}

ast *ast_eval_mul(ast *a, bool print, ast *parent) {
  ast_assign_modifiable_ref(a, &a);

  // ast* p = ast_inc_ref(a);

  for (size_t i = 0; i < ast_size(a); i++) {
    ast_replace_operand(
        a, ast_eval(ast_operand(a, i), print, parent ? parent : a), i);
  }

  ast *t = 0;

	a = ast_sort_childs(a, 0, ast_size(a) - 1);

  size_t j = 0;

  t = 0;

  if (ast_is_kind(ast_operand(a, 0), ast::mul)) {
    t = ast_detatch_operand(a, 0);
    eval_mul_mul(a, t);

    ast_delete(t);
  }

  // printf("---: %s\n", ast_to_string(a).c_str());
  for (long i = 1; i < (long)ast_size(a); i++) {
    ast *aj = ast_operand(a, j);
    ast *ai = ast_operand(a, i);

    t = 0;

    if (ast_is_kind(ai, ast::fail) || ast_is_kind(aj, ast::fail)) {
      return ast_set_to_fail(a);
    } else if (ast_is_kind(ai, ast::undefined) ||
               ast_is_kind(aj, ast::undefined)) {
      return ast_set_to_undefined(a);
    } else if (ast_is_zero(ast_operand(a, i)) ||
               ast_is_zero(ast_operand(a, j))) {
      return ast_set_to_int(a, 0);
    } else if (ast_is_kind(ai, ast::mul)) {
      ast_set_operand(a, 0, i);
      ast_remove(a, i--);
      a = eval_mul_mul(a, ai);
      ast_delete(ai);
    } else if (ast_is_kind(aj, ast::constant) &&
               ast_is_kind(ai, ast::constant)) {

      // if(ast_value(aj) == 1) {
      // 	ast_remove(a, j);
      // 	continue;
      // }

      // if(ast_value(ai) == 1) {
      // 	ast_remove(a, i);
      // 	continue;
      // }

      t = eval_mul_consts(a, j, a, i);

      if (t) {
        a = t;
        ast_remove(a, i--);
      }
    } else if (ast_is_kind(aj, ast::multiplicable) &&
               ast_is_kind(ai, ast::multiplicable)) {
      t = eval_mul_nconst(a, j, a, i);

      if (t) {
        a = t;
        ast_remove(a, i--);
      }
    }

    if (t == 0) {
      j = i;
    }

    // // TODO: code like this could be used to return the history of operations
    // if (tmp && print) {
    //   printf("%s\n", ast_to_string(parent ? parent : a).c_str());
    // }
  }
  if (ast_is_kind(a, ast::mul) && ast_size(a) == 1) {
    ast_raise_to_first_op(a);
  }

  return a;
}

ast *ast_eval_sub(ast *a, bool print, ast *parent) {
  for (size_t i = 1; i < ast_size(a); i++) {
    eval_mul_int(a, i, -1);
  }

  ast_set_kind(a, ast::add);

  return ast_eval(a, print, parent ? parent : a);
}

ast *ast_eval_pow(ast *a, bool print, ast *parent) {
  ast_assign_modifiable_ref(a, &a);

  ast_set_operand(a, ast_eval(ast_operand(a, 1), print, parent ? parent : a),
                  1);

  // TODO: if expoent is zero return 1, if expoent is 1 return base

  ast_set_operand(a, ast_eval(ast_operand(a, 0), print, parent ? parent : a),
                  0);
  if (!ast_is_kind(ast_operand(a, 1), ast::integer)) {
    return a;
  }

  if (ast_value(ast_operand(a, 1)) == 1) {
		return ast_raise_to_first_op(a);
  }

  if (ast_value(ast_operand(a, 1)) == 0) {
    return ast_set_to_int(a, 1);
  }

  if (ast_is_kind(ast_operand(a, 0), ast::integer)) {
    Int b = ast_value(ast_operand(a, 0));
    Int c = ast_value(ast_operand(a, 1));

    bool n = c < 0;

    c = abs(c);

    Int d = pow(b, c);

    ast_remove(a, 1);

    if (!n || d == 1) {
      a = ast_set_to_int(a, d);
    } else {
      a = ast_set_to_fra(a, 1, d);
    }

    if (print) {
      printf("%s\n", ast_to_string(parent ? parent : a).c_str());
    }

    return a;
  }
  if (ast_is_kind(ast_operand(a, 0), ast::fraction)) {
    Int b = ast_value(ast_operand(ast_operand(a, 0), 0));
    Int c = ast_value(ast_operand(ast_operand(a, 0), 1));

    Int d = ast_value(ast_operand(a, 1));

    bool n = d < 0;

    d = abs(d);

    b = pow(b, d);
    c = pow(c, d);

    Int g = gcd(b, c);

    b = b / g;
    c = c / g;

    a = !n ? ast_set_to_fra(a, b, c) : ast_set_to_fra(a, c, b);

    if (print) {
      printf("%s\n", ast_to_string(parent ? parent : a).c_str());
    }

    return a;
  }

  if (ast_is_kind(ast_operand(a, 0), ast::terminal)) {
    return a;
  }

  if (ast_is_kind(ast_operand(a, 0), ast::mul)) {

    long long y = ast_value(ast_operand(a, 1)).longValue();

    ast *b = ast_detatch_operand(a, 0);

    a = ast_set_to_int(a, 1);

    while (y) {
      if (y % 2 == 1) {
        a = ast_create(ast::mul, {a, ast_inc_ref(b)});
        a = ast_eval(a);
      }

      y = y >> 1;

      ast *t = ast_create(ast::mul, ast_size(b));
      ast_set_size(t, ast_size(b));
      for (size_t i = 0; i < ast_size(b); i++) {
        ast *v = ast_create(ast::mul, {
                                          ast_inc_ref(ast_operand(b, i)),
                                          ast_inc_ref(ast_operand(b, i)),
                                      });

        ast_set_operand(t, v, i);
      }

			ast_assign(&b, t);
    }

    ast_delete(b);

    return a;
  }

  return a;
}

ast *ast_eval_div(ast *a, bool print, ast *parent) {
  ast_assign_modifiable_ref(a, &a);

  ast_set_kind(a, ast::mul);

  a = ast_set_op_to_pow(a, 1, -1);

  if (print) {
    printf("%s\n", ast_to_string(parent ? parent : a).c_str());
  }

  return ast_eval(a, print, parent ? parent : a);
}

ast *ast_eval_sqr(ast *a, bool print, ast *parent) {
  ast_assign_modifiable_ref(a, &a);

  ast_set_kind(a, ast::pow);

  ast_insert(a, ast_fraction(1, 2), 1);

  if (print) {
    printf("%s\n", ast_to_string(parent ? parent : a).c_str());
  }

  return ast_eval(a, print, parent ? parent : a);
}

ast *ast_eval_fac(ast *a, bool print, ast *parent) {
  if (ast_is_kind(ast_operand(a, 0), ast::integer)) {
    a = ast_set_to_int(a, fact(ast_value(ast_operand(a, 0))));

    if (print) {
      printf("%s\n", ast_to_string(parent ? parent : a).c_str());
    }

    return a;
  }

  if (ast_is_kind(ast_operand(a, 0), ast::fraction)) {
    Int c = fact(ast_value(ast_operand(ast_operand(a, 0), 0)));
    Int d = fact(ast_value(ast_operand(ast_operand(a, 0), 1)));

    Int g = abs(gcd(c, d));

    a = ast_set_to_fra(a, c / g, d / g);

    if (print) {
      printf("%s\n", ast_to_string(parent ? parent : a).c_str());
    }

    return a;
  }

  return a;
}

ast *ast_eval_fra(ast *a, bool print, ast *parent) {
  Int b = ast_value(ast_operand(a, 0));
  Int c = ast_value(ast_operand(a, 1));

  Int d = abs(gcd(b, c));

  a = ast_set_to_fra(a, b / d, c / d);

  if (print) {
    printf("%s\n", ast_to_string(parent ? parent : a).c_str());
  }

  return a;
}

ast *ast_eval(ast *a, bool print, ast *parent) {
  if (!a) {
    return a;
  } else if (ast_is_kind(a, ast::fraction)) {
    a = ast_eval_fra(a, print, parent);
  } else if (ast_is_kind(a, ast::add)) {
    a = ast_eval_add(a, print, parent);
  } else if (ast_is_kind(a, ast::mul)) {
    a = ast_eval_mul(a, print, parent);
  } else if (ast_is_kind(a, ast::sub)) {
    a = ast_eval_sub(a, print, parent);
  } else if (ast_is_kind(a, ast::div)) {
    a = ast_eval_div(a, print, parent);
  } else if (ast_is_kind(a, ast::pow)) {
    a = ast_eval_pow(a, print, parent);
  } else if (ast_is_kind(a, ast::sqrt)) {
    a = ast_eval_sqr(a, print, parent);
  } else if (ast_is_kind(a, ast::fact)) {
    a = ast_eval_fac(a, print, parent);
  }

  return a;
}

ast *ast_expand_mul(ast *a, size_t i, ast *b, size_t j) {
  ast *r = ast_operand(a, i);
  ast *s = ast_operand(b, j);

  if (ast_is_kind(r, ast::add) && ast_is_kind(s, ast::add)) {
    ast *u = ast_create(ast::add, ast_size(r) * ast_size(s));

    ast_set_size(u, ast_size(r) * ast_size(s));

    for (size_t k = 0; k < ast_size(r); k++) {
      for (size_t t = 0; t < ast_size(s); t++) {
        ast *v = ast_create(ast::mul, {ast_inc_ref(ast_operand(r, k)),
                                       ast_inc_ref(ast_operand(s, t))});

        ast_set_operand(u, v, k * ast_size(s) + t);
      }
    }

    return u;
  }

  if (ast_is_kind(r, ast::add)) {
    ast *u = ast_create(ast::add, ast_size(r));

    ast_set_size(u, ast_size(r));

    for (size_t k = 0; k < ast_size(r); k++) {
      ast *v = ast_create(ast::mul,
                          {ast_inc_ref(ast_operand(r, k)), ast_inc_ref(s)});

      ast_set_operand(u, v, k);
    }

    return u;
  }

  if (ast_is_kind(s, ast::add)) {
    ast *u = ast_create(ast::add, ast_size(s));

    ast_set_size(u, ast_size(s));

    for (size_t k = 0; k < ast_size(s); k++) {
      ast *v = ast_create(ast::mul,
                          {ast_inc_ref(r), ast_inc_ref(ast_operand(s, k))});

      ast_set_operand(u, v, k);
    }

    return u;
  }

  return ast_create(ast::mul, {ast_inc_ref(r), ast_inc_ref(s)});
}

ast *ast_expand_pow(ast *u, Int n) {
  if (n == 1)
    return ast_inc_ref(u);
  if (n == 0)
    return ast_integer(1);

  if (ast_is_kind(u, ast::add)) {
    Int c = fact(n);

    ast *o = ast_copy(u);

    ast *f = ast_detatch_operand(o, 0);

    if (ast_size(o) == 0)
      o = ast_set_to_int(o, 0);
    if (ast_size(o) == 1)
      o = ast_raise_to_first_op(o);

    ast *s = ast_create(ast::add, n.longValue() + 1);

    ast_set_size(s, n.longValue() + 1);

    for (Int k = 0; k <= n; k++) {
      ast *z = ast_create(
          ast::mul,
          {ast_integer(c / (fact(k) * fact(n - k))),
           ast_create(ast::pow, {ast_inc_ref(f), ast_integer(n - k)})});

      ast *t = ast_expand_pow(o, k);

      ast *q = t ? ast_create(ast::mul, {z, t}) : z;

      ast_set_operand(s, q, k.longValue());
    }

    ast_delete(f);
    ast_delete(o);

    return s;
  }

  if (ast_is_kind(u, ast::terminal)) {
    return 0;
  }

  return ast_create(ast::pow, {
                                  ast_inc_ref(u),
                                  ast_integer(n),
                              });

  // return ast_eval(t);
}

ast *ast_expand(ast *a) {
  ast_assign_modifiable_ref(a, &a);

  if (ast_is_kind(a, ast::terminal)) {
    return a;
  }

  if (ast_is_kind(a, ast::sub | ast::div | ast::fact)) {
    a = ast_eval(a);
  }

  if (ast_is_kind(a, ast::pow)) {
		ast_set_operand(a, ast_expand(ast_operand(a, 0)), 0);
    ast_set_operand(a, ast_expand(ast_operand(a, 1)), 1);

    if (ast_is_kind(ast_operand(a, 1), ast::integer)) {
      ast *t = ast_expand_pow(ast_operand(a, 0), ast_value(ast_operand(a, 1)));

      if (t) {
        ast_delete(a);
        a = t;
      }
    }
	}

  if (ast_is_kind(a, ast::mul)) {
    while (ast_size(a) > 1) {
      ast_set_operand(a, ast_expand(ast_operand(a, 0)), 0);
      ast_set_operand(a, ast_expand(ast_operand(a, 1)), 1);

      ast_insert(a, ast_expand_mul(a, 0, a, 1), 0);

      ast_remove(a, 1);
      ast_remove(a, 1);
    }

    a = ast_raise_to_first_op(a);
  }

  if (ast_is_kind(a, ast::add)) {
    for (size_t i = 0; i < ast_size(a); i++) {
      ast_set_operand(a, ast_expand(ast_operand(a, i)), i);
    }
  }

  return ast_eval(a);
}

} // namespace ast_teste