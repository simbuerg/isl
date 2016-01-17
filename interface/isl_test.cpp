#include "isl/Ctx.hpp"
#include "isl/BasicSet.hpp"
#include "isl/Set.hpp"
#include "isl/UnionSet.hpp"
#include "isl/UnionMap.hpp"
#include "isl/ScheduleNode.hpp"
#include "isl/MultiUnionPwAff.hpp"
#include "isl/Val.hpp"
#include "isl/MultiVal.hpp"
#include "isl/Id.hpp"
#include "isl/IdList.hpp"
#include "isl/IslException.h"
#include "isl/PwQpolynomial.hpp"
#include "isl/PwAff.hpp"
#include "isl/PwMultiAff.hpp"

#define ARRAY_SIZE(array) (sizeof(array)/sizeof(*array))

using namespace isl;

static int test_parse_multi_val(Ctx &C, const char *str) {
  try {
    isl::MultiVal MV = isl::MultiVal::readFromStr(C, str);
  } catch(IslException ex) {
    return -1;
  }
  return 0;
}

static bool test_parse_map_equal(Ctx &C, const char *m1, const char *m2) {
	Map M1 = Map::readFromStr(C, m1);
	Map M2 = Map::readFromStr(C, m2);
	return M1.isEqual(M2) == Bool::True;
}

/* Inputs for isl_pw_qpolynomial_gist tests.
 * "pwqp" is the input, "set" is the context and "gist" is the expected result.
 */
struct {
	const char *pwqp;
	const char *set;
	const char *gist;
} pwqp_gist_tests[] = {
	{ "{ [i] -> i }", "{ [k] : exists a : k = 2a }", "{ [i] -> i }" },
	{ "{ [i] -> i + [ (i + [i/3])/2 ] }", "{ [10] }", "{ [i] -> 16 }" },
	{ "{ [i] -> ([(i)/2]) }", "{ [k] : exists a : k = 2a+1 }",
	  "{ [i] -> -1/2 + 1/2 * i }" },
	{ "{ [i] -> i^2 : i != 0 }", "{ [i] : i != 0 }", "{ [i] -> i^2 }" },
};

static int test_pwqp(Ctx &C) {
  PwQpolynomial PwQp1 = PwQpolynomial::readFromStr(C,
	"{ [i,j,k] -> 1 + 9 * [i/5] + 7 * [j/11] + 4 * [k/13] }");
  PwQpolynomial PwQp2 = PwQpolynomial::readFromStr(C,
	"[j] -> { [i,k] -> 1 + 9 * [i/5] + 7 * [j/11] + 4 * [k/13] }");

  PwQp1.moveDims(DimType::Param, 0, DimType::In, 1, 1);
  PwQp1 = PwQp1.sub(PwQp2);
  assert(PwQp1.isZero() == Bool::True);

  for (int i = 0; i < ARRAY_SIZE(pwqp_gist_tests); ++i) {
	PwQp1 = PwQpolynomial::readFromStr(C, pwqp_gist_tests[i].pwqp);
	Set S = Set::readFromStr(C, pwqp_gist_tests[i].set);
	PwQp1 = PwQp1.gist(S);
	PwQp2 = PwQpolynomial::readFromStr(C, pwqp_gist_tests[i].gist);
	PwQp1 = PwQp1.sub(PwQp2);
	assert(PwQp1.isZero() == Bool::True);
  }

  return 0;
}

int test_parse(Ctx &C) {
  if (test_parse_multi_val(C, "{ A[B[2] -> C[5, 7]] }"))
    return -1;
  if (test_parse_multi_val(C, "[n] -> { [2] }"))
    return -1;
  if (test_parse_multi_val(C, "{ A[4, infty, NaN, -1/2, 2/3] }"))
    return -1;

  Map M = Map::readFromStr(C, "{ [i] -> [-i] }");
  M = Map::readFromStr(C, "{ A[i] -> L[([i/3])] }");
  M = Map::readFromStr(C, "{[[s] -> A[i]] -> [[s+1] -> A[i]]}");
  M = Map::readFromStr(C, "{ [p1, y1, y2] -> [2, y1, y2] : "
	"p1 = 1 && (y1 <= y2 || y2 = 0) }");

  assert(test_parse_map_equal(C, "{ [x,y]  : [([x/2]+y)/3] >= 1 }",
	"{ [x, y] : 2y >= 6 - x }"));
  assert(test_parse_map_equal(C, "{ [x,y] : x <= min(y, 2*y+3) }",
	"{ [x,y] : x <= y, 2*y + 3 }"));
  assert(test_parse_map_equal(C,
	"{ [x, y] : (y <= x and y >= -3) or (2y <= -3 + x and y <= -4) }",
	"{ [x,y] : x >= min(y, 2*y+3) }"));
  assert(test_parse_map_equal(C,
	"{[new,old] -> [new+1-2*[(new+1)/2],old+1-2*[(old+1)/2]]}",
	"{ [new, old] -> [o0, o1] : "
	"exists (e0 = [(-1 - new + o0)/2], e1 = [(-1 - old + o1)/2]: "
	"2e0 = -1 - new + o0 and 2e1 = -1 - old + o1 and o0 >= 0 and "
	"o0 <= 1 and o1 >= 0 and o1 <= 1) }"));
  assert(test_parse_map_equal(C,
	"{[new,old] -> [new+1-2*[(new+1)/2],old+1-2*[(old+1)/2]]}",
	"{[new,old] -> [(new+1)%2,(old+1)%2]}"));
  assert(test_parse_map_equal(C,
	"[n] -> { [c1] : c1>=0 and c1<=floord(n-4,3) }",
	"[n] -> { [c1] : c1 >= 0 and 3c1 <= -4 + n }"));
  assert(test_parse_map_equal(C,
	"{ [i,j] -> [i] : i < j; [i,j] -> [j] : j <= i }",
	"{ [i,j] -> [min(i,j)] }"));
  assert(test_parse_map_equal(C,
	"{ [i,j] : i != j }",
	"{ [i,j] : i < j or i > j }"));
  assert(test_parse_map_equal(C,
	"{ [i,j] : (i+1)*2 >= j }",
	"{ [i, j] : j <= 2 + 2i }"));
  assert(test_parse_map_equal(C,
	"{ [i] -> [i > 0 ? 4 : 5] }",
	"{ [i] -> [5] : i <= 0; [i] -> [4] : i >= 1 }"));
  assert(test_parse_map_equal(C,
	"[N=2,M] -> { [i=[(M+N)/4]] }",
	"[N, M] -> { [i] : N = 2 and 4i <= 2 + M and 4i >= -1 + M }"));
  assert(test_parse_map_equal(C,
	"{ [x] : x >= 0 }",
	"{ [x] : x-0 >= 0 }"));
  assert(test_parse_map_equal(C,
	"{ [i] : ((i > 10)) }",
	"{ [i] : i >= 11 }"));
  assert(test_parse_map_equal(C,
	"{ [i] -> [0] }",
	"{ [i] -> [0 * i] }"));
  assert(test_parse_map_equal(C,
	"{ [a] -> [b] : (not false) }",
	"{ [a] -> [b] : true }"));
  assert(test_parse_map_equal(C,
	"{ [i] : i/2 <= 5 }",
	"{ [i] : i <= 10 }"));
  assert(test_parse_map_equal(C,
	"{Sym=[n] [i] : i <= n }",
	"[n] -> { [i] : i <= n }"));
  assert(test_parse_map_equal(C,
	"{ [*] }",
	"{ [a] }"));
  assert(test_parse_map_equal(C,
	"{ [i] : 2*floor(i/2) = i }",
	"{ [i] : exists a : i = 2 a }"));
  assert(test_parse_map_equal(C,
	"{ [a] -> [b] : a = 5 implies b = 5 }",
	"{ [a] -> [b] : a != 5 or b = 5 }"));
  assert(test_parse_map_equal(C,
	"{ [a] -> [a - 1 : a > 0] }",
	"{ [a] -> [a - 1] : a > 0 }"));
  assert(test_parse_map_equal(C,
	"{ [a] -> [a - 1 : a > 0; a : a <= 0] }",
	"{ [a] -> [a - 1] : a > 0; [a] -> [a] : a <= 0 }"));
  assert(test_parse_map_equal(C,
	"{ [a] -> [(a) * 2 : a >= 0; 0 : a < 0] }",
	"{ [a] -> [2a] : a >= 0; [a] -> [0] : a < 0 }"));
  assert(test_parse_map_equal(C,
	"{ [a] -> [(a * 2) : a >= 0; 0 : a < 0] }",
	"{ [a] -> [2a] : a >= 0; [a] -> [0] : a < 0 }"));
  assert(test_parse_map_equal(C,
	"{ [a] -> [(a * 2 : a >= 0); 0 : a < 0] }",
	"{ [a] -> [2a] : a >= 0; [a] -> [0] : a < 0 }"));
  assert(test_parse_map_equal(C,
	"{ [a] -> [(a * 2 : a >= 0; 0 : a < 0)] }",
	"{ [a] -> [2a] : a >= 0; [a] -> [0] : a < 0 }"));


  {
	//test_parse_pwqp(ctx, "{ [i] -> i + [ (i + [i/3])/2 ] }");
  }
  {
	Map M = Map::readFromStr(C,
		"{ S1[i] -> [([i/10]),i%10] : 0 <= i <= 45 }"); }
  {

	//test_parse_pwaff(ctx, "{ [i] -> [i + 1] : i > 0; [a] -> [a] : a < 0 }");
  }
  {
//	test_parse_pwqp(ctx, "{ [x] -> ([(x)/2] * [(x)/3]) }");
  }

  return 0;
}

void test_set() {
  Ctx C = Ctx::alloc();
  Set S = Set::readFromStr(C, "{ [i,j] : 0 <= i <= 5 and 0 <= j <= 5 };");
  Set S1 = Set::readFromStr(C, "{ [i,j] : 0 <= i <= 5 and 0 <= j <= 5 };");

  Set R = S.union_(S1);
}

struct {
	int scale_tile;
	int shift_point;
	const char *domain;
	const char *schedule;
	const char *sizes;
	const char *tile;
	const char *point;
} tile_tests[] = {
	{ 0, 0, "[n] -> { S[i,j] : 0 <= i,j < n }",
	  "[{ S[i,j] -> [i] }, { S[i,j] -> [j] }]",
	  "{ [32,32] }",
	  "[{ S[i,j] -> [floor(i/32)] }, { S[i,j] -> [floor(j/32)] }]",
	  "[{ S[i,j] -> [i] }, { S[i,j] -> [j] }]",
	},
	{ 1, 0, "[n] -> { S[i,j] : 0 <= i,j < n }",
	  "[{ S[i,j] -> [i] }, { S[i,j] -> [j] }]",
	  "{ [32,32] }",
	  "[{ S[i,j] -> [32*floor(i/32)] }, { S[i,j] -> [32*floor(j/32)] }]",
	  "[{ S[i,j] -> [i] }, { S[i,j] -> [j] }]",
	},
	{ 0, 1, "[n] -> { S[i,j] : 0 <= i,j < n }",
	  "[{ S[i,j] -> [i] }, { S[i,j] -> [j] }]",
	  "{ [32,32] }",
	  "[{ S[i,j] -> [floor(i/32)] }, { S[i,j] -> [floor(j/32)] }]",
	  "[{ S[i,j] -> [i%32] }, { S[i,j] -> [j%32] }]",
	},
	{ 1, 1, "[n] -> { S[i,j] : 0 <= i,j < n }",
	  "[{ S[i,j] -> [i] }, { S[i,j] -> [j] }]",
	  "{ [32,32] }",
	  "[{ S[i,j] -> [32*floor(i/32)] }, { S[i,j] -> [32*floor(j/32)] }]",
	  "[{ S[i,j] -> [i%32] }, { S[i,j] -> [j%32] }]",
	},
};

int test_tile(Ctx &C)
{
	int scale = C.getTileScaleTileLoops();
	int shift = C.getTileShiftPointLoops();

	for (int i = 0; i < ARRAY_SIZE(tile_tests); ++i) {
		C.setTileScaleTileLoops(tile_tests[i].scale_tile);
		C.setTileShiftPointLoops(tile_tests[i].shift_point);

		UnionSet Domain =
		    UnionSet::readFromStr(C, tile_tests[i].domain);
		ScheduleNode Node = ScheduleNode::fromDomain(Domain);
		Node = Node.child(0);
		MultiUnionPwAff Mupa =
		    MultiUnionPwAff::readFromStr(C, tile_tests[i].schedule);
		Node = Node.insertPartialSchedule(Mupa);
		Val Sizes = Val::readFromStr(C, tile_tests[i].sizes);
		Node = Node.bandTile(Sizes);

		Mupa = MultiUnionPwAff::readFromStr(C, tile_tests[i].tile);
		MultiUnionPwAff Mupa2 = Node.bandGetPartialSchedule();
		Bool equal = Mupa.plainIsEqual(Mupa2);

		Node = Node.child(0);
		Mupa = MultiUnionPwAff::readFromStr(C, tile_tests[i].point);
		Mupa2 = Node.bandGetPartialSchedule();

		switch (equal) {
		case Bool::True:
			equal = Mupa.plainIsEqual(Mupa2);
		case Bool::Error:
			return -1;
		case Bool::False:
			isl_die(C.Get(), isl_error_unknown, "unexpected result",
				return -1);
		}
	}

	C.setTileScaleTileLoops(scale);
	C.setTileShiftPointLoops(shift);

	return 0;
}

struct {
	const char *set;
	const char *dual;
} coef_tests[] = {
	{ "{ rat: [i] : 0 <= i <= 10 }",
	  "{ rat: coefficients[[cst] -> [a]] : cst >= 0 and 10a + cst >= 0 }" },
	{ "{ rat: [i] : FALSE }",
	  "{ rat: coefficients[[cst] -> [a]] }" },
	{ "{ rat: [i] : }",
	  "{ rat: coefficients[[cst] -> [0]] : cst >= 0 }" },
};

struct {
	const char *set;
	const char *dual;
} sol_tests[] = {
	{ "{ rat: coefficients[[cst] -> [a]] : cst >= 0 and 10a + cst >= 0 }",
	  "{ rat: [i] : 0 <= i <= 10 }" },
	{ "{ rat: coefficients[[cst] -> [a]] : FALSE }",
	  "{ rat: [i] }" },
	{ "{ rat: coefficients[[cst] -> [a]] }",
	  "{ rat: [i] : FALSE }" },
};

/* Test the basic functionality of isl_basic_set_coefficients and
 * isl_basic_set_solutions.
 */
static int test_dual(Ctx &C)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(coef_tests); ++i) {
		Bool equal;

		BasicSet bset1 = BasicSet::readFromStr(C, coef_tests[i].set);
		BasicSet bset2 = BasicSet::readFromStr(C, coef_tests[i].dual);
		bset1 = bset1.coefficients();
		equal = bset1.isEqual(bset2);
		if (equal == Bool::Error)
			return -1;
		if (!(equal == Bool::True))
			isl_die(C.Get(), isl_error_unknown,
				"incorrect dual", return -1);
	}

	for (i = 0; i < ARRAY_SIZE(sol_tests); ++i) {
		Bool equal;

		BasicSet bset1 = BasicSet::readFromStr(C, sol_tests[i].set);
		BasicSet bset2 = BasicSet::readFromStr(C, sol_tests[i].dual);
		bset1 = bset1.solutions();
		equal = bset1.isEqual(bset2);
		if (equal == Bool::Error)
			return -1;
		if (!(equal == Bool::True))
			isl_die(C.Get(), isl_error_unknown,
				"incorrect dual", return -1);
	}

	return 0;
}

static int test_union(Ctx &C)
{
	Bool equal;

	UnionSet uset1 = UnionSet::readFromStr(C, "{ [i] : 0 <= i <= 1 }");
	UnionMap umap1 = UnionMap::readFromStr(C, "{ [1] -> [0] }");
	UnionMap umap2 = uset1.lexGtUnionSet(uset1);
	equal = umap1.isEqual(umap2);

	switch (equal) {
	case Bool::Error:
		return -1;
	case Bool::False:
		isl_die(C.Get(), isl_error_unknown, "union maps not equal",
			return -1);
	default:
		break;
	}

	umap1 = UnionMap::readFromStr(
	    C, "{ A[i] -> B[i]; B[i] -> C[i]; A[0] -> C[1] }");
	uset1 = UnionSet::readFromStr(C, "{ A[i]; B[i] }");
	UnionSet uset2 = umap1.domain();
	equal = uset1.isEqual(uset2);

	switch (equal) {
	case Bool::Error:
		return -1;
	case Bool::False:
		isl_die(C.Get(), isl_error_unknown, "union maps not equal",
			return -1);
	default:
		break;
	}
	return 0;
}

/* Check that the dependence analysis proceeds without errors.
 * Earlier versions of isl would break down during the analysis
 * due to the use of the wrong spaces.
 */
static int test_flow(Ctx &C)
{
	using UnionMapPtr = std::unique_ptr<UnionMap>;
	UnionMapPtr must_dep(nullptr);
	UnionMapPtr may_dep(nullptr);
	int r;

	UnionMap access = UnionMap::readFromStr(C,
		"{ S0[j] -> i[]; S1[j,i] -> i[]; S2[] -> i[]; S3[] -> i[] }");
	UnionMap schedule = UnionMap::readFromStr(C,
		"{ S0[j] -> [0,j,0,0] : 0 <= j < 10; "
		  "S1[j,i] -> [0,j,1,i] : 0 <= j < i < 10; "
		  "S2[] -> [1,0,0,0]; "
		  "S3[] -> [-1,0,0,0] }");
	r = access.computeFlow(access, access, schedule, &must_dep, &may_dep,
			       NULL, NULL);
	return r;
}

/* Check that the variable compression performed on the existentially
 * quantified variables inside isl_basic_set_compute_divs is not confused
 * by the implicit equalities among the parameters.
 */
static int test_compute_divs(Ctx &C)
{
	BasicSet bset = BasicSet::readFromStr(C,
		"[a, b, c, d, e] -> { [] : exists (e0: 2d = b and a <= 124 and "
		"b <= 2046 and b >= 0 and b <= 60 + 64a and 2e >= b + 2c and "
		"2e >= b and 2e <= 1 + b and 2e <= 1 + b + 2c and "
		"32768e0 >= -124 + a and 2097152e0 <= 60 + 64a - b) }");
	Set set = bset.computeDivs();
	return 0;
}

/* This is a regression test for a bug where isl_tab_basic_map_partial_lexopt
 * with gbr context would fail to disable the use of the shifted tableau
 * when transferring equalities for the input to the context, resulting
 * in invalid sample values.
 */
static int test_partial_lexmin(Ctx &C)
{
	BasicMap bmap = BasicMap::readFromStr(
	    C, "{ [1, b, c, 1 - c] -> [e] : 2e <= -c and 2e >= -3 + c }");
	BasicSet bset = BasicSet::readFromStr(
	    C, "{ [a, b, c, d] : c <= 1 and 2d >= 6 - 4b - c }");


	// Not exported yet.
	isl_map *map;
	map = isl_basic_map_partial_lexmin(bmap.Give(), bset.Give(), NULL);
	isl_map_free(map);

	if (!map)
		return -1;

	return 0;
}

/* This is a regression test for a bug where isl_basic_map_simplify
 * would end up in an infinite loop.  In particular, we construct
 * an empty basic set that is not obviously empty.
 * isl_basic_set_is_empty marks the basic set as empty.
 * After projecting out i3, the variable can be dropped completely,
 * but isl_basic_map_simplify refrains from doing so if the basic set
 * is empty and would end up in an infinite loop if it didn't test
 * explicitly for empty basic maps in the outer loop.
 */
static int test_simplify(Ctx &C)
{
	Bool empty;

	BasicSet bset = BasicSet::readFromStr(C,
	    "{ [i0, i1, i2, i3] : i0 >= -2 and 6i2 <= 4 + i0 + 5i1 and "
	    "i2 <= 22 and 75i2 <= 111 + 13i0 + 60i1 and "
	    "25i2 >= 38 + 6i0 + 20i1 and i0 <= -1 and i2 >= 20 and "
	    "i3 >= i2 }");
	empty = bset.isEmpty();
	bset = bset.projectOut(DimType::Set, 3, 1);
	if (!bset.Give())
		return -1;
	if (empty == Bool::False)
		isl_die(C.Get(), isl_error_unknown,
			"basic set should be empty", return -1);

	return 0;
}

/* Check that isl_basic_map_curry does not modify input.
 */
static int test_curry(Ctx &C)
{
	Bool equal;

	BasicMap bmap1 = BasicMap::readFromStr(C, "{ [A[] -> B[]] -> C[] }");
	BasicMap bmap2 = bmap1.curry();
	equal = bmap1.isEqual(bmap2);

	switch (equal) {
	case Bool::Error:
		return -1;
	case Bool::True:
		isl_die(C.Get(), isl_error_unknown,
			"curried map should not be equal to original",
			return -1);
	default:
		return 0;
	}
}

/* Check if dropping output dimensions from an isl_pw_multi_aff
 * works properly.
 */
static int test_pw_multi_aff(Ctx &C)
{
	const char *str;
	isl_pw_multi_aff *pma1, *pma2;
	Bool equal;

	str = "{ [i,j] -> [i+j, 4i-j] }";
	pma1 = isl_pw_multi_aff_read_from_str(C.Get(), str);
	str = "{ [i,j] -> [4i-j] }";
	pma2 = isl_pw_multi_aff_read_from_str(C.Get(), str);

	PwMultiAff PMA1 = PwMultiAff(C, pma1);
	PwMultiAff PMA2 = PwMultiAff(C, pma2);

	PMA1 = PMA1.dropDims(DimType::Out, 0, 1);
	equal = PMA1.plainIsEqual(PMA2);

	switch (equal) {
	case Bool::Error:
		return -1;
	case Bool::False:
		isl_die(C.Get(), isl_error_unknown, "expressions not equal",
			return -1);
	default:
		return 0;
	}
}

/* Check that we can properly parse multi piecewise affine expressions
 * where the piecewise affine expressions have different domains.
 */
static int test_multi_pw_aff(Ctx &C)
{
	Bool equal;
	Bool equal_domain;

	MultiPwAff MPA1 = MultiPwAff(
	    C, isl_multi_pw_aff_read_from_str(C.Get(), "{ [i] -> [i] }"));
	Set dom = Set::readFromStr(C, "{ [i] : i > 0 }");
	MPA1 = MultiPwAff(
	    C, isl_multi_pw_aff_intersect_domain(MPA1.Give(), dom.Give()));
	MultiPwAff MPA2 = MultiPwAff(
	    C, isl_multi_pw_aff_read_from_str(C.Get(), "{ [i] -> [2i] }"));
	MPA2 = MultiPwAff(
	    C, isl_multi_pw_aff_flat_range_product(MPA1.Give(), MPA2.Give()));
	MPA1 = MultiPwAff(C, isl_multi_pw_aff_read_from_str(
				 C.Get(), "{ [i] -> [(i : i > 0), 2i] }"));

	equal = MPA1.plainIsEqual(MPA2);

	PwAff pa = PwAff(C, isl_multi_pw_aff_get_pw_aff(MPA1.Get(), 0));
	dom = pa.domain();
	pa = PwAff(C, isl_multi_pw_aff_get_pw_aff(MPA1.Get(), 1));
	Set dom2 = pa.domain();
	equal_domain = dom.isEqual(dom2);

	switch (equal) {
	case Bool::Error:
		return -1;
	case Bool::False:
		isl_die(C.Get(), isl_error_unknown, "expressions not equal",
			return -1);
	default:
		break;
	}

	switch (equal_domain) {
	case Bool::Error:
		return -1;
	case Bool::True:
		isl_die(C.Get(), isl_error_unknown,
			"domains unexpectedly equal", return -1);
	default:
		break;
	}

	return 0;
}

const char *set_conversion_tests[] = {
	"[N] -> { [i] : N - 1 <= 2 i <= N }",
	"[N] -> { [i] : exists a : i = 4 a and N - 1 <= i <= N }",
	"[N] -> { [i,j] : exists a : i = 4 a and N - 1 <= i, 2j <= N }",
	"[N] -> { [[i]->[j]] : exists a : i = 4 a and N - 1 <= i, 2j <= N }",
	"[N] -> { [3*floor(N/2) + 5*floor(N/3)] }",
};

/* Check that converting from isl_set to isl_pw_multi_aff and back
 * to isl_set produces the original isl_set.
 */
static int test_set_conversion(Ctx &C)
{
	int i;
	Bool equal;

	for (i = 0; i < ARRAY_SIZE(set_conversion_tests); ++i) {
		Set set1 = Set::readFromStr(C, set_conversion_tests[i]);
		PwMultiAff pma =
		    PwMultiAff(C, isl_pw_multi_aff_from_set(set1.GetCopy()));
		Set set2 = Set(C, isl_set_from_pw_multi_aff(pma.Give()));
		equal = set1.isEqual(set2);

		switch (equal) {
		case Bool::Error:
			return -1;
		case Bool::False:
			isl_die(C.Get(), isl_error_unknown, "bad conversion",
				return -1);
		default:
			break;
		}
	}

	return 0;
}

/* Check that converting from isl_map to isl_pw_multi_aff and back
 * to isl_map produces the original isl_map.
 */
static int test_map_conversion(Ctx &C)
{
	Bool equal;

	Map map1 = Map::readFromStr(C, "{ [a, b, c, d] -> s0[a, b, e, f] : "
		"exists (e0 = [(a - 2c)/3], e1 = [(-4 + b - 5d)/9], "
		"e2 = [(-d + f)/9]: 3e0 = a - 2c and 9e1 = -4 + b - 5d and "
		"9e2 = -d + f and f >= 0 and f <= 8 and 9e >= -5 - 2a and "
		"9e <= -2 - 2a) }");
	PwMultiAff pma =
	    PwMultiAff(C, isl_pw_multi_aff_from_map(map1.GetCopy()));
	Map map2 = Map(C, isl_map_from_pw_multi_aff(pma.Give()));
	equal = map1.isEqual(map2);

	switch (equal) {
	case Bool::Error:
		return -1;
	case Bool::False:
		isl_die(C.Get(), isl_error_unknown, "bad conversion",
			return -1);
	default:
		break;
	}

	return 0;
}

static int test_list(Ctx &C)
{
	int ok;

	Id a = Id::alloc(C, "a", nullptr);
	Id b = Id::alloc(C, "b", nullptr);
	Id c = Id::alloc(C, "c", nullptr);
	Id d = Id::alloc(C, "d", nullptr);

	IdList list = IdList::alloc(C, 4);

	list.add(a);
	list.add(b);
	list.add(c);
	list.add(d);
	list = IdList(C, isl_id_list_drop(list.Give(), 1, 1));

	if (isl_id_list_n_id(list.Get()) != 3) {
		isl_die(C.Get(), isl_error_unknown,
			"unexpected number of elements in list", return -1);
	}

	Id id = Id(C, isl_id_list_get_id(list.Get(), 0));
	ok = id.Get() == a.Get();

	id = Id(C, isl_id_list_get_id(list.Get(), 1));
	ok = ok && id.Get()  == c.Get();

	id = Id(C, isl_id_list_get_id(list.Get(), 2));
	ok = ok && id.Get() == d.Get();

	if (!ok)
		isl_die(C.Get(), isl_error_unknown,
			"unexpected elements in list", return -1);

	return 0;
}

static int test_conversion(Ctx &C)
{
	if (test_set_conversion(C) < 0)
		return -1;
	if (test_map_conversion(C) < 0)
		return -1;
	return 0;
}

struct {
	const char *name;
	int (*fn)(isl::Ctx &C);
} tests[] = {
    {"parse", &test_parse},
    {"union", &test_union},
    {"dual", &test_dual},
    {"dependence analysis", &test_flow},
    {"partial lexmin", &test_partial_lexmin},
    {"simplify", &test_simplify},
    {"curry", &test_curry},
    {"piecewise multi affine expressions", &test_pw_multi_aff},
    {"multi piecewise affine expressions", &test_multi_pw_aff},
    {"list", &test_list},
    {"conversion", &test_conversion},
    {"compute divs", &test_compute_divs},
    //    {"tile", &test_tile},
};

int main(int argc, char **argv)
{
	int i;
        isl::Ctx C = isl::Ctx::alloc();

	const char * srcdir = getenv("srcdir");
	assert(srcdir);

	for (i = 0; i < ARRAY_SIZE(tests); ++i) {
		printf("%s\n", tests[i].name);
		if (tests[i].fn(C) < 0)
			return -1;
	}
	return 0;
}