/*
 * Copyright 2012-2014 Ecole Normale Superieure
 * Copyright 2014      INRIA Rocquencourt
 *
 * Use of this software is governed by the MIT license
 *
 * Written by Sven Verdoolaege,
 * Ecole Normale Superieure, 45 rue d’Ulm, 75230 Paris, France
 * and Inria Paris - Rocquencourt, Domaine de Voluceau - Rocquencourt,
 * B.P. 105 - 78153 Le Chesnay, France
 */

#include <limits.h>
#include <isl/aff.h>
#include <isl/constraint.h>
#include <isl/set.h>
#include <isl/ilp.h>
#include <isl/union_set.h>
#include <isl/union_map.h>
#include <isl/schedule_node.h>
#include <isl_sort.h>
#include <isl_tarjan.h>
#include <isl_ast_private.h>
#include <isl_ast_build_expr.h>
#include <isl_ast_build_private.h>
#include <isl_ast_graft_private.h>

/* Data used in generate_domain.
 *
 * "build" is the input build.
 * "list" collects the results.
 */
struct isl_generate_domain_data {
	isl_ast_build *build;

	isl_ast_graft_list *list;
};

static __isl_give isl_ast_graft_list *generate_next_level(
	__isl_take isl_union_map *executed,
	__isl_take isl_ast_build *build);
static __isl_give isl_ast_graft_list *generate_code(
	__isl_take isl_union_map *executed, __isl_take isl_ast_build *build,
	int internal);

/* Generate an AST for a single domain based on
 * the (non single valued) inverse schedule "executed".
 *
 * We extend the schedule with the iteration domain
 * and continue generating through a call to generate_code.
 *
 * In particular, if executed has the form
 *
 *	S -> D
 *
 * then we continue generating code on
 *
 *	[S -> D] -> D
 *
 * The extended inverse schedule is clearly single valued
 * ensuring that the nested generate_code will not reach this function,
 * but will instead create calls to all elements of D that need
 * to be executed from the current schedule domain.
 */
static isl_stat generate_non_single_valued(__isl_take isl_map *executed,
	struct isl_generate_domain_data *data)
{
	isl_map *identity;
	isl_ast_build *build;
	isl_ast_graft_list *list;

	build = isl_ast_build_copy(data->build);

	identity = isl_set_identity(isl_map_range(isl_map_copy(executed)));
	executed = isl_map_domain_product(executed, identity);
	build = isl_ast_build_set_single_valued(build, 1);

	list = generate_code(isl_union_map_from_map(executed), build, 1);

	data->list = isl_ast_graft_list_concat(data->list, list);

	return isl_stat_ok;
}

/* Call the at_each_domain callback, if requested by the user,
 * after recording the current inverse schedule in the build.
 */
static __isl_give isl_ast_graft *at_each_domain(__isl_take isl_ast_graft *graft,
	__isl_keep isl_map *executed, __isl_keep isl_ast_build *build)
{
	if (!graft || !build)
		return isl_ast_graft_free(graft);
	if (!build->at_each_domain)
		return graft;

	build = isl_ast_build_copy(build);
	build = isl_ast_build_set_executed(build,
			isl_union_map_from_map(isl_map_copy(executed)));
	if (!build)
		return isl_ast_graft_free(graft);

	graft->node = build->at_each_domain(graft->node,
					build, build->at_each_domain_user);
	isl_ast_build_free(build);

	if (!graft->node)
		graft = isl_ast_graft_free(graft);

	return graft;
}

/* Generate an AST for a single domain based on
 * the inverse schedule "executed" and add it to data->list.
 *
 * If there is more than one domain element associated to the current
 * schedule "time", then we need to continue the generation process
 * in generate_non_single_valued.
 * Note that the inverse schedule being single-valued may depend
 * on constraints that are only available in the original context
 * domain specified by the user.  We therefore first introduce
 * some of the constraints of data->build->domain.  In particular,
 * we intersect with a single-disjunct approximation of this set.
 * We perform this approximation to avoid further splitting up
 * the executed relation, possibly introducing a disjunctive guard
 * on the statement.
 *
 * On the other hand, we only perform the test after having taken the gist
 * of the domain as the resulting map is the one from which the call
 * expression is constructed.  Using this map to construct the call
 * expression usually yields simpler results.
 * Because we perform the single-valuedness test on the gisted map,
 * we may in rare cases fail to recognize that the inverse schedule
 * is single-valued.  This becomes problematic if this happens
 * from the recursive call through generate_non_single_valued
 * as we would then end up in an infinite recursion.
 * We therefore check if we are inside a call to generate_non_single_valued
 * and revert to the ungisted map if the gisted map turns out not to be
 * single-valued.
 *
 * Otherwise, we generate a call expression for the single executed
 * domain element and put a guard around it based on the (simplified)
 * domain of "executed".
 *
 * At this stage, any pending constraints in the build can no longer
 * be simplified with respect to any enforced constraints since
 * the call node does not have any enforced constraints.
 * We therefore turn all pending constraints into guards
 * (after simplifying them with respect to the already generated
 * constraints) and add them to both the generated constraints
 * and the guard of the constructed graft.  This guard will ensure
 * that the constraints are effectively generated.
 *
 * If the user has set an at_each_domain callback, it is called
 * on the constructed call expression node.
 */
static isl_stat generate_domain(__isl_take isl_map *executed, void *user)
{
	struct isl_generate_domain_data *data = user;
	isl_ast_build *build;
	isl_ast_graft *graft;
	isl_ast_graft_list *list;
	isl_set *guard, *domain;
	isl_map *map = NULL;
	int empty, sv;

	domain = isl_ast_build_get_domain(data->build);
	domain = isl_set_from_basic_set(isl_set_simple_hull(domain));
	executed = isl_map_intersect_domain(executed, domain);
	empty = isl_map_is_empty(executed);
	if (empty < 0)
		goto error;
	if (empty) {
		isl_map_free(executed);
		return isl_stat_ok;
	}

	executed = isl_map_coalesce(executed);
	map = isl_map_copy(executed);
	map = isl_ast_build_compute_gist_map_domain(data->build, map);
	sv = isl_map_is_single_valued(map);
	if (sv < 0)
		goto error;
	if (!sv) {
		isl_map_free(map);
		if (data->build->single_valued)
			map = isl_map_copy(executed);
		else
			return generate_non_single_valued(executed, data);
	}
	guard = isl_map_domain(isl_map_copy(map));
	guard = isl_set_compute_divs(guard);
	guard = isl_set_intersect(guard,
				    isl_ast_build_get_pending(data->build));
	guard = isl_set_coalesce(guard);
	guard = isl_ast_build_specialize(data->build, guard);
	guard = isl_set_gist(guard, isl_ast_build_get_generated(data->build));

	build = isl_ast_build_copy(data->build);
	build = isl_ast_build_replace_pending_by_guard(build,
							isl_set_copy(guard));
	graft = isl_ast_graft_alloc_domain(map, build);
	graft = at_each_domain(graft, executed, build);
	isl_ast_build_free(build);
	isl_map_free(executed);
	graft = isl_ast_graft_add_guard(graft, guard, data->build);

	list = isl_ast_graft_list_from_ast_graft(graft);
	data->list = isl_ast_graft_list_concat(data->list, list);

	return isl_stat_ok;
error:
	isl_map_free(map);
	isl_map_free(executed);
	return isl_stat_error;
}

/* Call build->create_leaf to a create "leaf" node in the AST,
 * encapsulate the result in an isl_ast_graft and return the result
 * as a 1-element list.
 *
 * Note that the node returned by the user may be an entire tree.
 *
 * Since the node itself cannot enforce any constraints, we turn
 * all pending constraints into guards and add them to the resulting
 * graft to ensure that they will be generated.
 *
 * Before we pass control to the user, we first clear some information
 * from the build that is (presumbably) only meaningful
 * for the current code generation.
 * This includes the create_leaf callback itself, so we make a copy
 * of the build first.
 */
static __isl_give isl_ast_graft_list *call_create_leaf(
	__isl_take isl_union_map *executed, __isl_take isl_ast_build *build)
{
	isl_set *guard;
	isl_ast_node *node;
	isl_ast_graft *graft;
	isl_ast_build *user_build;

	guard = isl_ast_build_get_pending(build);
	user_build = isl_ast_build_copy(build);
	user_build = isl_ast_build_replace_pending_by_guard(user_build,
							isl_set_copy(guard));
	user_build = isl_ast_build_set_executed(user_build, executed);
	user_build = isl_ast_build_clear_local_info(user_build);
	if (!user_build)
		node = NULL;
	else
		node = build->create_leaf(user_build, build->create_leaf_user);
	graft = isl_ast_graft_alloc(node, build);
	graft = isl_ast_graft_add_guard(graft, guard, build);
	isl_ast_build_free(build);
	return isl_ast_graft_list_from_ast_graft(graft);
}

static __isl_give isl_ast_graft_list *build_ast_from_child(
	__isl_take isl_ast_build *build, __isl_take isl_schedule_node *node,
	__isl_take isl_union_map *executed);

/* Generate an AST after having handled the complete schedule
 * of this call to the code generator or the complete band
 * if we are generating an AST from a schedule tree.
 *
 * If we are inside a band node, then move on to the child of the band.
 *
 * If the user has specified a create_leaf callback, control
 * is passed to the user in call_create_leaf.
 *
 * Otherwise, we generate one or more calls for each individual
 * domain in generate_domain.
 */
static __isl_give isl_ast_graft_list *generate_inner_level(
	__isl_take isl_union_map *executed, __isl_take isl_ast_build *build)
{
	isl_ctx *ctx;
	struct isl_generate_domain_data data = { build };

	if (!build || !executed)
		goto error;

	if (isl_ast_build_has_schedule_node(build)) {
		isl_schedule_node *node;
		node = isl_ast_build_get_schedule_node(build);
		build = isl_ast_build_reset_schedule_node(build);
		return build_ast_from_child(build, node, executed);
	}

	if (build->create_leaf)
		return call_create_leaf(executed, build);

	ctx = isl_union_map_get_ctx(executed);
	data.list = isl_ast_graft_list_alloc(ctx, 0);
	if (isl_union_map_foreach_map(executed, &generate_domain, &data) < 0)
		data.list = isl_ast_graft_list_free(data.list);

	if (0)
error:		data.list = NULL;
	isl_ast_build_free(build);
	isl_union_map_free(executed);
	return data.list;
}

/* Call the before_each_for callback, if requested by the user.
 */
static __isl_give isl_ast_node *before_each_for(__isl_take isl_ast_node *node,
	__isl_keep isl_ast_build *build)
{
	isl_id *id;

	if (!node || !build)
		return isl_ast_node_free(node);
	if (!build->before_each_for)
		return node;
	id = build->before_each_for(build, build->before_each_for_user);
	node = isl_ast_node_set_annotation(node, id);
	return node;
}

/* Call the after_each_for callback, if requested by the user.
 */
static __isl_give isl_ast_graft *after_each_for(__isl_take isl_ast_graft *graft,
	__isl_keep isl_ast_build *build)
{
	if (!graft || !build)
		return isl_ast_graft_free(graft);
	if (!build->after_each_for)
		return graft;
	graft->node = build->after_each_for(graft->node, build,
						build->after_each_for_user);
	if (!graft->node)
		return isl_ast_graft_free(graft);
	return graft;
}

/* Plug in all the know values of the current and outer dimensions
 * in the domain of "executed".  In principle, we only need to plug
 * in the known value of the current dimension since the values of
 * outer dimensions have been plugged in already.
 * However, it turns out to be easier to just plug in all known values.
 */
static __isl_give isl_union_map *plug_in_values(
	__isl_take isl_union_map *executed, __isl_keep isl_ast_build *build)
{
	return isl_ast_build_substitute_values_union_map_domain(build,
								    executed);
}

/* Check if the constraint "c" is a lower bound on dimension "pos",
 * an upper bound, or independent of dimension "pos".
 */
static int constraint_type(isl_constraint *c, int pos)
{
	if (isl_constraint_is_lower_bound(c, isl_dim_set, pos))
		return 1;
	if (isl_constraint_is_upper_bound(c, isl_dim_set, pos))
		return 2;
	return 0;
}

/* Compare the types of the constraints "a" and "b",
 * resulting in constraints that are independent of "depth"
 * to be sorted before the lower bounds on "depth", which in
 * turn are sorted before the upper bounds on "depth".
 */
static int cmp_constraint(__isl_keep isl_constraint *a,
	__isl_keep isl_constraint *b, void *user)
{
	int *depth = user;
	int t1 = constraint_type(a, *depth);
	int t2 = constraint_type(b, *depth);

	return t1 - t2;
}

/* Extract a lower bound on dimension "pos" from constraint "c".
 *
 * If the constraint is of the form
 *
 *	a x + f(...) >= 0
 *
 * then we essentially return
 *
 *	l = ceil(-f(...)/a)
 *
 * However, if the current dimension is strided, then we need to make
 * sure that the lower bound we construct is of the form
 *
 *	f + s a
 *
 * with f the offset and s the stride.
 * We therefore compute
 *
 *	f + s * ceil((l - f)/s)
 */
static __isl_give isl_aff *lower_bound(__isl_keep isl_constraint *c,
	int pos, __isl_keep isl_ast_build *build)
{
	isl_aff *aff;

	aff = isl_constraint_get_bound(c, isl_dim_set, pos);
	aff = isl_aff_ceil(aff);

	if (isl_ast_build_has_stride(build, pos)) {
		isl_aff *offset;
		isl_val *stride;

		offset = isl_ast_build_get_offset(build, pos);
		stride = isl_ast_build_get_stride(build, pos);

		aff = isl_aff_sub(aff, isl_aff_copy(offset));
		aff = isl_aff_scale_down_val(aff, isl_val_copy(stride));
		aff = isl_aff_ceil(aff);
		aff = isl_aff_scale_val(aff, stride);
		aff = isl_aff_add(aff, offset);
	}

	aff = isl_ast_build_compute_gist_aff(build, aff);

	return aff;
}

/* Return the exact lower bound (or upper bound if "upper" is set)
 * of "domain" as a piecewise affine expression.
 *
 * If we are computing a lower bound (of a strided dimension), then
 * we need to make sure it is of the form
 *
 *	f + s a
 *
 * where f is the offset and s is the stride.
 * We therefore need to include the stride constraint before computing
 * the minimum.
 */
static __isl_give isl_pw_aff *exact_bound(__isl_keep isl_set *domain,
	__isl_keep isl_ast_build *build, int upper)
{
	isl_set *stride;
	isl_map *it_map;
	isl_pw_aff *pa;
	isl_pw_multi_aff *pma;

	domain = isl_set_copy(domain);
	if (!upper) {
		stride = isl_ast_build_get_stride_constraint(build);
		domain = isl_set_intersect(domain, stride);
	}
	it_map = isl_ast_build_map_to_iterator(build, domain);
	if (upper)
		pma = isl_map_lexmax_pw_multi_aff(it_map);
	else
		pma = isl_map_lexmin_pw_multi_aff(it_map);
	pa = isl_pw_multi_aff_get_pw_aff(pma, 0);
	isl_pw_multi_aff_free(pma);
	pa = isl_ast_build_compute_gist_pw_aff(build, pa);
	pa = isl_pw_aff_coalesce(pa);

	return pa;
}

/* Callback for sorting the isl_pw_aff_list passed to reduce_list and
 * remove_redundant_lower_bounds.
 */
static int reduce_list_cmp(__isl_keep isl_pw_aff *a, __isl_keep isl_pw_aff *b,
	void *user)
{
	return isl_pw_aff_plain_cmp(a, b);
}

/* Given a list of lower bounds "list", remove those that are redundant
 * with respect to the other bounds in "list" and the domain of "build".
 *
 * We first sort the bounds in the same way as they would be sorted
 * by set_for_node_expressions so that we can try and remove the last
 * bounds first.
 *
 * For a lower bound to be effective, there needs to be at least
 * one domain element for which it is larger than all other lower bounds.
 * For each lower bound we therefore intersect the domain with
 * the conditions that it is larger than all other bounds and
 * check whether the result is empty.  If so, the bound can be removed.
 */
static __isl_give isl_pw_aff_list *remove_redundant_lower_bounds(
	__isl_take isl_pw_aff_list *list, __isl_keep isl_ast_build *build)
{
	int i, j, n;
	isl_set *domain;

	list = isl_pw_aff_list_sort(list, &reduce_list_cmp, NULL);
	if (!list)
		return NULL;

	n = isl_pw_aff_list_n_pw_aff(list);
	if (n <= 1)
		return list;

	domain = isl_ast_build_get_domain(build);

	for (i = n - 1; i >= 0; --i) {
		isl_pw_aff *pa_i;
		isl_set *domain_i;
		int empty;

		domain_i = isl_set_copy(domain);
		pa_i = isl_pw_aff_list_get_pw_aff(list, i);

		for (j = 0; j < n; ++j) {
			isl_pw_aff *pa_j;
			isl_set *better;

			if (j == i)
				continue;

			pa_j = isl_pw_aff_list_get_pw_aff(list, j);
			better = isl_pw_aff_gt_set(isl_pw_aff_copy(pa_i), pa_j);
			domain_i = isl_set_intersect(domain_i, better);
		}

		empty = isl_set_is_empty(domain_i);

		isl_set_free(domain_i);
		isl_pw_aff_free(pa_i);

		if (empty < 0)
			goto error;
		if (!empty)
			continue;
		list = isl_pw_aff_list_drop(list, i, 1);
		n--;
	}

	isl_set_free(domain);

	return list;
error:
	isl_set_free(domain);
	return isl_pw_aff_list_free(list);
}

/* Extract a lower bound on dimension "pos" from each constraint
 * in "constraints" and return the list of lower bounds.
 * If "constraints" has zero elements, then we extract a lower bound
 * from "domain" instead.
 *
 * If the current dimension is strided, then the lower bound
 * is adjusted by lower_bound to match the stride information.
 * This modification may make one or more lower bounds redundant
 * with respect to the other lower bounds.  We therefore check
 * for this condition and remove the redundant lower bounds.
 */
static __isl_give isl_pw_aff_list *lower_bounds(
	__isl_keep isl_constraint_list *constraints, int pos,
	__isl_keep isl_set *domain, __isl_keep isl_ast_build *build)
{
	isl_ctx *ctx;
	isl_pw_aff_list *list;
	int i, n;

	if (!build)
		return NULL;

	n = isl_constraint_list_n_constraint(constraints);
	if (n == 0) {
		isl_pw_aff *pa;
		pa = exact_bound(domain, build, 0);
		return isl_pw_aff_list_from_pw_aff(pa);
	}

	ctx = isl_ast_build_get_ctx(build);
	list = isl_pw_aff_list_alloc(ctx,n);

	for (i = 0; i < n; ++i) {
		isl_aff *aff;
		isl_constraint *c;

		c = isl_constraint_list_get_constraint(constraints, i);
		aff = lower_bound(c, pos, build);
		isl_constraint_free(c);
		list = isl_pw_aff_list_add(list, isl_pw_aff_from_aff(aff));
	}

	if (isl_ast_build_has_stride(build, pos))
		list = remove_redundant_lower_bounds(list, build);

	return list;
}

/* Extract an upper bound on dimension "pos" from each constraint
 * in "constraints" and return the list of upper bounds.
 * If "constraints" has zero elements, then we extract an upper bound
 * from "domain" instead.
 */
static __isl_give isl_pw_aff_list *upper_bounds(
	__isl_keep isl_constraint_list *constraints, int pos,
	__isl_keep isl_set *domain, __isl_keep isl_ast_build *build)
{
	isl_ctx *ctx;
	isl_pw_aff_list *list;
	int i, n;

	n = isl_constraint_list_n_constraint(constraints);
	if (n == 0) {
		isl_pw_aff *pa;
		pa = exact_bound(domain, build, 1);
		return isl_pw_aff_list_from_pw_aff(pa);
	}

	ctx = isl_ast_build_get_ctx(build);
	list = isl_pw_aff_list_alloc(ctx,n);

	for (i = 0; i < n; ++i) {
		isl_aff *aff;
		isl_constraint *c;

		c = isl_constraint_list_get_constraint(constraints, i);
		aff = isl_constraint_get_bound(c, isl_dim_set, pos);
		isl_constraint_free(c);
		aff = isl_aff_floor(aff);
		list = isl_pw_aff_list_add(list, isl_pw_aff_from_aff(aff));
	}

	return list;
}

/* Return an isl_ast_expr that performs the reduction of type "type"
 * on AST expressions corresponding to the elements in "list".
 *
 * The list is assumed to contain at least one element.
 * If the list contains exactly one element, then the returned isl_ast_expr
 * simply computes that affine expression.
 * If the list contains more than one element, then we sort it
 * using a fairly abitrary but hopefully reasonably stable order.
 */
static __isl_give isl_ast_expr *reduce_list(enum isl_ast_op_type type,
	__isl_keep isl_pw_aff_list *list, __isl_keep isl_ast_build *build)
{
	int i, n;
	isl_ctx *ctx;
	isl_ast_expr *expr;

	if (!list)
		return NULL;

	n = isl_pw_aff_list_n_pw_aff(list);

	if (n == 1)
		return isl_ast_build_expr_from_pw_aff_internal(build,
				isl_pw_aff_list_get_pw_aff(list, 0));

	ctx = isl_pw_aff_list_get_ctx(list);
	expr = isl_ast_expr_alloc_op(ctx, type, n);
	if (!expr)
		return NULL;

	list = isl_pw_aff_list_copy(list);
	list = isl_pw_aff_list_sort(list, &reduce_list_cmp, NULL);
	if (!list)
		return isl_ast_expr_free(expr);

	for (i = 0; i < n; ++i) {
		isl_ast_expr *expr_i;

		expr_i = isl_ast_build_expr_from_pw_aff_internal(build,
				isl_pw_aff_list_get_pw_aff(list, i));
		if (!expr_i)
			goto error;
		expr->u.op.args[i] = expr_i;
	}

	isl_pw_aff_list_free(list);
	return expr;
error:
	isl_pw_aff_list_free(list);
	isl_ast_expr_free(expr);
	return NULL;
}

/* Add guards implied by the "generated constraints",
 * but not (necessarily) enforced by the generated AST to "guard".
 * In particular, if there is any stride constraints,
 * then add the guard implied by those constraints.
 * If we have generated a degenerate loop, then add the guard
 * implied by "bounds" on the outer dimensions, i.e., the guard
 * that ensures that the single value actually exists.
 * Since there may also be guards implied by a combination
 * of these constraints, we first combine them before
 * deriving the implied constraints.
 */
static __isl_give isl_set *add_implied_guards(__isl_take isl_set *guard,
	int degenerate, __isl_keep isl_basic_set *bounds,
	__isl_keep isl_ast_build *build)
{
	int depth, has_stride;
	isl_space *space;
	isl_set *dom, *set;

	depth = isl_ast_build_get_depth(build);
	has_stride = isl_ast_build_has_stride(build, depth);
	if (!has_stride && !degenerate)
		return guard;

	space = isl_basic_set_get_space(bounds);
	dom = isl_set_universe(space);

	if (degenerate) {
		bounds = isl_basic_set_copy(bounds);
		bounds = isl_basic_set_drop_constraints_not_involving_dims(
					bounds, isl_dim_set, depth, 1);
		set = isl_set_from_basic_set(bounds);
		dom = isl_set_intersect(dom, set);
	}

	if (has_stride) {
		set = isl_ast_build_get_stride_constraint(build);
		dom = isl_set_intersect(dom, set);
	}

	dom = isl_set_eliminate(dom, isl_dim_set, depth, 1);
	dom = isl_ast_build_compute_gist(build, dom);
	guard = isl_set_intersect(guard, dom);

	return guard;
}

/* Update "graft" based on "sub_build" for the degenerate case.
 *
 * "build" is the build in which graft->node was created
 * "sub_build" contains information about the current level itself,
 * including the single value attained.
 *
 * We set the initialization part of the for loop to the single
 * value attained by the current dimension.
 * The increment and condition are not strictly needed as the are known
 * to be "1" and "iterator <= value" respectively.
 */
static __isl_give isl_ast_graft *refine_degenerate(
	__isl_take isl_ast_graft *graft, __isl_keep isl_ast_build *build,
	__isl_keep isl_ast_build *sub_build)
{
	isl_pw_aff *value;

	if (!graft || !sub_build)
		return isl_ast_graft_free(graft);

	value = isl_pw_aff_copy(sub_build->value);

	graft->node->u.f.init = isl_ast_build_expr_from_pw_aff_internal(build,
						value);
	if (!graft->node->u.f.init)
		return isl_ast_graft_free(graft);

	return graft;
}

/* Return the intersection of constraints in "list" as a set.
 */
static __isl_give isl_set *intersect_constraints(
	__isl_keep isl_constraint_list *list)
{
	int i, n;
	isl_basic_set *bset;

	n = isl_constraint_list_n_constraint(list);
	if (n < 1)
		isl_die(isl_constraint_list_get_ctx(list), isl_error_internal,
			"expecting at least one constraint", return NULL);

	bset = isl_basic_set_from_constraint(
				isl_constraint_list_get_constraint(list, 0));
	for (i = 1; i < n; ++i) {
		isl_basic_set *bset_i;

		bset_i = isl_basic_set_from_constraint(
				isl_constraint_list_get_constraint(list, i));
		bset = isl_basic_set_intersect(bset, bset_i);
	}

	return isl_set_from_basic_set(bset);
}

/* Compute the constraints on the outer dimensions enforced by
 * graft->node and add those constraints to graft->enforced,
 * in case the upper bound is expressed as a set "upper".
 *
 * In particular, if l(...) is a lower bound in "lower", and
 *
 *	-a i + f(...) >= 0		or	a i <= f(...)
 *
 * is an upper bound ocnstraint on the current dimension i,
 * then the for loop enforces the constraint
 *
 *	-a l(...) + f(...) >= 0		or	a l(...) <= f(...)
 *
 * We therefore simply take each lower bound in turn, plug it into
 * the upper bounds and compute the intersection over all lower bounds.
 *
 * If a lower bound is a rational expression, then
 * isl_basic_set_preimage_multi_aff will force this rational
 * expression to have only integer values.  However, the loop
 * itself does not enforce this integrality constraint.  We therefore
 * use the ceil of the lower bounds instead of the lower bounds themselves.
 * Other constraints will make sure that the for loop is only executed
 * when each of the lower bounds attains an integral value.
 * In particular, potentially rational values only occur in
 * lower_bound if the offset is a (seemingly) rational expression,
 * but then outer conditions will make sure that this rational expression
 * only attains integer values.
 */
static __isl_give isl_ast_graft *set_enforced_from_set(
	__isl_take isl_ast_graft *graft,
	__isl_keep isl_pw_aff_list *lower, int pos, __isl_keep isl_set *upper)
{
	isl_space *space;
	isl_basic_set *enforced;
	isl_pw_multi_aff *pma;
	int i, n;

	if (!graft || !lower)
		return isl_ast_graft_free(graft);

	space = isl_set_get_space(upper);
	enforced = isl_basic_set_universe(isl_space_copy(space));

	space = isl_space_map_from_set(space);
	pma = isl_pw_multi_aff_identity(space);

	n = isl_pw_aff_list_n_pw_aff(lower);
	for (i = 0; i < n; ++i) {
		isl_pw_aff *pa;
		isl_set *enforced_i;
		isl_basic_set *hull;
		isl_pw_multi_aff *pma_i;

		pa = isl_pw_aff_list_get_pw_aff(lower, i);
		pa = isl_pw_aff_ceil(pa);
		pma_i = isl_pw_multi_aff_copy(pma);
		pma_i = isl_pw_multi_aff_set_pw_aff(pma_i, pos, pa);
		enforced_i = isl_set_copy(upper);
		enforced_i = isl_set_preimage_pw_multi_aff(enforced_i, pma_i);
		hull = isl_set_simple_hull(enforced_i);
		enforced = isl_basic_set_intersect(enforced, hull);
	}

	isl_pw_multi_aff_free(pma);

	graft = isl_ast_graft_enforce(graft, enforced);

	return graft;
}

/* Compute the constraints on the outer dimensions enforced by
 * graft->node and add those constraints to graft->enforced,
 * in case the upper bound is expressed as
 * a list of affine expressions "upper".
 *
 * The enforced condition is that each lower bound expression is less
 * than or equal to each upper bound expression.
 */
static __isl_give isl_ast_graft *set_enforced_from_list(
	__isl_take isl_ast_graft *graft,
	__isl_keep isl_pw_aff_list *lower, __isl_keep isl_pw_aff_list *upper)
{
	isl_set *cond;
	isl_basic_set *enforced;

	lower = isl_pw_aff_list_copy(lower);
	upper = isl_pw_aff_list_copy(upper);
	cond = isl_pw_aff_list_le_set(lower, upper);
	enforced = isl_set_simple_hull(cond);
	graft = isl_ast_graft_enforce(graft, enforced);

	return graft;
}

/* Does "aff" have a negative constant term?
 */
static isl_stat aff_constant_is_negative(__isl_take isl_set *set,
	__isl_take isl_aff *aff, void *user)
{
	int *neg = user;
	isl_val *v;

	v = isl_aff_get_constant_val(aff);
	*neg = isl_val_is_neg(v);
	isl_val_free(v);
	isl_set_free(set);
	isl_aff_free(aff);

	return *neg ? isl_stat_ok : isl_stat_error;
}

/* Does "pa" have a negative constant term over its entire domain?
 */
static isl_stat pw_aff_constant_is_negative(__isl_take isl_pw_aff *pa,
	void *user)
{
	isl_stat r;
	int *neg = user;

	r = isl_pw_aff_foreach_piece(pa, &aff_constant_is_negative, user);
	isl_pw_aff_free(pa);

	return (*neg && r >= 0) ? isl_stat_ok : isl_stat_error;
}

/* Does each element in "list" have a negative constant term?
 *
 * The callback terminates the iteration as soon an element has been
 * found that does not have a negative constant term.
 */
static int list_constant_is_negative(__isl_keep isl_pw_aff_list *list)
{
	int neg = 1;

	if (isl_pw_aff_list_foreach(list,
				&pw_aff_constant_is_negative, &neg) < 0 && neg)
		return -1;

	return neg;
}

/* Add 1 to each of the elements in "list", where each of these elements
 * is defined over the internal schedule space of "build".
 */
static __isl_give isl_pw_aff_list *list_add_one(
	__isl_take isl_pw_aff_list *list, __isl_keep isl_ast_build *build)
{
	int i, n;
	isl_space *space;
	isl_aff *aff;
	isl_pw_aff *one;

	space = isl_ast_build_get_space(build, 1);
	aff = isl_aff_zero_on_domain(isl_local_space_from_space(space));
	aff = isl_aff_add_constant_si(aff, 1);
	one = isl_pw_aff_from_aff(aff);

	n = isl_pw_aff_list_n_pw_aff(list);
	for (i = 0; i < n; ++i) {
		isl_pw_aff *pa;
		pa = isl_pw_aff_list_get_pw_aff(list, i);
		pa = isl_pw_aff_add(pa, isl_pw_aff_copy(one));
		list = isl_pw_aff_list_set_pw_aff(list, i, pa);
	}

	isl_pw_aff_free(one);

	return list;
}

/* Set the condition part of the for node graft->node in case
 * the upper bound is represented as a list of piecewise affine expressions.
 *
 * In particular, set the condition to
 *
 *	iterator <= min(list of upper bounds)
 *
 * If each of the upper bounds has a negative constant term, then
 * set the condition to
 *
 *	iterator < min(list of (upper bound + 1)s)
 *
 */
static __isl_give isl_ast_graft *set_for_cond_from_list(
	__isl_take isl_ast_graft *graft, __isl_keep isl_pw_aff_list *list,
	__isl_keep isl_ast_build *build)
{
	int neg;
	isl_ast_expr *bound, *iterator, *cond;
	enum isl_ast_op_type type = isl_ast_op_le;

	if (!graft || !list)
		return isl_ast_graft_free(graft);

	neg = list_constant_is_negative(list);
	if (neg < 0)
		return isl_ast_graft_free(graft);
	list = isl_pw_aff_list_copy(list);
	if (neg) {
		list = list_add_one(list, build);
		type = isl_ast_op_lt;
	}

	bound = reduce_list(isl_ast_op_min, list, build);
	iterator = isl_ast_expr_copy(graft->node->u.f.iterator);
	cond = isl_ast_expr_alloc_binary(type, iterator, bound);
	graft->node->u.f.cond = cond;

	isl_pw_aff_list_free(list);
	if (!graft->node->u.f.cond)
		return isl_ast_graft_free(graft);
	return graft;
}

/* Set the condition part of the for node graft->node in case
 * the upper bound is represented as a set.
 */
static __isl_give isl_ast_graft *set_for_cond_from_set(
	__isl_take isl_ast_graft *graft, __isl_keep isl_set *set,
	__isl_keep isl_ast_build *build)
{
	isl_ast_expr *cond;

	if (!graft)
		return NULL;

	cond = isl_ast_build_expr_from_set_internal(build, isl_set_copy(set));
	graft->node->u.f.cond = cond;
	if (!graft->node->u.f.cond)
		return isl_ast_graft_free(graft);
	return graft;
}

/* Construct an isl_ast_expr for the increment (i.e., stride) of
 * the current dimension.
 */
static __isl_give isl_ast_expr *for_inc(__isl_keep isl_ast_build *build)
{
	int depth;
	isl_val *v;
	isl_ctx *ctx;

	if (!build)
		return NULL;
	ctx = isl_ast_build_get_ctx(build);
	depth = isl_ast_build_get_depth(build);

	if (!isl_ast_build_has_stride(build, depth))
		return isl_ast_expr_alloc_int_si(ctx, 1);

	v = isl_ast_build_get_stride(build, depth);
	return isl_ast_expr_from_val(v);
}

/* Should we express the loop condition as
 *
 *	iterator <= min(list of upper bounds)
 *
 * or as a conjunction of constraints?
 *
 * The first is constructed from a list of upper bounds.
 * The second is constructed from a set.
 *
 * If there are no upper bounds in "constraints", then this could mean
 * that "domain" simply doesn't have an upper bound or that we didn't
 * pick any upper bound.  In the first case, we want to generate the
 * loop condition as a(n empty) conjunction of constraints
 * In the second case, we will compute
 * a single upper bound from "domain" and so we use the list form.
 *
 * If there are upper bounds in "constraints",
 * then we use the list form iff the atomic_upper_bound option is set.
 */
static int use_upper_bound_list(isl_ctx *ctx, int n_upper,
	__isl_keep isl_set *domain, int depth)
{
	if (n_upper > 0)
		return isl_options_get_ast_build_atomic_upper_bound(ctx);
	else
		return isl_set_dim_has_upper_bound(domain, isl_dim_set, depth);
}

/* Fill in the expressions of the for node in graft->node.
 *
 * In particular,
 * - set the initialization part of the loop to the maximum of the lower bounds
 * - extract the increment from the stride of the current dimension
 * - construct the for condition either based on a list of upper bounds
 *	or on a set of upper bound constraints.
 */
static __isl_give isl_ast_graft *set_for_node_expressions(
	__isl_take isl_ast_graft *graft, __isl_keep isl_pw_aff_list *lower,
	int use_list, __isl_keep isl_pw_aff_list *upper_list,
	__isl_keep isl_set *upper_set, __isl_keep isl_ast_build *build)
{
	isl_ast_node *node;

	if (!graft)
		return NULL;

	build = isl_ast_build_copy(build);

	node = graft->node;
	node->u.f.init = reduce_list(isl_ast_op_max, lower, build);
	node->u.f.inc = for_inc(build);

	if (use_list)
		graft = set_for_cond_from_list(graft, upper_list, build);
	else
		graft = set_for_cond_from_set(graft, upper_set, build);

	isl_ast_build_free(build);

	if (!node->u.f.iterator || !node->u.f.init ||
	    !node->u.f.cond || !node->u.f.inc)
		return isl_ast_graft_free(graft);

	return graft;
}

/* Update "graft" based on "bounds" and "domain" for the generic,
 * non-degenerate, case.
 *
 * "c_lower" and "c_upper" contain the lower and upper bounds
 * that the loop node should express.
 * "domain" is the subset of the intersection of the constraints
 * for which some code is executed.
 *
 * There may be zero lower bounds or zero upper bounds in "constraints"
 * in case the list of constraints was created
 * based on the atomic option or based on separation with explicit bounds.
 * In that case, we use "domain" to derive lower and/or upper bounds.
 *
 * We first compute a list of one or more lower bounds.
 *
 * Then we decide if we want to express the condition as
 *
 *	iterator <= min(list of upper bounds)
 *
 * or as a conjunction of constraints.
 *
 * The set of enforced constraints is then computed either based on
 * a list of upper bounds or on a set of upper bound constraints.
 * We do not compute any enforced constraints if we were forced
 * to compute a lower or upper bound using exact_bound.  The domains
 * of the resulting expressions may imply some bounds on outer dimensions
 * that we do not want to appear in the enforced constraints since
 * they are not actually enforced by the corresponding code.
 *
 * Finally, we fill in the expressions of the for node.
 */
static __isl_give isl_ast_graft *refine_generic_bounds(
	__isl_take isl_ast_graft *graft,
	__isl_take isl_constraint_list *c_lower,
	__isl_take isl_constraint_list *c_upper,
	__isl_keep isl_set *domain, __isl_keep isl_ast_build *build)
{
	int depth;
	isl_ctx *ctx;
	isl_pw_aff_list *lower;
	int use_list;
	isl_set *upper_set = NULL;
	isl_pw_aff_list *upper_list = NULL;
	int n_lower, n_upper;

	if (!graft || !c_lower || !c_upper || !build)
		goto error;

	depth = isl_ast_build_get_depth(build);
	ctx = isl_ast_graft_get_ctx(graft);

	n_lower = isl_constraint_list_n_constraint(c_lower);
	n_upper = isl_constraint_list_n_constraint(c_upper);

	use_list = use_upper_bound_list(ctx, n_upper, domain, depth);

	lower = lower_bounds(c_lower, depth, domain, build);

	if (use_list)
		upper_list = upper_bounds(c_upper, depth, domain, build);
	else if (n_upper > 0)
		upper_set = intersect_constraints(c_upper);
	else
		upper_set = isl_set_universe(isl_set_get_space(domain));

	if (n_lower == 0 || n_upper == 0)
		;
	else if (use_list)
		graft = set_enforced_from_list(graft, lower, upper_list);
	else
		graft = set_enforced_from_set(graft, lower, depth, upper_set);

	graft = set_for_node_expressions(graft, lower, use_list, upper_list,
					upper_set, build);

	isl_pw_aff_list_free(lower);
	isl_pw_aff_list_free(upper_list);
	isl_set_free(upper_set);
	isl_constraint_list_free(c_lower);
	isl_constraint_list_free(c_upper);

	return graft;
error:
	isl_constraint_list_free(c_lower);
	isl_constraint_list_free(c_upper);
	return isl_ast_graft_free(graft);
}

/* Internal data structure used inside count_constraints to keep
 * track of the number of constraints that are independent of dimension "pos",
 * the lower bounds in "pos" and the upper bounds in "pos".
 */
struct isl_ast_count_constraints_data {
	int pos;

	int n_indep;
	int n_lower;
	int n_upper;
};

/* Increment data->n_indep, data->lower or data->upper depending
 * on whether "c" is independenct of dimensions data->pos,
 * a lower bound or an upper bound.
 */
static isl_stat count_constraints(__isl_take isl_constraint *c, void *user)
{
	struct isl_ast_count_constraints_data *data = user;

	if (isl_constraint_is_lower_bound(c, isl_dim_set, data->pos))
		data->n_lower++;
	else if (isl_constraint_is_upper_bound(c, isl_dim_set, data->pos))
		data->n_upper++;
	else
		data->n_indep++;

	isl_constraint_free(c);

	return isl_stat_ok;
}

/* Update "graft" based on "bounds" and "domain" for the generic,
 * non-degenerate, case.
 *
 * "list" respresent the list of bounds that need to be encoded by
 * the for loop.  Only the constraints that involve the iterator
 * are relevant here.  The other constraints are taken care of by
 * the caller and are included in the generated constraints of "build".
 * "domain" is the subset of the intersection of the constraints
 * for which some code is executed.
 * "build" is the build in which graft->node was created.
 *
 * We separate lower bounds, upper bounds and constraints that
 * are independent of the loop iterator.
 *
 * The actual for loop bounds are generated in refine_generic_bounds.
 */
static __isl_give isl_ast_graft *refine_generic_split(
	__isl_take isl_ast_graft *graft, __isl_take isl_constraint_list *list,
	__isl_keep isl_set *domain, __isl_keep isl_ast_build *build)
{
	struct isl_ast_count_constraints_data data;
	isl_constraint_list *lower;
	isl_constraint_list *upper;

	if (!list)
		return isl_ast_graft_free(graft);

	data.pos = isl_ast_build_get_depth(build);

	list = isl_constraint_list_sort(list, &cmp_constraint, &data.pos);
	if (!list)
		return isl_ast_graft_free(graft);

	data.n_indep = data.n_lower = data.n_upper = 0;
	if (isl_constraint_list_foreach(list, &count_constraints, &data) < 0) {
		isl_constraint_list_free(list);
		return isl_ast_graft_free(graft);
	}

	lower = isl_constraint_list_drop(list, 0, data.n_indep);
	upper = isl_constraint_list_copy(lower);
	lower = isl_constraint_list_drop(lower, data.n_lower, data.n_upper);
	upper = isl_constraint_list_drop(upper, 0, data.n_lower);

	return refine_generic_bounds(graft, lower, upper, domain, build);
}

/* Update "graft" based on "bounds" and "domain" for the generic,
 * non-degenerate, case.
 *
 * "bounds" respresent the bounds that need to be encoded by
 * the for loop (or a guard around the for loop).
 * "domain" is the subset of "bounds" for which some code is executed.
 * "build" is the build in which graft->node was created.
 *
 * We break up "bounds" into a list of constraints and continue with
 * refine_generic_split.
 */
static __isl_give isl_ast_graft *refine_generic(
	__isl_take isl_ast_graft *graft,
	__isl_keep isl_basic_set *bounds, __isl_keep isl_set *domain,
	__isl_keep isl_ast_build *build)
{
	isl_constraint_list *list;

	if (!build || !graft)
		return isl_ast_graft_free(graft);

	list = isl_basic_set_get_constraint_list(bounds);

	graft = refine_generic_split(graft, list, domain, build);

	return graft;
}

/* Create a for node for the current level.
 *
 * Mark the for node degenerate if "degenerate" is set.
 */
static __isl_give isl_ast_node *create_for(__isl_keep isl_ast_build *build,
	int degenerate)
{
	int depth;
	isl_id *id;
	isl_ast_node *node;

	if (!build)
		return NULL;

	depth = isl_ast_build_get_depth(build);
	id = isl_ast_build_get_iterator_id(build, depth);
	node = isl_ast_node_alloc_for(id);
	if (degenerate)
		node = isl_ast_node_for_mark_degenerate(node);

	return node;
}

/* If the ast_build_exploit_nested_bounds option is set, then return
 * the constraints enforced by all elements in "list".
 * Otherwise, return the universe.
 */
static __isl_give isl_basic_set *extract_shared_enforced(
	__isl_keep isl_ast_graft_list *list, __isl_keep isl_ast_build *build)
{
	isl_ctx *ctx;
	isl_space *space;

	if (!list)
		return NULL;

	ctx = isl_ast_graft_list_get_ctx(list);
	if (isl_options_get_ast_build_exploit_nested_bounds(ctx))
		return isl_ast_graft_list_extract_shared_enforced(list, build);

	space = isl_ast_build_get_space(build, 1);
	return isl_basic_set_universe(space);
}

/* Return the pending constraints of "build" that are not already taken
 * care of (by a combination of "enforced" and the generated constraints
 * of "build").
 */
static __isl_give isl_set *extract_pending(__isl_keep isl_ast_build *build,
	__isl_keep isl_basic_set *enforced)
{
	isl_set *guard, *context;

	guard = isl_ast_build_get_pending(build);
	context = isl_set_from_basic_set(isl_basic_set_copy(enforced));
	context = isl_set_intersect(context,
					isl_ast_build_get_generated(build));
	return isl_set_gist(guard, context);
}

/* Create an AST node for the current dimension based on
 * the schedule domain "bounds" and return the node encapsulated
 * in an isl_ast_graft.
 *
 * "executed" is the current inverse schedule, taking into account
 * the bounds in "bounds"
 * "domain" is the domain of "executed", with inner dimensions projected out.
 * It may be a strict subset of "bounds" in case "bounds" was created
 * based on the atomic option or based on separation with explicit bounds.
 *
 * "domain" may satisfy additional equalities that result
 * from intersecting "executed" with "bounds" in add_node.
 * It may also satisfy some global constraints that were dropped out because
 * we performed separation with explicit bounds.
 * The very first step is then to copy these constraints to "bounds".
 *
 * Since we may be calling before_each_for and after_each_for
 * callbacks, we record the current inverse schedule in the build.
 *
 * We consider three builds,
 * "build" is the one in which the current level is created,
 * "body_build" is the build in which the next level is created,
 * "sub_build" is essentially the same as "body_build", except that
 * the depth has not been increased yet.
 *
 * "build" already contains information (in strides and offsets)
 * about the strides at the current level, but this information is not
 * reflected in the build->domain.
 * We first add this information and the "bounds" to the sub_build->domain.
 * isl_ast_build_set_loop_bounds adds the stride information and
 * checks whether the current dimension attains
 * only a single value and whether this single value can be represented using
 * a single affine expression.
 * In the first case, the current level is considered "degenerate".
 * In the second, sub-case, the current level is considered "eliminated".
 * Eliminated levels don't need to be reflected in the AST since we can
 * simply plug in the affine expression.  For degenerate, but non-eliminated,
 * levels, we do introduce a for node, but mark is as degenerate so that
 * it can be printed as an assignment of the single value to the loop
 * "iterator".
 *
 * If the current level is eliminated, we explicitly plug in the value
 * for the current level found by isl_ast_build_set_loop_bounds in the
 * inverse schedule.  This ensures that if we are working on a slice
 * of the domain based on information available in the inverse schedule
 * and the build domain, that then this information is also reflected
 * in the inverse schedule.  This operation also eliminates the current
 * dimension from the inverse schedule making sure no inner dimensions depend
 * on the current dimension.  Otherwise, we create a for node, marking
 * it degenerate if appropriate.  The initial for node is still incomplete
 * and will be completed in either refine_degenerate or refine_generic.
 *
 * We then generate a sequence of grafts for the next level,
 * create a surrounding graft for the current level and insert
 * the for node we created (if the current level is not eliminated).
 * Before creating a graft for the current level, we first extract
 * hoistable constraints from the child guards and combine them
 * with the pending constraints in the build.  These constraints
 * are used to simplify the child guards and then added to the guard
 * of the current graft to ensure that they will be generated.
 * If the hoisted guard is a disjunction, then we use it directly
 * to gist the guards on the children before intersect it with the
 * pending constraints.  We do so because this disjunction is typically
 * identical to the guards on the children such that these guards
 * can be effectively removed completely.  After the intersection,
 * the gist operation would have a harder time figuring this out.
 *
 * Finally, we set the bounds of the for loop in either
 * refine_degenerate or refine_generic.
 * We do so in a context where the pending constraints of the build
 * have been replaced by the guard of the current graft.
 */
static __isl_give isl_ast_graft *create_node_scaled(
	__isl_take isl_union_map *executed,
	__isl_take isl_basic_set *bounds, __isl_take isl_set *domain,
	__isl_take isl_ast_build *build)
{
	int depth;
	int degenerate, eliminated;
	isl_basic_set *hull;
	isl_basic_set *enforced;
	isl_set *guard, *hoisted;
	isl_ast_node *node = NULL;
	isl_ast_graft *graft;
	isl_ast_graft_list *children;
	isl_ast_build *sub_build;
	isl_ast_build *body_build;

	domain = isl_ast_build_eliminate_divs(build, domain);
	domain = isl_set_detect_equalities(domain);
	hull = isl_set_unshifted_simple_hull(isl_set_copy(domain));
	bounds = isl_basic_set_intersect(bounds, hull);
	build = isl_ast_build_set_executed(build, isl_union_map_copy(executed));

	depth = isl_ast_build_get_depth(build);
	sub_build = isl_ast_build_copy(build);
	bounds = isl_basic_set_remove_redundancies(bounds);
	sub_build = isl_ast_build_set_loop_bounds(sub_build,
						isl_basic_set_copy(bounds));
	degenerate = isl_ast_build_has_value(sub_build);
	eliminated = isl_ast_build_has_affine_value(sub_build, depth);
	if (degenerate < 0 || eliminated < 0)
		executed = isl_union_map_free(executed);
	if (eliminated)
		executed = plug_in_values(executed, sub_build);
	else
		node = create_for(build, degenerate);

	body_build = isl_ast_build_copy(sub_build);
	body_build = isl_ast_build_increase_depth(body_build);
	if (!eliminated)
		node = before_each_for(node, body_build);
	children = generate_next_level(executed,
				    isl_ast_build_copy(body_build));

	enforced = extract_shared_enforced(children, build);
	guard = extract_pending(sub_build, enforced);
	hoisted = isl_ast_graft_list_extract_hoistable_guard(children, build);
	if (isl_set_n_basic_set(hoisted) > 1)
		children = isl_ast_graft_list_gist_guards(children,
						    isl_set_copy(hoisted));
	guard = isl_set_intersect(guard, hoisted);
	if (!eliminated)
		guard = add_implied_guards(guard, degenerate, bounds, build);

	graft = isl_ast_graft_alloc_from_children(children,
			    isl_set_copy(guard), enforced, build, sub_build);

	if (!degenerate)
		bounds = isl_ast_build_compute_gist_basic_set(build, bounds);
	if (!eliminated) {
		isl_ast_build *for_build;

		graft = isl_ast_graft_insert_for(graft, node);
		for_build = isl_ast_build_copy(build);
		for_build = isl_ast_build_replace_pending_by_guard(for_build,
							isl_set_copy(guard));
		if (degenerate)
			graft = refine_degenerate(graft, for_build, sub_build);
		else
			graft = refine_generic(graft, bounds,
					domain, for_build);
		isl_ast_build_free(for_build);
	}
	isl_set_free(guard);
	if (!eliminated)
		graft = after_each_for(graft, body_build);

	isl_ast_build_free(body_build);
	isl_ast_build_free(sub_build);
	isl_ast_build_free(build);
	isl_basic_set_free(bounds);
	isl_set_free(domain);

	return graft;
}

/* Internal data structure for checking if all constraints involving
 * the input dimension "depth" are such that the other coefficients
 * are multiples of "m", reducing "m" if they are not.
 * If "m" is reduced all the way down to "1", then the check has failed
 * and we break out of the iteration.
 */
struct isl_check_scaled_data {
	int depth;
	isl_val *m;
};

/* If constraint "c" involves the input dimension data->depth,
 * then make sure that all the other coefficients are multiples of data->m,
 * reducing data->m if needed.
 * Break out of the iteration if data->m has become equal to "1".
 */
static isl_stat constraint_check_scaled(__isl_take isl_constraint *c,
	void *user)
{
	struct isl_check_scaled_data *data = user;
	int i, j, n;
	enum isl_dim_type t[] = { isl_dim_param, isl_dim_in, isl_dim_out,
				    isl_dim_div };

	if (!isl_constraint_involves_dims(c, isl_dim_in, data->depth, 1)) {
		isl_constraint_free(c);
		return isl_stat_ok;
	}

	for (i = 0; i < 4; ++i) {
		n = isl_constraint_dim(c, t[i]);
		for (j = 0; j < n; ++j) {
			isl_val *d;

			if (t[i] == isl_dim_in && j == data->depth)
				continue;
			if (!isl_constraint_involves_dims(c, t[i], j, 1))
				continue;
			d = isl_constraint_get_coefficient_val(c, t[i], j);
			data->m = isl_val_gcd(data->m, d);
			if (isl_val_is_one(data->m))
				break;
		}
		if (j < n)
			break;
	}

	isl_constraint_free(c);

	return i < 4 ? isl_stat_error : isl_stat_ok;
}

/* For each constraint of "bmap" that involves the input dimension data->depth,
 * make sure that all the other coefficients are multiples of data->m,
 * reducing data->m if needed.
 * Break out of the iteration if data->m has become equal to "1".
 */
static isl_stat basic_map_check_scaled(__isl_take isl_basic_map *bmap,
	void *user)
{
	isl_stat r;

	r = isl_basic_map_foreach_constraint(bmap,
						&constraint_check_scaled, user);
	isl_basic_map_free(bmap);

	return r;
}

/* For each constraint of "map" that involves the input dimension data->depth,
 * make sure that all the other coefficients are multiples of data->m,
 * reducing data->m if needed.
 * Break out of the iteration if data->m has become equal to "1".
 */
static isl_stat map_check_scaled(__isl_take isl_map *map, void *user)
{
	isl_stat r;

	r = isl_map_foreach_basic_map(map, &basic_map_check_scaled, user);
	isl_map_free(map);

	return r;
}

/* Create an AST node for the current dimension based on
 * the schedule domain "bounds" and return the node encapsulated
 * in an isl_ast_graft.
 *
 * "executed" is the current inverse schedule, taking into account
 * the bounds in "bounds"
 * "domain" is the domain of "executed", with inner dimensions projected out.
 *
 *
 * Before moving on to the actual AST node construction in create_node_scaled,
 * we first check if the current dimension is strided and if we can scale
 * down this stride.  Note that we only do this if the ast_build_scale_strides
 * option is set.
 *
 * In particular, let the current dimension take on values
 *
 *	f + s a
 *
 * with a an integer.  We check if we can find an integer m that (obviously)
 * divides both f and s.
 *
 * If so, we check if the current dimension only appears in constraints
 * where the coefficients of the other variables are multiples of m.
 * We perform this extra check to avoid the risk of introducing
 * divisions by scaling down the current dimension.
 *
 * If so, we scale the current dimension down by a factor of m.
 * That is, we plug in
 *
 *	i = m i'							(1)
 *
 * Note that in principle we could always scale down strided loops
 * by plugging in
 *
 *	i = f + s i'
 *
 * but this may result in i' taking on larger values than the original i,
 * due to the shift by "f".
 * By constrast, the scaling in (1) can only reduce the (absolute) value "i".
 */
static __isl_give isl_ast_graft *create_node(__isl_take isl_union_map *executed,
	__isl_take isl_basic_set *bounds, __isl_take isl_set *domain,
	__isl_take isl_ast_build *build)
{
	struct isl_check_scaled_data data;
	isl_ctx *ctx;
	isl_aff *offset;
	isl_val *d;

	ctx = isl_ast_build_get_ctx(build);
	if (!isl_options_get_ast_build_scale_strides(ctx))
		return create_node_scaled(executed, bounds, domain, build);

	data.depth = isl_ast_build_get_depth(build);
	if (!isl_ast_build_has_stride(build, data.depth))
		return create_node_scaled(executed, bounds, domain, build);

	offset = isl_ast_build_get_offset(build, data.depth);
	data.m = isl_ast_build_get_stride(build, data.depth);
	if (!data.m)
		offset = isl_aff_free(offset);
	offset = isl_aff_scale_down_val(offset, isl_val_copy(data.m));
	d = isl_aff_get_denominator_val(offset);
	if (!d)
		executed = isl_union_map_free(executed);

	if (executed && isl_val_is_divisible_by(data.m, d))
		data.m = isl_val_div(data.m, d);
	else {
		data.m = isl_val_set_si(data.m, 1);
		isl_val_free(d);
	}

	if (!isl_val_is_one(data.m)) {
		if (isl_union_map_foreach_map(executed, &map_check_scaled,
						&data) < 0 &&
		    !isl_val_is_one(data.m))
			executed = isl_union_map_free(executed);
	}

	if (!isl_val_is_one(data.m)) {
		isl_space *space;
		isl_multi_aff *ma;
		isl_aff *aff;
		isl_map *map;
		isl_union_map *umap;

		space = isl_ast_build_get_space(build, 1);
		space = isl_space_map_from_set(space);
		ma = isl_multi_aff_identity(space);
		aff = isl_multi_aff_get_aff(ma, data.depth);
		aff = isl_aff_scale_val(aff, isl_val_copy(data.m));
		ma = isl_multi_aff_set_aff(ma, data.depth, aff);

		bounds = isl_basic_set_preimage_multi_aff(bounds,
						isl_multi_aff_copy(ma));
		domain = isl_set_preimage_multi_aff(domain,
						isl_multi_aff_copy(ma));
		map = isl_map_reverse(isl_map_from_multi_aff(ma));
		umap = isl_union_map_from_map(map);
		executed = isl_union_map_apply_domain(executed,
						isl_union_map_copy(umap));
		build = isl_ast_build_scale_down(build, isl_val_copy(data.m),
						umap);
	}
	isl_aff_free(offset);
	isl_val_free(data.m);

	return create_node_scaled(executed, bounds, domain, build);
}

/* Add the basic set to the list that "user" points to.
 */
static isl_stat collect_basic_set(__isl_take isl_basic_set *bset, void *user)
{
	isl_basic_set_list **list = user;

	*list = isl_basic_set_list_add(*list, bset);

	return isl_stat_ok;
}

/* Extract the basic sets of "set" and collect them in an isl_basic_set_list.
 */
static __isl_give isl_basic_set_list *isl_basic_set_list_from_set(
	__isl_take isl_set *set)
{
	int n;
	isl_ctx *ctx;
	isl_basic_set_list *list;

	if (!set)
		return NULL;

	ctx = isl_set_get_ctx(set);

	n = isl_set_n_basic_set(set);
	list = isl_basic_set_list_alloc(ctx, n);
	if (isl_set_foreach_basic_set(set, &collect_basic_set, &list) < 0)
		list = isl_basic_set_list_free(list);

	isl_set_free(set);
	return list;
}

/* Generate code for the schedule domain "bounds"
 * and add the result to "list".
 *
 * We mainly detect strides here and check if the bounds do not
 * conflict with the current build domain
 * and then pass over control to create_node.
 *
 * "bounds" reflects the bounds on the current dimension and possibly
 * some extra conditions on outer dimensions.
 * It does not, however, include any divs involving the current dimension,
 * so it does not capture any stride constraints.
 * We therefore need to compute that part of the schedule domain that
 * intersects with "bounds" and derive the strides from the result.
 */
static __isl_give isl_ast_graft_list *add_node(
	__isl_take isl_ast_graft_list *list, __isl_take isl_union_map *executed,
	__isl_take isl_basic_set *bounds, __isl_take isl_ast_build *build)
{
	isl_ast_graft *graft;
	isl_set *domain = NULL;
	isl_union_set *uset;
	int empty, disjoint;

	uset = isl_union_set_from_basic_set(isl_basic_set_copy(bounds));
	executed = isl_union_map_intersect_domain(executed, uset);
	empty = isl_union_map_is_empty(executed);
	if (empty < 0)
		goto error;
	if (empty)
		goto done;

	uset = isl_union_map_domain(isl_union_map_copy(executed));
	domain = isl_set_from_union_set(uset);
	domain = isl_ast_build_specialize(build, domain);

	domain = isl_set_compute_divs(domain);
	domain = isl_ast_build_eliminate_inner(build, domain);
	disjoint = isl_set_is_disjoint(domain, build->domain);
	if (disjoint < 0)
		goto error;
	if (disjoint)
		goto done;

	build = isl_ast_build_detect_strides(build, isl_set_copy(domain));

	graft = create_node(executed, bounds, domain,
				isl_ast_build_copy(build));
	list = isl_ast_graft_list_add(list, graft);
	isl_ast_build_free(build);
	return list;
error:
	list = isl_ast_graft_list_free(list);
done:
	isl_set_free(domain);
	isl_basic_set_free(bounds);
	isl_union_map_free(executed);
	isl_ast_build_free(build);
	return list;
}

/* Does any element of i follow or coincide with any element of j
 * at the current depth for equal values of the outer dimensions?
 */
static isl_bool domain_follows_at_depth(__isl_keep isl_basic_set *i,
	__isl_keep isl_basic_set *j, void *user)
{
	int depth = *(int *) user;
	isl_basic_map *test;
	isl_bool empty;
	int l;

	test = isl_basic_map_from_domain_and_range(isl_basic_set_copy(i),
						    isl_basic_set_copy(j));
	for (l = 0; l < depth; ++l)
		test = isl_basic_map_equate(test, isl_dim_in, l,
						isl_dim_out, l);
	test = isl_basic_map_order_ge(test, isl_dim_in, depth,
					isl_dim_out, depth);
	empty = isl_basic_map_is_empty(test);
	isl_basic_map_free(test);

	return empty < 0 ? isl_bool_error : !empty;
}

/* Split up each element of "list" into a part that is related to "bset"
 * according to "gt" and a part that is not.
 * Return a list that consist of "bset" and all the pieces.
 */
static __isl_give isl_basic_set_list *add_split_on(
	__isl_take isl_basic_set_list *list, __isl_take isl_basic_set *bset,
	__isl_keep isl_basic_map *gt)
{
	int i, n;
	isl_basic_set_list *res;

	if (!list)
		bset = isl_basic_set_free(bset);

	gt = isl_basic_map_copy(gt);
	gt = isl_basic_map_intersect_domain(gt, isl_basic_set_copy(bset));
	n = isl_basic_set_list_n_basic_set(list);
	res = isl_basic_set_list_from_basic_set(bset);
	for (i = 0; res && i < n; ++i) {
		isl_basic_set *bset;
		isl_set *set1, *set2;
		isl_basic_map *bmap;
		int empty;

		bset = isl_basic_set_list_get_basic_set(list, i);
		bmap = isl_basic_map_copy(gt);
		bmap = isl_basic_map_intersect_range(bmap, bset);
		bset = isl_basic_map_range(bmap);
		empty = isl_basic_set_is_empty(bset);
		if (empty < 0)
			res = isl_basic_set_list_free(res);
		if (empty)  {
			isl_basic_set_free(bset);
			bset = isl_basic_set_list_get_basic_set(list, i);
			res = isl_basic_set_list_add(res, bset);
			continue;
		}

		res = isl_basic_set_list_add(res, isl_basic_set_copy(bset));
		set1 = isl_set_from_basic_set(bset);
		bset = isl_basic_set_list_get_basic_set(list, i);
		set2 = isl_set_from_basic_set(bset);
		set1 = isl_set_subtract(set2, set1);
		set1 = isl_set_make_disjoint(set1);

		res = isl_basic_set_list_concat(res,
					    isl_basic_set_list_from_set(set1));
	}
	isl_basic_map_free(gt);
	isl_basic_set_list_free(list);
	return res;
}

static __isl_give isl_ast_graft_list *generate_sorted_domains(
	__isl_keep isl_basic_set_list *domain_list,
	__isl_keep isl_union_map *executed,
	__isl_keep isl_ast_build *build);

/* Internal data structure for add_nodes.
 *
 * "executed" and "build" are extra arguments to be passed to add_node.
 * "list" collects the results.
 */
struct isl_add_nodes_data {
	isl_union_map *executed;
	isl_ast_build *build;

	isl_ast_graft_list *list;
};

/* Generate code for the schedule domains in "scc"
 * and add the results to "list".
 *
 * The domains in "scc" form a strongly connected component in the ordering.
 * If the number of domains in "scc" is larger than 1, then this means
 * that we cannot determine a valid ordering for the domains in the component.
 * This should be fairly rare because the individual domains
 * have been made disjoint first.
 * The problem is that the domains may be integrally disjoint but not
 * rationally disjoint.  For example, we may have domains
 *
 *	{ [i,i] : 0 <= i <= 1 }		and	{ [i,1-i] : 0 <= i <= 1 }
 *
 * These two domains have an empty intersection, but their rational
 * relaxations do intersect.  It is impossible to order these domains
 * in the second dimension because the first should be ordered before
 * the second for outer dimension equal to 0, while it should be ordered
 * after for outer dimension equal to 1.
 *
 * This may happen in particular in case of unrolling since the domain
 * of each slice is replaced by its simple hull.
 *
 * For each basic set i in "scc" and for each of the following basic sets j,
 * we split off that part of the basic set i that shares the outer dimensions
 * with j and lies before j in the current dimension.
 * We collect all the pieces in a new list that replaces "scc".
 *
 * While the elements in "scc" should be disjoint, we double-check
 * this property to avoid running into an infinite recursion in case
 * they intersect due to some internal error.
 */
static isl_stat add_nodes(__isl_take isl_basic_set_list *scc, void *user)
{
	struct isl_add_nodes_data *data = user;
	int i, n, depth;
	isl_basic_set *bset, *first;
	isl_basic_set_list *list;
	isl_space *space;
	isl_basic_map *gt;

	n = isl_basic_set_list_n_basic_set(scc);
	bset = isl_basic_set_list_get_basic_set(scc, 0);
	if (n == 1) {
		isl_basic_set_list_free(scc);
		data->list = add_node(data->list,
				isl_union_map_copy(data->executed), bset,
				isl_ast_build_copy(data->build));
		return data->list ? isl_stat_ok : isl_stat_error;
	}

	depth = isl_ast_build_get_depth(data->build);
	space = isl_basic_set_get_space(bset);
	space = isl_space_map_from_set(space);
	gt = isl_basic_map_universe(space);
	for (i = 0; i < depth; ++i)
		gt = isl_basic_map_equate(gt, isl_dim_in, i, isl_dim_out, i);
	gt = isl_basic_map_order_gt(gt, isl_dim_in, depth, isl_dim_out, depth);

	first = isl_basic_set_copy(bset);
	list = isl_basic_set_list_from_basic_set(bset);
	for (i = 1; i < n; ++i) {
		int disjoint;

		bset = isl_basic_set_list_get_basic_set(scc, i);

		disjoint = isl_basic_set_is_disjoint(bset, first);
		if (disjoint < 0)
			list = isl_basic_set_list_free(list);
		else if (!disjoint)
			isl_die(isl_basic_set_list_get_ctx(scc),
				isl_error_internal,
				"basic sets in scc are assumed to be disjoint",
				list = isl_basic_set_list_free(list));

		list = add_split_on(list, bset, gt);
	}
	isl_basic_set_free(first);
	isl_basic_map_free(gt);
	isl_basic_set_list_free(scc);
	scc = list;
	data->list = isl_ast_graft_list_concat(data->list,
		    generate_sorted_domains(scc, data->executed, data->build));
	isl_basic_set_list_free(scc);

	return data->list ? isl_stat_ok : isl_stat_error;
}

/* Sort the domains in "domain_list" according to the execution order
 * at the current depth (for equal values of the outer dimensions),
 * generate code for each of them, collecting the results in a list.
 * If no code is generated (because the intersection of the inverse schedule
 * with the domains turns out to be empty), then an empty list is returned.
 *
 * The caller is responsible for ensuring that the basic sets in "domain_list"
 * are pair-wise disjoint.  It can, however, in principle happen that
 * two basic sets should be ordered one way for one value of the outer
 * dimensions and the other way for some other value of the outer dimensions.
 * We therefore play safe and look for strongly connected components.
 * The function add_nodes takes care of handling non-trivial components.
 */
static __isl_give isl_ast_graft_list *generate_sorted_domains(
	__isl_keep isl_basic_set_list *domain_list,
	__isl_keep isl_union_map *executed, __isl_keep isl_ast_build *build)
{
	isl_ctx *ctx;
	struct isl_add_nodes_data data;
	int depth;
	int n;

	if (!domain_list)
		return NULL;

	ctx = isl_basic_set_list_get_ctx(domain_list);
	n = isl_basic_set_list_n_basic_set(domain_list);
	data.list = isl_ast_graft_list_alloc(ctx, n);
	if (n == 0)
		return data.list;
	if (n == 1)
		return add_node(data.list, isl_union_map_copy(executed),
			isl_basic_set_list_get_basic_set(domain_list, 0),
			isl_ast_build_copy(build));

	depth = isl_ast_build_get_depth(build);
	data.executed = executed;
	data.build = build;
	if (isl_basic_set_list_foreach_scc(domain_list,
					&domain_follows_at_depth, &depth,
					&add_nodes, &data) < 0)
		data.list = isl_ast_graft_list_free(data.list);

	return data.list;
}

/* Do i and j share any values for the outer dimensions?
 */
static isl_bool shared_outer(__isl_keep isl_basic_set *i,
	__isl_keep isl_basic_set *j, void *user)
{
	int depth = *(int *) user;
	isl_basic_map *test;
	isl_bool empty;
	int l;

	test = isl_basic_map_from_domain_and_range(isl_basic_set_copy(i),
						    isl_basic_set_copy(j));
	for (l = 0; l < depth; ++l)
		test = isl_basic_map_equate(test, isl_dim_in, l,
						isl_dim_out, l);
	empty = isl_basic_map_is_empty(test);
	isl_basic_map_free(test);

	return empty < 0 ? isl_bool_error : !empty;
}

/* Internal data structure for generate_sorted_domains_wrap.
 *
 * "n" is the total number of basic sets
 * "executed" and "build" are extra arguments to be passed
 *	to generate_sorted_domains.
 *
 * "single" is set to 1 by generate_sorted_domains_wrap if there
 * is only a single component.
 * "list" collects the results.
 */
struct isl_ast_generate_parallel_domains_data {
	int n;
	isl_union_map *executed;
	isl_ast_build *build;

	int single;
	isl_ast_graft_list *list;
};

/* Call generate_sorted_domains on "scc", fuse the result into a list
 * with either zero or one graft and collect the these single element
 * lists into data->list.
 *
 * If there is only one component, i.e., if the number of basic sets
 * in the current component is equal to the total number of basic sets,
 * then data->single is set to 1 and the result of generate_sorted_domains
 * is not fused.
 */
static isl_stat generate_sorted_domains_wrap(__isl_take isl_basic_set_list *scc,
	void *user)
{
	struct isl_ast_generate_parallel_domains_data *data = user;
	isl_ast_graft_list *list;

	list = generate_sorted_domains(scc, data->executed, data->build);
	data->single = isl_basic_set_list_n_basic_set(scc) == data->n;
	if (!data->single)
		list = isl_ast_graft_list_fuse(list, data->build);
	if (!data->list)
		data->list = list;
	else
		data->list = isl_ast_graft_list_concat(data->list, list);

	isl_basic_set_list_free(scc);
	if (!data->list)
		return isl_stat_error;

	return isl_stat_ok;
}

/* Look for any (weakly connected) components in the "domain_list"
 * of domains that share some values of the outer dimensions.
 * That is, domains in different components do not share any values
 * of the outer dimensions.  This means that these components
 * can be freely reordered.
 * Within each of the components, we sort the domains according
 * to the execution order at the current depth.
 *
 * If there is more than one component, then generate_sorted_domains_wrap
 * fuses the result of each call to generate_sorted_domains
 * into a list with either zero or one graft and collects these (at most)
 * single element lists into a bigger list. This means that the elements of the
 * final list can be freely reordered.  In particular, we sort them
 * according to an arbitrary but fixed ordering to ease merging of
 * graft lists from different components.
 */
static __isl_give isl_ast_graft_list *generate_parallel_domains(
	__isl_keep isl_basic_set_list *domain_list,
	__isl_keep isl_union_map *executed, __isl_keep isl_ast_build *build)
{
	int depth;
	struct isl_ast_generate_parallel_domains_data data;

	if (!domain_list)
		return NULL;

	data.n = isl_basic_set_list_n_basic_set(domain_list);
	if (data.n <= 1)
		return generate_sorted_domains(domain_list, executed, build);

	depth = isl_ast_build_get_depth(build);
	data.list = NULL;
	data.executed = executed;
	data.build = build;
	data.single = 0;
	if (isl_basic_set_list_foreach_scc(domain_list, &shared_outer, &depth,
					    &generate_sorted_domains_wrap,
					    &data) < 0)
		data.list = isl_ast_graft_list_free(data.list);

	if (!data.single)
		data.list = isl_ast_graft_list_sort_guard(data.list);

	return data.list;
}

/* Internal data for separate_domain.
 *
 * "explicit" is set if we only want to use explicit bounds.
 *
 * "domain" collects the separated domains.
 */
struct isl_separate_domain_data {
	isl_ast_build *build;
	int explicit;
	isl_set *domain;
};

/* Extract implicit bounds on the current dimension for the executed "map".
 *
 * The domain of "map" may involve inner dimensions, so we
 * need to eliminate them.
 */
static __isl_give isl_set *implicit_bounds(__isl_take isl_map *map,
	__isl_keep isl_ast_build *build)
{
	isl_set *domain;

	domain = isl_map_domain(map);
	domain = isl_ast_build_eliminate(build, domain);

	return domain;
}

/* Extract explicit bounds on the current dimension for the executed "map".
 *
 * Rather than eliminating the inner dimensions as in implicit_bounds,
 * we simply drop any constraints involving those inner dimensions.
 * The idea is that most bounds that are implied by constraints on the
 * inner dimensions will be enforced by for loops and not by explicit guards.
 * There is then no need to separate along those bounds.
 */
static __isl_give isl_set *explicit_bounds(__isl_take isl_map *map,
	__isl_keep isl_ast_build *build)
{
	isl_set *domain;
	int depth, dim;

	dim = isl_map_dim(map, isl_dim_out);
	map = isl_map_drop_constraints_involving_dims(map, isl_dim_out, 0, dim);

	domain = isl_map_domain(map);
	depth = isl_ast_build_get_depth(build);
	dim = isl_set_dim(domain, isl_dim_set);
	domain = isl_set_detect_equalities(domain);
	domain = isl_set_drop_constraints_involving_dims(domain,
				isl_dim_set, depth + 1, dim - (depth + 1));
	domain = isl_set_remove_divs_involving_dims(domain,
				isl_dim_set, depth, 1);
	domain = isl_set_remove_unknown_divs(domain);

	return domain;
}

/* Split data->domain into pieces that intersect with the range of "map"
 * and pieces that do not intersect with the range of "map"
 * and then add that part of the range of "map" that does not intersect
 * with data->domain.
 */
static isl_stat separate_domain(__isl_take isl_map *map, void *user)
{
	struct isl_separate_domain_data *data = user;
	isl_set *domain;
	isl_set *d1, *d2;

	if (data->explicit)
		domain = explicit_bounds(map, data->build);
	else
		domain = implicit_bounds(map, data->build);

	domain = isl_set_coalesce(domain);
	domain = isl_set_make_disjoint(domain);
	d1 = isl_set_subtract(isl_set_copy(domain), isl_set_copy(data->domain));
	d2 = isl_set_subtract(isl_set_copy(data->domain), isl_set_copy(domain));
	data->domain = isl_set_intersect(data->domain, domain);
	data->domain = isl_set_union(data->domain, d1);
	data->domain = isl_set_union(data->domain, d2);

	return isl_stat_ok;
}

/* Separate the schedule domains of "executed".
 *
 * That is, break up the domain of "executed" into basic sets,
 * such that for each basic set S, every element in S is associated with
 * the same domain spaces.
 *
 * "space" is the (single) domain space of "executed".
 */
static __isl_give isl_set *separate_schedule_domains(
	__isl_take isl_space *space, __isl_take isl_union_map *executed,
	__isl_keep isl_ast_build *build)
{
	struct isl_separate_domain_data data = { build };
	isl_ctx *ctx;

	ctx = isl_ast_build_get_ctx(build);
	data.explicit = isl_options_get_ast_build_separation_bounds(ctx) ==
				    ISL_AST_BUILD_SEPARATION_BOUNDS_EXPLICIT;
	data.domain = isl_set_empty(space);
	if (isl_union_map_foreach_map(executed, &separate_domain, &data) < 0)
		data.domain = isl_set_free(data.domain);

	isl_union_map_free(executed);
	return data.domain;
}

/* Temporary data used during the search for a lower bound for unrolling.
 *
 * "build" is the build in which the unrolling will be performed
 * "domain" is the original set for which to find a lower bound
 * "depth" is the dimension for which to find a lower boudn
 * "expansion" is the expansion that needs to be applied to "domain"
 * in the unrolling that will be performed
 *
 * "lower" is the best lower bound found so far.  It is NULL if we have not
 * found any yet.
 * "n" is the corresponding size.  If lower is NULL, then the value of n
 * is undefined.
 * "n_div" is the maximal number of integer divisions in the first
 * unrolled iteration (after expansion).  It is set to -1 if it hasn't
 * been computed yet.
 */
struct isl_find_unroll_data {
	isl_ast_build *build;
	isl_set *domain;
	int depth;
	isl_basic_map *expansion;

	isl_aff *lower;
	int *n;
	int n_div;
};

/* Return the constraint
 *
 *	i_"depth" = aff + offset
 */
static __isl_give isl_constraint *at_offset(int depth, __isl_keep isl_aff *aff,
	int offset)
{
	aff = isl_aff_copy(aff);
	aff = isl_aff_add_coefficient_si(aff, isl_dim_in, depth, -1);
	aff = isl_aff_add_constant_si(aff, offset);
	return isl_equality_from_aff(aff);
}

/* Update *user to the number of integer divsions in the first element
 * of "ma", if it is larger than the current value.
 */
static isl_stat update_n_div(__isl_take isl_set *set,
	__isl_take isl_multi_aff *ma, void *user)
{
	isl_aff *aff;
	int *n = user;
	int n_div;

	aff = isl_multi_aff_get_aff(ma, 0);
	n_div = isl_aff_dim(aff, isl_dim_div);
	isl_aff_free(aff);
	isl_multi_aff_free(ma);
	isl_set_free(set);

	if (n_div > *n)
		*n = n_div;

	return aff ? isl_stat_ok : isl_stat_error;
}

/* Get the number of integer divisions in the expression for the iterator
 * value at the first slice in the unrolling based on lower bound "lower",
 * taking into account the expansion that needs to be performed on this slice.
 */
static int get_expanded_n_div(struct isl_find_unroll_data *data,
	__isl_keep isl_aff *lower)
{
	isl_constraint *c;
	isl_set *set;
	isl_map *it_map, *expansion;
	isl_pw_multi_aff *pma;
	int n;

	c = at_offset(data->depth, lower, 0);
	set = isl_set_copy(data->domain);
	set = isl_set_add_constraint(set, c);
	expansion = isl_map_from_basic_map(isl_basic_map_copy(data->expansion));
	set = isl_set_apply(set, expansion);
	it_map = isl_ast_build_map_to_iterator(data->build, set);
	pma = isl_pw_multi_aff_from_map(it_map);
	n = 0;
	if (isl_pw_multi_aff_foreach_piece(pma, &update_n_div, &n) < 0)
		n = -1;
	isl_pw_multi_aff_free(pma);

	return n;
}

/* Is the lower bound "lower" with corresponding iteration count "n"
 * better than the one stored in "data"?
 * If there is no upper bound on the iteration count ("n" is infinity) or
 * if the count is too large, then we cannot use this lower bound.
 * Otherwise, if there was no previous lower bound or
 * if the iteration count of the new lower bound is smaller than
 * the iteration count of the previous lower bound, then we consider
 * the new lower bound to be better.
 * If the iteration count is the same, then compare the number
 * of integer divisions that would be needed to express
 * the iterator value at the first slice in the unrolling
 * according to the lower bound.  If we end up computing this
 * number, then store the lowest value in data->n_div.
 */
static int is_better_lower_bound(struct isl_find_unroll_data *data,
	__isl_keep isl_aff *lower, __isl_keep isl_val *n)
{
	int cmp;
	int n_div;

	if (!n)
		return -1;
	if (isl_val_is_infty(n))
		return 0;
	if (isl_val_cmp_si(n, INT_MAX) > 0)
		return 0;
	if (!data->lower)
		return 1;
	cmp = isl_val_cmp_si(n, *data->n);
	if (cmp < 0)
		return 1;
	if (cmp > 0)
		return 0;
	if (data->n_div < 0)
		data->n_div = get_expanded_n_div(data, data->lower);
	if (data->n_div < 0)
		return -1;
	if (data->n_div == 0)
		return 0;
	n_div = get_expanded_n_div(data, lower);
	if (n_div < 0)
		return -1;
	if (n_div >= data->n_div)
		return 0;
	data->n_div = n_div;

	return 1;
}

/* Check if we can use "c" as a lower bound and if it is better than
 * any previously found lower bound.
 *
 * If "c" does not involve the dimension at the current depth,
 * then we cannot use it.
 * Otherwise, let "c" be of the form
 *
 *	i >= f(j)/a
 *
 * We compute the maximal value of
 *
 *	-ceil(f(j)/a)) + i + 1
 *
 * over the domain.  If there is such a value "n", then we know
 *
 *	-ceil(f(j)/a)) + i + 1 <= n
 *
 * or
 *
 *	i < ceil(f(j)/a)) + n
 *
 * meaning that we can use ceil(f(j)/a)) as a lower bound for unrolling.
 * We just need to check if we have found any lower bound before and
 * if the new lower bound is better (smaller n or fewer integer divisions)
 * than the previously found lower bounds.
 */
static isl_stat update_unrolling_lower_bound(struct isl_find_unroll_data *data,
	__isl_keep isl_constraint *c)
{
	isl_aff *aff, *lower;
	isl_val *max;
	int better;

	if (!isl_constraint_is_lower_bound(c, isl_dim_set, data->depth))
		return isl_stat_ok;

	lower = isl_constraint_get_bound(c, isl_dim_set, data->depth);
	lower = isl_aff_ceil(lower);
	aff = isl_aff_copy(lower);
	aff = isl_aff_neg(aff);
	aff = isl_aff_add_coefficient_si(aff, isl_dim_in, data->depth, 1);
	aff = isl_aff_add_constant_si(aff, 1);
	max = isl_set_max_val(data->domain, aff);
	isl_aff_free(aff);

	better = is_better_lower_bound(data, lower, max);
	if (better < 0 || !better) {
		isl_val_free(max);
		isl_aff_free(lower);
		return better < 0 ? isl_stat_error : isl_stat_ok;
	}

	isl_aff_free(data->lower);
	data->lower = lower;
	*data->n = isl_val_get_num_si(max);
	isl_val_free(max);

	return isl_stat_ok;
}

/* Check if we can use "c" as a lower bound and if it is better than
 * any previously found lower bound.
 */
static isl_stat constraint_find_unroll(__isl_take isl_constraint *c, void *user)
{
	struct isl_find_unroll_data *data;
	isl_stat r;

	data = (struct isl_find_unroll_data *) user;
	r = update_unrolling_lower_bound(data, c);
	isl_constraint_free(c);

	return r;
}

/* Look for a lower bound l(i) on the dimension at "depth"
 * and a size n such that "domain" is a subset of
 *
 *	{ [i] : l(i) <= i_d < l(i) + n }
 *
 * where d is "depth" and l(i) depends only on earlier dimensions.
 * Furthermore, try and find a lower bound such that n is as small as possible.
 * In particular, "n" needs to be finite.
 * "build" is the build in which the unrolling will be performed.
 * "expansion" is the expansion that needs to be applied to "domain"
 * in the unrolling that will be performed.
 *
 * Inner dimensions have been eliminated from "domain" by the caller.
 *
 * We first construct a collection of lower bounds on the input set
 * by computing its simple hull.  We then iterate through them,
 * discarding those that we cannot use (either because they do not
 * involve the dimension at "depth" or because they have no corresponding
 * upper bound, meaning that "n" would be unbounded) and pick out the
 * best from the remaining ones.
 *
 * If we cannot find a suitable lower bound, then we consider that
 * to be an error.
 */
static __isl_give isl_aff *find_unroll_lower_bound(
	__isl_keep isl_ast_build *build, __isl_keep isl_set *domain,
	int depth, __isl_keep isl_basic_map *expansion, int *n)
{
	struct isl_find_unroll_data data =
			{ build, domain, depth, expansion, NULL, n, -1 };
	isl_basic_set *hull;

	hull = isl_set_simple_hull(isl_set_copy(domain));

	if (isl_basic_set_foreach_constraint(hull,
					    &constraint_find_unroll, &data) < 0)
		goto error;

	isl_basic_set_free(hull);

	if (!data.lower)
		isl_die(isl_set_get_ctx(domain), isl_error_invalid,
			"cannot find lower bound for unrolling", return NULL);

	return data.lower;
error:
	isl_basic_set_free(hull);
	return isl_aff_free(data.lower);
}

/* Call "fn" on each iteration of the current dimension of "domain".
 * If "init" is not NULL, then it is called with the number of
 * iterations before any call to "fn".
 * Return -1 on failure.
 *
 * Since we are going to be iterating over the individual values,
 * we first check if there are any strides on the current dimension.
 * If there is, we rewrite the current dimension i as
 *
 *		i = stride i' + offset
 *
 * and then iterate over individual values of i' instead.
 *
 * We then look for a lower bound on i' and a size such that the domain
 * is a subset of
 *
 *	{ [j,i'] : l(j) <= i' < l(j) + n }
 *
 * and then take slices of the domain at values of i'
 * between l(j) and l(j) + n - 1.
 *
 * We compute the unshifted simple hull of each slice to ensure that
 * we have a single basic set per offset.  The slicing constraint
 * may get simplified away before the unshifted simple hull is taken
 * and may therefore in some rare cases disappear from the result.
 * We therefore explicitly add the constraint back after computing
 * the unshifted simple hull to ensure that the basic sets
 * remain disjoint.  The constraints that are dropped by taking the hull
 * will be taken into account at the next level, as in the case of the
 * atomic option.
 *
 * Finally, we map i' back to i and call "fn".
 */
static int foreach_iteration(__isl_take isl_set *domain,
	__isl_keep isl_ast_build *build, int (*init)(int n, void *user),
	int (*fn)(__isl_take isl_basic_set *bset, void *user), void *user)
{
	int i, n;
	int empty;
	int depth;
	isl_multi_aff *expansion;
	isl_basic_map *bmap;
	isl_aff *lower = NULL;
	isl_ast_build *stride_build;

	depth = isl_ast_build_get_depth(build);

	domain = isl_ast_build_eliminate_inner(build, domain);
	domain = isl_set_intersect(domain, isl_ast_build_get_domain(build));
	stride_build = isl_ast_build_copy(build);
	stride_build = isl_ast_build_detect_strides(stride_build,
							isl_set_copy(domain));
	expansion = isl_ast_build_get_stride_expansion(stride_build);

	domain = isl_set_preimage_multi_aff(domain,
					    isl_multi_aff_copy(expansion));
	domain = isl_ast_build_eliminate_divs(stride_build, domain);
	isl_ast_build_free(stride_build);

	bmap = isl_basic_map_from_multi_aff(expansion);

	empty = isl_set_is_empty(domain);
	if (empty < 0) {
		n = -1;
	} else if (empty) {
		n = 0;
	} else {
		lower = find_unroll_lower_bound(build, domain, depth, bmap, &n);
		if (!lower)
			n = -1;
	}
	if (n >= 0 && init && init(n, user) < 0)
		n = -1;
	for (i = 0; i < n; ++i) {
		isl_set *set;
		isl_basic_set *bset;
		isl_constraint *slice;

		slice = at_offset(depth, lower, i);
		set = isl_set_copy(domain);
		set = isl_set_add_constraint(set, isl_constraint_copy(slice));
		bset = isl_set_unshifted_simple_hull(set);
		bset = isl_basic_set_add_constraint(bset, slice);
		bset = isl_basic_set_apply(bset, isl_basic_map_copy(bmap));

		if (fn(bset, user) < 0)
			break;
	}

	isl_aff_free(lower);
	isl_set_free(domain);
	isl_basic_map_free(bmap);

	return n < 0 || i < n ? -1 : 0;
}

/* Data structure for storing the results and the intermediate objects
 * of compute_domains.
 *
 * "list" is the main result of the function and contains a list
 * of disjoint basic sets for which code should be generated.
 *
 * "executed" and "build" are inputs to compute_domains.
 * "schedule_domain" is the domain of "executed".
 *
 * "option" constains the domains at the current depth that should by
 * atomic, separated or unrolled.  These domains are as specified by
 * the user, except that inner dimensions have been eliminated and
 * that they have been made pair-wise disjoint.
 *
 * "sep_class" contains the user-specified split into separation classes
 * specialized to the current depth.
 * "done" contains the union of the separation domains that have already
 * been handled.
 */
struct isl_codegen_domains {
	isl_basic_set_list *list;

	isl_union_map *executed;
	isl_ast_build *build;
	isl_set *schedule_domain;

	isl_set *option[4];

	isl_map *sep_class;
	isl_set *done;
};

/* Internal data structure for do_unroll.
 *
 * "domains" stores the results of compute_domains.
 * "class_domain" is the original class domain passed to do_unroll.
 * "unroll_domain" collects the unrolled iterations.
 */
struct isl_ast_unroll_data {
	struct isl_codegen_domains *domains;
	isl_set *class_domain;
	isl_set *unroll_domain;
};

/* Given an iteration of an unrolled domain represented by "bset",
 * add it to data->domains->list.
 * Since we may have dropped some constraints, we intersect with
 * the class domain again to ensure that each element in the list
 * is disjoint from the other class domains.
 */
static int do_unroll_iteration(__isl_take isl_basic_set *bset, void *user)
{
	struct isl_ast_unroll_data *data = user;
	isl_set *set;
	isl_basic_set_list *list;

	set = isl_set_from_basic_set(bset);
	data->unroll_domain = isl_set_union(data->unroll_domain,
					    isl_set_copy(set));
	set = isl_set_intersect(set, isl_set_copy(data->class_domain));
	set = isl_set_make_disjoint(set);
	list = isl_basic_set_list_from_set(set);
	data->domains->list = isl_basic_set_list_concat(data->domains->list,
							list);

	return 0;
}

/* Extend domains->list with a list of basic sets, one for each value
 * of the current dimension in "domain" and remove the corresponding
 * sets from the class domain.  Return the updated class domain.
 * The divs that involve the current dimension have not been projected out
 * from this domain.
 *
 * We call foreach_iteration to iterate over the individual values and
 * in do_unroll_iteration we collect the individual basic sets in
 * domains->list and their union in data->unroll_domain, which is then
 * used to update the class domain.
 */
static __isl_give isl_set *do_unroll(struct isl_codegen_domains *domains,
	__isl_take isl_set *domain, __isl_take isl_set *class_domain)
{
	struct isl_ast_unroll_data data;

	if (!domain)
		return isl_set_free(class_domain);
	if (!class_domain)
		return isl_set_free(domain);

	data.domains = domains;
	data.class_domain = class_domain;
	data.unroll_domain = isl_set_empty(isl_set_get_space(domain));

	if (foreach_iteration(domain, domains->build, NULL,
				&do_unroll_iteration, &data) < 0)
		data.unroll_domain = isl_set_free(data.unroll_domain);

	class_domain = isl_set_subtract(class_domain, data.unroll_domain);

	return class_domain;
}

/* Add domains to domains->list for each individual value of the current
 * dimension, for that part of the schedule domain that lies in the
 * intersection of the option domain and the class domain.
 * Remove the corresponding sets from the class domain and
 * return the updated class domain.
 *
 * We first break up the unroll option domain into individual pieces
 * and then handle each of them separately.  The unroll option domain
 * has been made disjoint in compute_domains_init_options,
 *
 * Note that we actively want to combine different pieces of the
 * schedule domain that have the same value at the current dimension.
 * We therefore need to break up the unroll option domain before
 * intersecting with class and schedule domain, hoping that the
 * unroll option domain specified by the user is relatively simple.
 */
static __isl_give isl_set *compute_unroll_domains(
	struct isl_codegen_domains *domains, __isl_take isl_set *class_domain)
{
	isl_set *unroll_domain;
	isl_basic_set_list *unroll_list;
	int i, n;
	int empty;

	empty = isl_set_is_empty(domains->option[isl_ast_loop_unroll]);
	if (empty < 0)
		return isl_set_free(class_domain);
	if (empty)
		return class_domain;

	unroll_domain = isl_set_copy(domains->option[isl_ast_loop_unroll]);
	unroll_list = isl_basic_set_list_from_set(unroll_domain);

	n = isl_basic_set_list_n_basic_set(unroll_list);
	for (i = 0; i < n; ++i) {
		isl_basic_set *bset;

		bset = isl_basic_set_list_get_basic_set(unroll_list, i);
		unroll_domain = isl_set_from_basic_set(bset);
		unroll_domain = isl_set_intersect(unroll_domain,
						    isl_set_copy(class_domain));
		unroll_domain = isl_set_intersect(unroll_domain,
					isl_set_copy(domains->schedule_domain));

		empty = isl_set_is_empty(unroll_domain);
		if (empty >= 0 && empty) {
			isl_set_free(unroll_domain);
			continue;
		}

		class_domain = do_unroll(domains, unroll_domain, class_domain);
	}

	isl_basic_set_list_free(unroll_list);

	return class_domain;
}

/* Try and construct a single basic set that includes the intersection of
 * the schedule domain, the atomic option domain and the class domain.
 * Add the resulting basic set(s) to domains->list and remove them
 * from class_domain.  Return the updated class domain.
 *
 * We construct a single domain rather than trying to combine
 * the schedule domains of individual domains because we are working
 * within a single component so that non-overlapping schedule domains
 * should already have been separated.
 * We do however need to make sure that this single domains is a subset
 * of the class domain so that it would not intersect with any other
 * class domains.  This means that we may end up splitting up the atomic
 * domain in case separation classes are being used.
 *
 * "domain" is the intersection of the schedule domain and the class domain,
 * with inner dimensions projected out.
 */
static __isl_give isl_set *compute_atomic_domain(
	struct isl_codegen_domains *domains, __isl_take isl_set *class_domain)
{
	isl_basic_set *bset;
	isl_basic_set_list *list;
	isl_set *domain, *atomic_domain;
	int empty;

	domain = isl_set_copy(domains->option[isl_ast_loop_atomic]);
	domain = isl_set_intersect(domain, isl_set_copy(class_domain));
	domain = isl_set_intersect(domain,
				isl_set_copy(domains->schedule_domain));
	empty = isl_set_is_empty(domain);
	if (empty < 0)
		class_domain = isl_set_free(class_domain);
	if (empty) {
		isl_set_free(domain);
		return class_domain;
	}

	domain = isl_ast_build_eliminate(domains->build, domain);
	domain = isl_set_coalesce(domain);
	bset = isl_set_unshifted_simple_hull(domain);
	domain = isl_set_from_basic_set(bset);
	atomic_domain = isl_set_copy(domain);
	domain = isl_set_intersect(domain, isl_set_copy(class_domain));
	class_domain = isl_set_subtract(class_domain, atomic_domain);
	domain = isl_set_make_disjoint(domain);
	list = isl_basic_set_list_from_set(domain);
	domains->list = isl_basic_set_list_concat(domains->list, list);

	return class_domain;
}

/* Split up the schedule domain into uniform basic sets,
 * in the sense that each element in a basic set is associated to
 * elements of the same domains, and add the result to domains->list.
 * Do this for that part of the schedule domain that lies in the
 * intersection of "class_domain" and the separate option domain.
 *
 * "class_domain" may or may not include the constraints
 * of the schedule domain, but this does not make a difference
 * since we are going to intersect it with the domain of the inverse schedule.
 * If it includes schedule domain constraints, then they may involve
 * inner dimensions, but we will eliminate them in separation_domain.
 */
static int compute_separate_domain(struct isl_codegen_domains *domains,
	__isl_keep isl_set *class_domain)
{
	isl_space *space;
	isl_set *domain;
	isl_union_map *executed;
	isl_basic_set_list *list;
	int empty;

	domain = isl_set_copy(domains->option[isl_ast_loop_separate]);
	domain = isl_set_intersect(domain, isl_set_copy(class_domain));
	executed = isl_union_map_copy(domains->executed);
	executed = isl_union_map_intersect_domain(executed,
				    isl_union_set_from_set(domain));
	empty = isl_union_map_is_empty(executed);
	if (empty < 0 || empty) {
		isl_union_map_free(executed);
		return empty < 0 ? -1 : 0;
	}

	space = isl_set_get_space(class_domain);
	domain = separate_schedule_domains(space, executed, domains->build);

	list = isl_basic_set_list_from_set(domain);
	domains->list = isl_basic_set_list_concat(domains->list, list);

	return 0;
}

/* Split up the domain at the current depth into disjoint
 * basic sets for which code should be generated separately
 * for the given separation class domain.
 *
 * If any separation classes have been defined, then "class_domain"
 * is the domain of the current class and does not refer to inner dimensions.
 * Otherwise, "class_domain" is the universe domain.
 *
 * We first make sure that the class domain is disjoint from
 * previously considered class domains.
 *
 * The separate domains can be computed directly from the "class_domain".
 *
 * The unroll, atomic and remainder domains need the constraints
 * from the schedule domain.
 *
 * For unrolling, the actual schedule domain is needed (with divs that
 * may refer to the current dimension) so that stride detection can be
 * performed.
 *
 * For atomic and remainder domains, inner dimensions and divs involving
 * the current dimensions should be eliminated.
 * In case we are working within a separation class, we need to intersect
 * the result with the current "class_domain" to ensure that the domains
 * are disjoint from those generated from other class domains.
 *
 * The domain that has been made atomic may be larger than specified
 * by the user since it needs to be representable as a single basic set.
 * This possibly larger domain is removed from class_domain by
 * compute_atomic_domain.  It is computed first so that the extended domain
 * would not overlap with any domains computed before.
 * Similary, the unrolled domains may have some constraints removed and
 * may therefore also be larger than specified by the user.
 *
 * If anything is left after handling separate, unroll and atomic,
 * we split it up into basic sets and append the basic sets to domains->list.
 */
static isl_stat compute_partial_domains(struct isl_codegen_domains *domains,
	__isl_take isl_set *class_domain)
{
	isl_basic_set_list *list;
	isl_set *domain;

	class_domain = isl_set_subtract(class_domain,
					isl_set_copy(domains->done));
	domains->done = isl_set_union(domains->done,
					isl_set_copy(class_domain));

	class_domain = compute_atomic_domain(domains, class_domain);
	class_domain = compute_unroll_domains(domains, class_domain);

	domain = isl_set_copy(class_domain);

	if (compute_separate_domain(domains, domain) < 0)
		goto error;
	domain = isl_set_subtract(domain,
			isl_set_copy(domains->option[isl_ast_loop_separate]));

	domain = isl_set_intersect(domain,
				isl_set_copy(domains->schedule_domain));

	domain = isl_ast_build_eliminate(domains->build, domain);
	domain = isl_set_intersect(domain, isl_set_copy(class_domain));

	domain = isl_set_coalesce(domain);
	domain = isl_set_make_disjoint(domain);

	list = isl_basic_set_list_from_set(domain);
	domains->list = isl_basic_set_list_concat(domains->list, list);

	isl_set_free(class_domain);

	return isl_stat_ok;
error:
	isl_set_free(domain);
	isl_set_free(class_domain);
	return isl_stat_error;
}

/* Split up the domain at the current depth into disjoint
 * basic sets for which code should be generated separately
 * for the separation class identified by "pnt".
 *
 * We extract the corresponding class domain from domains->sep_class,
 * eliminate inner dimensions and pass control to compute_partial_domains.
 */
static isl_stat compute_class_domains(__isl_take isl_point *pnt, void *user)
{
	struct isl_codegen_domains *domains = user;
	isl_set *class_set;
	isl_set *domain;
	int disjoint;

	class_set = isl_set_from_point(pnt);
	domain = isl_map_domain(isl_map_intersect_range(
				isl_map_copy(domains->sep_class), class_set));
	domain = isl_ast_build_compute_gist(domains->build, domain);
	domain = isl_ast_build_eliminate(domains->build, domain);

	disjoint = isl_set_plain_is_disjoint(domain, domains->schedule_domain);
	if (disjoint < 0)
		return isl_stat_error;
	if (disjoint) {
		isl_set_free(domain);
		return isl_stat_ok;
	}

	return compute_partial_domains(domains, domain);
}

/* Extract the domains at the current depth that should be atomic,
 * separated or unrolled and store them in option.
 *
 * The domains specified by the user might overlap, so we make
 * them disjoint by subtracting earlier domains from later domains.
 */
static void compute_domains_init_options(isl_set *option[4],
	__isl_keep isl_ast_build *build)
{
	enum isl_ast_loop_type type, type2;
	isl_set *unroll;

	for (type = isl_ast_loop_atomic;
	    type <= isl_ast_loop_separate; ++type) {
		option[type] = isl_ast_build_get_option_domain(build, type);
		for (type2 = isl_ast_loop_atomic; type2 < type; ++type2)
			option[type] = isl_set_subtract(option[type],
						isl_set_copy(option[type2]));
	}

	unroll = option[isl_ast_loop_unroll];
	unroll = isl_set_coalesce(unroll);
	unroll = isl_set_make_disjoint(unroll);
	option[isl_ast_loop_unroll] = unroll;
}

/* Split up the domain at the current depth into disjoint
 * basic sets for which code should be generated separately,
 * based on the user-specified options.
 * Return the list of disjoint basic sets.
 *
 * There are three kinds of domains that we need to keep track of.
 * - the "schedule domain" is the domain of "executed"
 * - the "class domain" is the domain corresponding to the currrent
 *	separation class
 * - the "option domain" is the domain corresponding to one of the options
 *	atomic, unroll or separate
 *
 * We first consider the individial values of the separation classes
 * and split up the domain for each of them separately.
 * Finally, we consider the remainder.  If no separation classes were
 * specified, then we call compute_partial_domains with the universe
 * "class_domain".  Otherwise, we take the "schedule_domain" as "class_domain",
 * with inner dimensions removed.  We do this because we want to
 * avoid computing the complement of the class domains (i.e., the difference
 * between the universe and domains->done).
 */
static __isl_give isl_basic_set_list *compute_domains(
	__isl_keep isl_union_map *executed, __isl_keep isl_ast_build *build)
{
	struct isl_codegen_domains domains;
	isl_ctx *ctx;
	isl_set *domain;
	isl_union_set *schedule_domain;
	isl_set *classes;
	isl_space *space;
	int n_param;
	enum isl_ast_loop_type type;
	int empty;

	if (!executed)
		return NULL;

	ctx = isl_union_map_get_ctx(executed);
	domains.list = isl_basic_set_list_alloc(ctx, 0);

	schedule_domain = isl_union_map_domain(isl_union_map_copy(executed));
	domain = isl_set_from_union_set(schedule_domain);

	compute_domains_init_options(domains.option, build);

	domains.sep_class = isl_ast_build_get_separation_class(build);
	classes = isl_map_range(isl_map_copy(domains.sep_class));
	n_param = isl_set_dim(classes, isl_dim_param);
	classes = isl_set_project_out(classes, isl_dim_param, 0, n_param);

	space = isl_set_get_space(domain);
	domains.build = build;
	domains.schedule_domain = isl_set_copy(domain);
	domains.executed = executed;
	domains.done = isl_set_empty(space);

	if (isl_set_foreach_point(classes, &compute_class_domains, &domains) < 0)
		domains.list = isl_basic_set_list_free(domains.list);
	isl_set_free(classes);

	empty = isl_set_is_empty(domains.done);
	if (empty < 0) {
		domains.list = isl_basic_set_list_free(domains.list);
		domain = isl_set_free(domain);
	} else if (empty) {
		isl_set_free(domain);
		domain = isl_set_universe(isl_set_get_space(domains.done));
	} else {
		domain = isl_ast_build_eliminate(build, domain);
	}
	if (compute_partial_domains(&domains, domain) < 0)
		domains.list = isl_basic_set_list_free(domains.list);

	isl_set_free(domains.schedule_domain);
	isl_set_free(domains.done);
	isl_map_free(domains.sep_class);
	for (type = isl_ast_loop_atomic; type <= isl_ast_loop_separate; ++type)
		isl_set_free(domains.option[type]);

	return domains.list;
}

/* Generate code for a single component, after shifting (if any)
 * has been applied, in case the schedule was specified as a union map.
 *
 * We first split up the domain at the current depth into disjoint
 * basic sets based on the user-specified options.
 * Then we generated code for each of them and concatenate the results.
 */
static __isl_give isl_ast_graft_list *generate_shifted_component_flat(
	__isl_take isl_union_map *executed, __isl_take isl_ast_build *build)
{
	isl_basic_set_list *domain_list;
	isl_ast_graft_list *list = NULL;

	domain_list = compute_domains(executed, build);
	list = generate_parallel_domains(domain_list, executed, build);

	isl_basic_set_list_free(domain_list);
	isl_union_map_free(executed);
	isl_ast_build_free(build);

	return list;
}

/* Generate code for a single component, after shifting (if any)
 * has been applied, in case the schedule was specified as a schedule tree
 * and the separate option was specified.
 *
 * We perform separation on the domain of "executed" and then generate
 * an AST for each of the resulting disjoint basic sets.
 */
static __isl_give isl_ast_graft_list *generate_shifted_component_tree_separate(
	__isl_take isl_union_map *executed, __isl_take isl_ast_build *build)
{
	isl_space *space;
	isl_set *domain;
	isl_basic_set_list *domain_list;
	isl_ast_graft_list *list;

	space = isl_ast_build_get_space(build, 1);
	domain = separate_schedule_domains(space,
					isl_union_map_copy(executed), build);
	domain_list = isl_basic_set_list_from_set(domain);

	list = generate_parallel_domains(domain_list, executed, build);

	isl_basic_set_list_free(domain_list);
	isl_union_map_free(executed);
	isl_ast_build_free(build);

	return list;
}

/* Internal data structure for generate_shifted_component_tree_unroll.
 *
 * "executed" and "build" are inputs to generate_shifted_component_tree_unroll.
 * "list" collects the constructs grafts.
 */
struct isl_ast_unroll_tree_data {
	isl_union_map *executed;
	isl_ast_build *build;
	isl_ast_graft_list *list;
};

/* Initialize data->list to a list of "n" elements.
 */
static int init_unroll_tree(int n, void *user)
{
	struct isl_ast_unroll_tree_data *data = user;
	isl_ctx *ctx;

	ctx = isl_ast_build_get_ctx(data->build);
	data->list = isl_ast_graft_list_alloc(ctx, n);

	return 0;
}

/* Given an iteration of an unrolled domain represented by "bset",
 * generate the corresponding AST and add the result to data->list.
 */
static int do_unroll_tree_iteration(__isl_take isl_basic_set *bset, void *user)
{
	struct isl_ast_unroll_tree_data *data = user;

	data->list = add_node(data->list, isl_union_map_copy(data->executed),
				bset, isl_ast_build_copy(data->build));

	return 0;
}

/* Generate code for a single component, after shifting (if any)
 * has been applied, in case the schedule was specified as a schedule tree
 * and the unroll option was specified.
 *
 * We call foreach_iteration to iterate over the individual values and
 * construct and collect the corresponding grafts in do_unroll_tree_iteration.
 */
static __isl_give isl_ast_graft_list *generate_shifted_component_tree_unroll(
	__isl_take isl_union_map *executed, __isl_take isl_set *domain,
	__isl_take isl_ast_build *build)
{
	struct isl_ast_unroll_tree_data data = { executed, build, NULL };

	if (foreach_iteration(domain, build, &init_unroll_tree,
				&do_unroll_tree_iteration, &data) < 0)
		data.list = isl_ast_graft_list_free(data.list);

	isl_union_map_free(executed);
	isl_ast_build_free(build);

	return data.list;
}

/* Generate code for a single component, after shifting (if any)
 * has been applied, in case the schedule was specified as a schedule tree.
 * In particular, handle the base case where there is either no isolated
 * set or we are within the isolated set (in which case "isolated" is set)
 * or the iterations that precede or follow the isolated set.
 *
 * The schedule domain is broken up or combined into basic sets
 * according to the AST generation option specified in the current
 * schedule node, which may be either atomic, separate, unroll or
 * unspecified.  If the option is unspecified, then we currently simply
 * split the schedule domain into disjoint basic sets.
 *
 * In case the separate option is specified, the AST generation is
 * handled by generate_shifted_component_tree_separate.
 * In the other cases, we need the global schedule domain.
 * In the unroll case, the AST generation is then handled by
 * generate_shifted_component_tree_unroll which needs the actual
 * schedule domain (with divs that may refer to the current dimension)
 * so that stride detection can be performed.
 * In the atomic or unspecified case, inner dimensions and divs involving
 * the current dimensions should be eliminated.
 * The result is then either combined into a single basic set or
 * split up into disjoint basic sets.
 * Finally an AST is generated for each basic set and the results are
 * concatenated.
 */
static __isl_give isl_ast_graft_list *generate_shifted_component_tree_base(
	__isl_take isl_union_map *executed, __isl_take isl_ast_build *build,
	int isolated)
{
	isl_union_set *schedule_domain;
	isl_set *domain;
	isl_basic_set_list *domain_list;
	isl_ast_graft_list *list;
	enum isl_ast_loop_type type;

	type = isl_ast_build_get_loop_type(build, isolated);
	if (type < 0)
		goto error;

	if (type == isl_ast_loop_separate)
		return generate_shifted_component_tree_separate(executed,
								build);

	schedule_domain = isl_union_map_domain(isl_union_map_copy(executed));
	domain = isl_set_from_union_set(schedule_domain);

	if (type == isl_ast_loop_unroll)
		return generate_shifted_component_tree_unroll(executed, domain,
								build);

	domain = isl_ast_build_eliminate(build, domain);
	domain = isl_set_coalesce(domain);

	if (type == isl_ast_loop_atomic) {
		isl_basic_set *hull;
		hull = isl_set_unshifted_simple_hull(domain);
		domain_list = isl_basic_set_list_from_basic_set(hull);
	} else {
		domain = isl_set_make_disjoint(domain);
		domain_list = isl_basic_set_list_from_set(domain);
	}

	list = generate_parallel_domains(domain_list, executed, build);

	isl_basic_set_list_free(domain_list);
	isl_union_map_free(executed);
	isl_ast_build_free(build);

	return list;
error:
	isl_union_map_free(executed);
	isl_ast_build_free(build);
	return NULL;
}

/* Extract out the disjunction imposed by "domain" on the outer
 * schedule dimensions.
 *
 * In particular, remove all inner dimensions from "domain" (including
 * the current dimension) and then remove the constraints that are shared
 * by all disjuncts in the result.
 */
static __isl_give isl_set *extract_disjunction(__isl_take isl_set *domain,
	__isl_keep isl_ast_build *build)
{
	isl_set *hull;
	int depth, dim;

	domain = isl_ast_build_specialize(build, domain);
	depth = isl_ast_build_get_depth(build);
	dim = isl_set_dim(domain, isl_dim_set);
	domain = isl_set_eliminate(domain, isl_dim_set, depth, dim - depth);
	domain = isl_set_remove_unknown_divs(domain);
	hull = isl_set_copy(domain);
	hull = isl_set_from_basic_set(isl_set_unshifted_simple_hull(hull));
	domain = isl_set_gist(domain, hull);

	return domain;
}

/* Add "guard" to the grafts in "list".
 * "build" is the outer AST build, while "sub_build" includes "guard"
 * in its generated domain.
 *
 * First combine the grafts into a single graft and then add the guard.
 * If the list is empty, or if some error occurred, then simply return
 * the list.
 */
static __isl_give isl_ast_graft_list *list_add_guard(
	__isl_take isl_ast_graft_list *list, __isl_keep isl_set *guard,
	__isl_keep isl_ast_build *build, __isl_keep isl_ast_build *sub_build)
{
	isl_ast_graft *graft;

	list = isl_ast_graft_list_fuse(list, sub_build);

	if (isl_ast_graft_list_n_ast_graft(list) != 1)
		return list;

	graft = isl_ast_graft_list_get_ast_graft(list, 0);
	graft = isl_ast_graft_add_guard(graft, isl_set_copy(guard), build);
	list = isl_ast_graft_list_set_ast_graft(list, 0, graft);

	return list;
}

/* Generate code for a single component, after shifting (if any)
 * has been applied, in case the schedule was specified as a schedule tree.
 * In particular, do so for the specified subset of the schedule domain.
 *
 * If we are outside of the isolated part, then "domain" may include
 * a disjunction.  Explicitly generate this disjunction at this point
 * instead of relying on the disjunction getting hoisted back up
 * to this level.
 */
static __isl_give isl_ast_graft_list *generate_shifted_component_tree_part(
	__isl_keep isl_union_map *executed, __isl_take isl_set *domain,
	__isl_keep isl_ast_build *build, int isolated)
{
	isl_union_set *uset;
	isl_ast_graft_list *list;
	isl_ast_build *sub_build;
	int empty;

	uset = isl_union_set_from_set(isl_set_copy(domain));
	executed = isl_union_map_copy(executed);
	executed = isl_union_map_intersect_domain(executed, uset);
	empty = isl_union_map_is_empty(executed);
	if (empty < 0)
		goto error;
	if (empty) {
		isl_ctx *ctx;
		isl_union_map_free(executed);
		isl_set_free(domain);
		ctx = isl_ast_build_get_ctx(build);
		return isl_ast_graft_list_alloc(ctx, 0);
	}

	sub_build = isl_ast_build_copy(build);
	if (!isolated) {
		domain = extract_disjunction(domain, build);
		sub_build = isl_ast_build_restrict_generated(sub_build,
							isl_set_copy(domain));
	}
	list = generate_shifted_component_tree_base(executed,
				isl_ast_build_copy(sub_build), isolated);
	if (!isolated)
		list = list_add_guard(list, domain, build, sub_build);
	isl_ast_build_free(sub_build);
	isl_set_free(domain);
	return list;
error:
	isl_union_map_free(executed);
	isl_set_free(domain);
	return NULL;
}

/* Generate code for a single component, after shifting (if any)
 * has been applied, in case the schedule was specified as a schedule tree.
 *
 * We first check if the user has specified an isolated schedule domain
 * and that we are not already outside of this isolated schedule domain.
 * If so, we break up the schedule domain into iterations that
 * precede the isolated domain, the isolated domain itself,
 * the iterations that follow the isolated domain and
 * the remaining iterations (those that are incomparable
 * to the isolated domain).
 * We generate an AST for each piece and concatenate the results.
 * If no isolated set has been specified, then we generate an
 * AST for the entire inverse schedule.
 */
static __isl_give isl_ast_graft_list *generate_shifted_component_tree(
	__isl_take isl_union_map *executed, __isl_take isl_ast_build *build)
{
	int i, depth;
	int empty, has_isolate;
	isl_space *space;
	isl_union_set *schedule_domain;
	isl_set *domain;
	isl_basic_set *hull;
	isl_set *isolated, *before, *after, *test;
	isl_map *gt, *lt;
	isl_ast_graft_list *list, *res;

	build = isl_ast_build_extract_isolated(build);
	has_isolate = isl_ast_build_has_isolated(build);
	if (has_isolate < 0)
		executed = isl_union_map_free(executed);
	else if (!has_isolate)
		return generate_shifted_component_tree_base(executed, build, 0);

	schedule_domain = isl_union_map_domain(isl_union_map_copy(executed));
	domain = isl_set_from_union_set(schedule_domain);

	isolated = isl_ast_build_get_isolated(build);
	isolated = isl_set_intersect(isolated, isl_set_copy(domain));
	test = isl_ast_build_specialize(build, isl_set_copy(isolated));
	empty = isl_set_is_empty(test);
	isl_set_free(test);
	if (empty < 0)
		goto error;
	if (empty) {
		isl_set_free(isolated);
		isl_set_free(domain);
		return generate_shifted_component_tree_base(executed, build, 0);
	}
	isolated = isl_ast_build_eliminate(build, isolated);
	hull = isl_set_unshifted_simple_hull(isolated);
	isolated = isl_set_from_basic_set(hull);

	depth = isl_ast_build_get_depth(build);
	space = isl_space_map_from_set(isl_set_get_space(isolated));
	gt = isl_map_universe(space);
	for (i = 0; i < depth; ++i)
		gt = isl_map_equate(gt, isl_dim_in, i, isl_dim_out, i);
	gt = isl_map_order_gt(gt, isl_dim_in, depth, isl_dim_out, depth);
	lt = isl_map_reverse(isl_map_copy(gt));
	before = isl_set_apply(isl_set_copy(isolated), gt);
	after = isl_set_apply(isl_set_copy(isolated), lt);

	domain = isl_set_subtract(domain, isl_set_copy(isolated));
	domain = isl_set_subtract(domain, isl_set_copy(before));
	domain = isl_set_subtract(domain, isl_set_copy(after));
	after = isl_set_subtract(after, isl_set_copy(isolated));
	after = isl_set_subtract(after, isl_set_copy(before));
	before = isl_set_subtract(before, isl_set_copy(isolated));

	res = generate_shifted_component_tree_part(executed, before, build, 0);
	list = generate_shifted_component_tree_part(executed, isolated,
						    build, 1);
	res = isl_ast_graft_list_concat(res, list);
	list = generate_shifted_component_tree_part(executed, after, build, 0);
	res = isl_ast_graft_list_concat(res, list);
	list = generate_shifted_component_tree_part(executed, domain, build, 0);
	res = isl_ast_graft_list_concat(res, list);

	isl_union_map_free(executed);
	isl_ast_build_free(build);

	return res;
error:
	isl_set_free(domain);
	isl_set_free(isolated);
	isl_union_map_free(executed);
	isl_ast_build_free(build);
	return NULL;
}

/* Generate code for a single component, after shifting (if any)
 * has been applied.
 *
 * Call generate_shifted_component_tree or generate_shifted_component_flat
 * depending on whether the schedule was specified as a schedule tree.
 */
static __isl_give isl_ast_graft_list *generate_shifted_component(
	__isl_take isl_union_map *executed, __isl_take isl_ast_build *build)
{
	if (isl_ast_build_has_schedule_node(build))
		return generate_shifted_component_tree(executed, build);
	else
		return generate_shifted_component_flat(executed, build);
}

struct isl_set_map_pair {
	isl_set *set;
	isl_map *map;
};

/* Given an array "domain" of isl_set_map_pairs and an array "order"
 * of indices into the "domain" array,
 * return the union of the "map" fields of the elements
 * indexed by the first "n" elements of "order".
 */
static __isl_give isl_union_map *construct_component_executed(
	struct isl_set_map_pair *domain, int *order, int n)
{
	int i;
	isl_map *map;
	isl_union_map *executed;

	map = isl_map_copy(domain[order[0]].map);
	executed = isl_union_map_from_map(map);
	for (i = 1; i < n; ++i) {
		map = isl_map_copy(domain[order[i]].map);
		executed = isl_union_map_add_map(executed, map);
	}

	return executed;
}

/* Generate code for a single component, after shifting (if any)
 * has been applied.
 *
 * The component inverse schedule is specified as the "map" fields
 * of the elements of "domain" indexed by the first "n" elements of "order".
 */
static __isl_give isl_ast_graft_list *generate_shifted_component_from_list(
	struct isl_set_map_pair *domain, int *order, int n,
	__isl_take isl_ast_build *build)
{
	isl_union_map *executed;

	executed = construct_component_executed(domain, order, n);
	return generate_shifted_component(executed, build);
}

/* Does set dimension "pos" of "set" have an obviously fixed value?
 */
static int dim_is_fixed(__isl_keep isl_set *set, int pos)
{
	int fixed;
	isl_val *v;

	v = isl_set_plain_get_val_if_fixed(set, isl_dim_set, pos);
	if (!v)
		return -1;
	fixed = !isl_val_is_nan(v);
	isl_val_free(v);

	return fixed;
}

/* Given an array "domain" of isl_set_map_pairs and an array "order"
 * of indices into the "domain" array,
 * do all (except for at most one) of the "set" field of the elements
 * indexed by the first "n" elements of "order" have a fixed value
 * at position "depth"?
 */
static int at_most_one_non_fixed(struct isl_set_map_pair *domain,
	int *order, int n, int depth)
{
	int i;
	int non_fixed = -1;

	for (i = 0; i < n; ++i) {
		int f;

		f = dim_is_fixed(domain[order[i]].set, depth);
		if (f < 0)
			return -1;
		if (f)
			continue;
		if (non_fixed >= 0)
			return 0;
		non_fixed = i;
	}

	return 1;
}

/* Given an array "domain" of isl_set_map_pairs and an array "order"
 * of indices into the "domain" array,
 * eliminate the inner dimensions from the "set" field of the elements
 * indexed by the first "n" elements of "order", provided the current
 * dimension does not have a fixed value.
 *
 * Return the index of the first element in "order" with a corresponding
 * "set" field that does not have an (obviously) fixed value.
 */
static int eliminate_non_fixed(struct isl_set_map_pair *domain,
	int *order, int n, int depth, __isl_keep isl_ast_build *build)
{
	int i;
	int base = -1;

	for (i = n - 1; i >= 0; --i) {
		int f;
		f = dim_is_fixed(domain[order[i]].set, depth);
		if (f < 0)
			return -1;
		if (f)
			continue;
		domain[order[i]].set = isl_ast_build_eliminate_inner(build,
							domain[order[i]].set);
		base = i;
	}

	return base;
}

/* Given an array "domain" of isl_set_map_pairs and an array "order"
 * of indices into the "domain" array,
 * find the element of "domain" (amongst those indexed by the first "n"
 * elements of "order") with the "set" field that has the smallest
 * value for the current iterator.
 *
 * Note that the domain with the smallest value may depend on the parameters
 * and/or outer loop dimension.  Since the result of this function is only
 * used as heuristic, we only make a reasonable attempt at finding the best
 * domain, one that should work in case a single domain provides the smallest
 * value for the current dimension over all values of the parameters
 * and outer dimensions.
 *
 * In particular, we compute the smallest value of the first domain
 * and replace it by that of any later domain if that later domain
 * has a smallest value that is smaller for at least some value
 * of the parameters and outer dimensions.
 */
static int first_offset(struct isl_set_map_pair *domain, int *order, int n,
	__isl_keep isl_ast_build *build)
{
	int i;
	isl_map *min_first;
	int first = 0;

	min_first = isl_ast_build_map_to_iterator(build,
					isl_set_copy(domain[order[0]].set));
	min_first = isl_map_lexmin(min_first);

	for (i = 1; i < n; ++i) {
		isl_map *min, *test;
		int empty;

		min = isl_ast_build_map_to_iterator(build,
					isl_set_copy(domain[order[i]].set));
		min = isl_map_lexmin(min);
		test = isl_map_copy(min);
		test = isl_map_apply_domain(isl_map_copy(min_first), test);
		test = isl_map_order_lt(test, isl_dim_in, 0, isl_dim_out, 0);
		empty = isl_map_is_empty(test);
		isl_map_free(test);
		if (empty >= 0 && !empty) {
			isl_map_free(min_first);
			first = i;
			min_first = min;
		} else
			isl_map_free(min);

		if (empty < 0)
			break;
	}

	isl_map_free(min_first);

	return i < n ? -1 : first;
}

/* Construct a shifted inverse schedule based on the original inverse schedule,
 * the stride and the offset.
 *
 * The original inverse schedule is specified as the "map" fields
 * of the elements of "domain" indexed by the first "n" elements of "order".
 *
 * "stride" and "offset" are such that the difference
 * between the values of the current dimension of domain "i"
 * and the values of the current dimension for some reference domain are
 * equal to
 *
 *	stride * integer + offset[i]
 *
 * Moreover, 0 <= offset[i] < stride.
 *
 * For each domain, we create a map
 *
 *	{ [..., j, ...] -> [..., j - offset[i], offset[i], ....] }
 *
 * where j refers to the current dimension and the other dimensions are
 * unchanged, and apply this map to the original schedule domain.
 *
 * For example, for the original schedule
 *
 *	{ A[i] -> [2i]: 0 <= i < 10; B[i] -> [2i+1] : 0 <= i < 10 }
 *
 * and assuming the offset is 0 for the A domain and 1 for the B domain,
 * we apply the mapping
 *
 *	{ [j] -> [j, 0] }
 *
 * to the schedule of the "A" domain and the mapping
 *
 *	{ [j - 1] -> [j, 1] }
 *
 * to the schedule of the "B" domain.
 *
 *
 * Note that after the transformation, the differences between pairs
 * of values of the current dimension over all domains are multiples
 * of stride and that we have therefore exposed the stride.
 *
 *
 * To see that the mapping preserves the lexicographic order,
 * first note that each of the individual maps above preserves the order.
 * If the value of the current iterator is j1 in one domain and j2 in another,
 * then if j1 = j2, we know that the same map is applied to both domains
 * and the order is preserved.
 * Otherwise, let us assume, without loss of generality, that j1 < j2.
 * If c1 >= c2 (with c1 and c2 the corresponding offsets), then
 *
 *	j1 - c1 < j2 - c2
 *
 * and the order is preserved.
 * If c1 < c2, then we know
 *
 *	0 <= c2 - c1 < s
 *
 * We also have
 *
 *	j2 - j1 = n * s + r
 *
 * with n >= 0 and 0 <= r < s.
 * In other words, r = c2 - c1.
 * If n > 0, then
 *
 *	j1 - c1 < j2 - c2
 *
 * If n = 0, then
 *
 *	j1 - c1 = j2 - c2
 *
 * and so
 *
 *	(j1 - c1, c1) << (j2 - c2, c2)
 *
 * with "<<" the lexicographic order, proving that the order is preserved
 * in all cases.
 */
static __isl_give isl_union_map *contruct_shifted_executed(
	struct isl_set_map_pair *domain, int *order, int n,
	__isl_keep isl_val *stride, __isl_keep isl_multi_val *offset,
	__isl_take isl_ast_build *build)
{
	int i;
	isl_union_map *executed;
	isl_space *space;
	isl_map *map;
	int depth;
	isl_constraint *c;

	depth = isl_ast_build_get_depth(build);
	space = isl_ast_build_get_space(build, 1);
	executed = isl_union_map_empty(isl_space_copy(space));
	space = isl_space_map_from_set(space);
	map = isl_map_identity(isl_space_copy(space));
	map = isl_map_eliminate(map, isl_dim_out, depth, 1);
	map = isl_map_insert_dims(map, isl_dim_out, depth + 1, 1);
	space = isl_space_insert_dims(space, isl_dim_out, depth + 1, 1);

	c = isl_constraint_alloc_equality(isl_local_space_from_space(space));
	c = isl_constraint_set_coefficient_si(c, isl_dim_in, depth, 1);
	c = isl_constraint_set_coefficient_si(c, isl_dim_out, depth, -1);

	for (i = 0; i < n; ++i) {
		isl_map *map_i;
		isl_val *v;

		v = isl_multi_val_get_val(offset, i);
		if (!v)
			break;
		map_i = isl_map_copy(map);
		map_i = isl_map_fix_val(map_i, isl_dim_out, depth + 1,
					isl_val_copy(v));
		v = isl_val_neg(v);
		c = isl_constraint_set_constant_val(c, v);
		map_i = isl_map_add_constraint(map_i, isl_constraint_copy(c));

		map_i = isl_map_apply_domain(isl_map_copy(domain[order[i]].map),
						map_i);
		executed = isl_union_map_add_map(executed, map_i);
	}

	isl_constraint_free(c);
	isl_map_free(map);

	if (i < n)
		executed = isl_union_map_free(executed);

	return executed;
}

/* Generate code for a single component, after exposing the stride,
 * given that the schedule domain is "shifted strided".
 *
 * The component inverse schedule is specified as the "map" fields
 * of the elements of "domain" indexed by the first "n" elements of "order".
 *
 * The schedule domain being "shifted strided" means that the differences
 * between the values of the current dimension of domain "i"
 * and the values of the current dimension for some reference domain are
 * equal to
 *
 *	stride * integer + offset[i]
 *
 * We first look for the domain with the "smallest" value for the current
 * dimension and adjust the offsets such that the offset of the "smallest"
 * domain is equal to zero.  The other offsets are reduced modulo stride.
 *
 * Based on this information, we construct a new inverse schedule in
 * contruct_shifted_executed that exposes the stride.
 * Since this involves the introduction of a new schedule dimension,
 * the build needs to be changed accodingly.
 * After computing the AST, the newly introduced dimension needs
 * to be removed again from the list of grafts.  We do this by plugging
 * in a mapping that represents the new schedule domain in terms of the
 * old schedule domain.
 */
static __isl_give isl_ast_graft_list *generate_shift_component(
	struct isl_set_map_pair *domain, int *order, int n,
	__isl_keep isl_val *stride, __isl_keep isl_multi_val *offset,
	__isl_take isl_ast_build *build)
{
	isl_ast_graft_list *list;
	int first;
	int depth;
	isl_val *val;
	isl_multi_val *mv;
	isl_space *space;
	isl_multi_aff *ma, *zero;
	isl_union_map *executed;

	depth = isl_ast_build_get_depth(build);

	first = first_offset(domain, order, n, build);
	if (first < 0)
		goto error;

	mv = isl_multi_val_copy(offset);
	val = isl_multi_val_get_val(offset, first);
	val = isl_val_neg(val);
	mv = isl_multi_val_add_val(mv, val);
	mv = isl_multi_val_mod_val(mv, isl_val_copy(stride));

	executed = contruct_shifted_executed(domain, order, n, stride, mv,
						build);
	space = isl_ast_build_get_space(build, 1);
	space = isl_space_map_from_set(space);
	ma = isl_multi_aff_identity(isl_space_copy(space));
	space = isl_space_from_domain(isl_space_domain(space));
	space = isl_space_add_dims(space, isl_dim_out, 1);
	zero = isl_multi_aff_zero(space);
	ma = isl_multi_aff_range_splice(ma, depth + 1, zero);
	build = isl_ast_build_insert_dim(build, depth + 1);
	list = generate_shifted_component(executed, build);

	list = isl_ast_graft_list_preimage_multi_aff(list, ma);

	isl_multi_val_free(mv);

	return list;
error:
	isl_ast_build_free(build);
	return NULL;
}

/* Does any node in the schedule tree rooted at the current schedule node
 * of "build" depend on outer schedule nodes?
 */
static int has_anchored_subtree(__isl_keep isl_ast_build *build)
{
	isl_schedule_node *node;
	int dependent = 0;

	node = isl_ast_build_get_schedule_node(build);
	dependent = isl_schedule_node_is_subtree_anchored(node);
	isl_schedule_node_free(node);

	return dependent;
}

/* Generate code for a single component.
 *
 * The component inverse schedule is specified as the "map" fields
 * of the elements of "domain" indexed by the first "n" elements of "order".
 *
 * This function may modify the "set" fields of "domain".
 *
 * Before proceeding with the actual code generation for the component,
 * we first check if there are any "shifted" strides, meaning that
 * the schedule domains of the individual domains are all strided,
 * but that they have different offsets, resulting in the union
 * of schedule domains not being strided anymore.
 *
 * The simplest example is the schedule
 *
 *	{ A[i] -> [2i]: 0 <= i < 10; B[i] -> [2i+1] : 0 <= i < 10 }
 *
 * Both schedule domains are strided, but their union is not.
 * This function detects such cases and then rewrites the schedule to
 *
 *	{ A[i] -> [2i, 0]: 0 <= i < 10; B[i] -> [2i, 1] : 0 <= i < 10 }
 *
 * In the new schedule, the schedule domains have the same offset (modulo
 * the stride), ensuring that the union of schedule domains is also strided.
 *
 *
 * If there is only a single domain in the component, then there is
 * nothing to do.   Similarly, if the current schedule dimension has
 * a fixed value for almost all domains then there is nothing to be done.
 * In particular, we need at least two domains where the current schedule
 * dimension does not have a fixed value.
 * Finally, in case of a schedule map input,
 * if any of the options refer to the current schedule dimension,
 * then we bail out as well.  It would be possible to reformulate the options
 * in terms of the new schedule domain, but that would introduce constraints
 * that separate the domains in the options and that is something we would
 * like to avoid.
 * In the case of a schedule tree input, we bail out if any of
 * the descendants of the current schedule node refer to outer
 * schedule nodes in any way.
 *
 *
 * To see if there is any shifted stride, we look at the differences
 * between the values of the current dimension in pairs of domains
 * for equal values of outer dimensions.  These differences should be
 * of the form
 *
 *	m x + r
 *
 * with "m" the stride and "r" a constant.  Note that we cannot perform
 * this analysis on individual domains as the lower bound in each domain
 * may depend on parameters or outer dimensions and so the current dimension
 * itself may not have a fixed remainder on division by the stride.
 *
 * In particular, we compare the first domain that does not have an
 * obviously fixed value for the current dimension to itself and all
 * other domains and collect the offsets and the gcd of the strides.
 * If the gcd becomes one, then we failed to find shifted strides.
 * If the gcd is zero, then the differences were all fixed, meaning
 * that some domains had non-obviously fixed values for the current dimension.
 * If all the offsets are the same (for those domains that do not have
 * an obviously fixed value for the current dimension), then we do not
 * apply the transformation.
 * If none of the domains were skipped, then there is nothing to do.
 * If some of them were skipped, then if we apply separation, the schedule
 * domain should get split in pieces with a (non-shifted) stride.
 *
 * Otherwise, we apply a shift to expose the stride in
 * generate_shift_component.
 */
static __isl_give isl_ast_graft_list *generate_component(
	struct isl_set_map_pair *domain, int *order, int n,
	__isl_take isl_ast_build *build)
{
	int i, d;
	int depth;
	isl_ctx *ctx;
	isl_map *map;
	isl_set *deltas;
	isl_val *gcd = NULL;
	isl_multi_val *mv;
	int fixed, skip;
	int base;
	isl_ast_graft_list *list;
	int res = 0;

	depth = isl_ast_build_get_depth(build);

	skip = n == 1;
	if (skip >= 0 && !skip)
		skip = at_most_one_non_fixed(domain, order, n, depth);
	if (skip >= 0 && !skip) {
		if (isl_ast_build_has_schedule_node(build))
			skip = has_anchored_subtree(build);
		else
			skip = isl_ast_build_options_involve_depth(build);
	}
	if (skip < 0)
		goto error;
	if (skip)
		return generate_shifted_component_from_list(domain,
							    order, n, build);

	base = eliminate_non_fixed(domain, order, n, depth, build);
	if (base < 0)
		goto error;

	ctx = isl_ast_build_get_ctx(build);

	mv = isl_multi_val_zero(isl_space_set_alloc(ctx, 0, n));

	fixed = 1;
	for (i = 0; i < n; ++i) {
		isl_val *r, *m;

		map = isl_map_from_domain_and_range(
					isl_set_copy(domain[order[base]].set),
					isl_set_copy(domain[order[i]].set));
		for (d = 0; d < depth; ++d)
			map = isl_map_equate(map, isl_dim_in, d,
						    isl_dim_out, d);
		deltas = isl_map_deltas(map);
		res = isl_set_dim_residue_class_val(deltas, depth, &m, &r);
		isl_set_free(deltas);
		if (res < 0)
			break;

		if (i == 0)
			gcd = m;
		else
			gcd = isl_val_gcd(gcd, m);
		if (isl_val_is_one(gcd)) {
			isl_val_free(r);
			break;
		}
		mv = isl_multi_val_set_val(mv, i, r);

		res = dim_is_fixed(domain[order[i]].set, depth);
		if (res < 0)
			break;
		if (res)
			continue;

		if (fixed && i > base) {
			isl_val *a, *b;
			a = isl_multi_val_get_val(mv, i);
			b = isl_multi_val_get_val(mv, base);
			if (isl_val_ne(a, b))
				fixed = 0;
			isl_val_free(a);
			isl_val_free(b);
		}
	}

	if (res < 0 || !gcd) {
		isl_ast_build_free(build);
		list = NULL;
	} else if (i < n || fixed || isl_val_is_zero(gcd)) {
		list = generate_shifted_component_from_list(domain,
							    order, n, build);
	} else {
		list = generate_shift_component(domain, order, n, gcd, mv,
						build);
	}

	isl_val_free(gcd);
	isl_multi_val_free(mv);

	return list;
error:
	isl_ast_build_free(build);
	return NULL;
}

/* Store both "map" itself and its domain in the
 * structure pointed to by *next and advance to the next array element.
 */
static isl_stat extract_domain(__isl_take isl_map *map, void *user)
{
	struct isl_set_map_pair **next = user;

	(*next)->map = isl_map_copy(map);
	(*next)->set = isl_map_domain(map);
	(*next)++;

	return isl_stat_ok;
}

static int after_in_tree(__isl_keep isl_union_map *umap,
	__isl_keep isl_schedule_node *node);

/* Is any domain element of "umap" scheduled after any of
 * the corresponding image elements by the tree rooted at
 * the child of "node"?
 */
static int after_in_child(__isl_keep isl_union_map *umap,
	__isl_keep isl_schedule_node *node)
{
	isl_schedule_node *child;
	int after;

	child = isl_schedule_node_get_child(node, 0);
	after = after_in_tree(umap, child);
	isl_schedule_node_free(child);

	return after;
}

/* Is any domain element of "umap" scheduled after any of
 * the corresponding image elements by the tree rooted at
 * the band node "node"?
 *
 * We first check if any domain element is scheduled after any
 * of the corresponding image elements by the band node itself.
 * If not, we restrict "map" to those pairs of element that
 * are scheduled together by the band node and continue with
 * the child of the band node.
 * If there are no such pairs then the map passed to after_in_child
 * will be empty causing it to return 0.
 */
static int after_in_band(__isl_keep isl_union_map *umap,
	__isl_keep isl_schedule_node *node)
{
	isl_multi_union_pw_aff *mupa;
	isl_union_map *partial, *test, *gt, *universe, *umap1, *umap2;
	isl_union_set *domain, *range;
	isl_space *space;
	int empty;
	int after;

	if (isl_schedule_node_band_n_member(node) == 0)
		return after_in_child(umap, node);

	mupa = isl_schedule_node_band_get_partial_schedule(node);
	space = isl_multi_union_pw_aff_get_space(mupa);
	partial = isl_union_map_from_multi_union_pw_aff(mupa);
	test = isl_union_map_copy(umap);
	test = isl_union_map_apply_domain(test, isl_union_map_copy(partial));
	test = isl_union_map_apply_range(test, isl_union_map_copy(partial));
	gt = isl_union_map_from_map(isl_map_lex_gt(space));
	test = isl_union_map_intersect(test, gt);
	empty = isl_union_map_is_empty(test);
	isl_union_map_free(test);

	if (empty < 0 || !empty) {
		isl_union_map_free(partial);
		return empty < 0 ? -1 : 1;
	}

	universe = isl_union_map_universe(isl_union_map_copy(umap));
	domain = isl_union_map_domain(isl_union_map_copy(universe));
	range = isl_union_map_range(universe);
	umap1 = isl_union_map_copy(partial);
	umap1 = isl_union_map_intersect_domain(umap1, domain);
	umap2 = isl_union_map_intersect_domain(partial, range);
	test = isl_union_map_apply_range(umap1, isl_union_map_reverse(umap2));
	test = isl_union_map_intersect(test, isl_union_map_copy(umap));
	after = after_in_child(test, node);
	isl_union_map_free(test);
	return after;
}

/* Is any domain element of "umap" scheduled after any of
 * the corresponding image elements by the tree rooted at
 * the context node "node"?
 *
 * The context constraints apply to the schedule domain,
 * so we cannot apply them directly to "umap", which contains
 * pairs of statement instances.  Instead, we add them
 * to the range of the prefix schedule for both domain and
 * range of "umap".
 */
static int after_in_context(__isl_keep isl_union_map *umap,
	__isl_keep isl_schedule_node *node)
{
	isl_union_map *prefix, *universe, *umap1, *umap2;
	isl_union_set *domain, *range;
	isl_set *context;
	int after;

	umap = isl_union_map_copy(umap);
	context = isl_schedule_node_context_get_context(node);
	prefix = isl_schedule_node_get_prefix_schedule_union_map(node);
	universe = isl_union_map_universe(isl_union_map_copy(umap));
	domain = isl_union_map_domain(isl_union_map_copy(universe));
	range = isl_union_map_range(universe);
	umap1 = isl_union_map_copy(prefix);
	umap1 = isl_union_map_intersect_domain(umap1, domain);
	umap2 = isl_union_map_intersect_domain(prefix, range);
	umap1 = isl_union_map_intersect_range(umap1,
					    isl_union_set_from_set(context));
	umap1 = isl_union_map_apply_range(umap1, isl_union_map_reverse(umap2));
	umap = isl_union_map_intersect(umap, umap1);

	after = after_in_child(umap, node);

	isl_union_map_free(umap);

	return after;
}

/* Is any domain element of "umap" scheduled after any of
 * the corresponding image elements by the tree rooted at
 * the expansion node "node"?
 *
 * We apply the expansion to domain and range of "umap" and
 * continue with its child.
 */
static int after_in_expansion(__isl_keep isl_union_map *umap,
	__isl_keep isl_schedule_node *node)
{
	isl_union_map *expansion;
	int after;

	expansion = isl_schedule_node_expansion_get_expansion(node);
	umap = isl_union_map_copy(umap);
	umap = isl_union_map_apply_domain(umap, isl_union_map_copy(expansion));
	umap = isl_union_map_apply_range(umap, expansion);

	after = after_in_child(umap, node);

	isl_union_map_free(umap);

	return after;
}

/* Is any domain element of "umap" scheduled after any of
 * the corresponding image elements by the tree rooted at
 * the extension node "node"?
 *
 * Since the extension node may add statement instances before or
 * after the pairs of statement instances in "umap", we return 1
 * to ensure that these pairs are not broken up.
 */
static int after_in_extension(__isl_keep isl_union_map *umap,
	__isl_keep isl_schedule_node *node)
{
	return 1;
}

/* Is any domain element of "umap" scheduled after any of
 * the corresponding image elements by the tree rooted at
 * the filter node "node"?
 *
 * We intersect domain and range of "umap" with the filter and
 * continue with its child.
 */
static int after_in_filter(__isl_keep isl_union_map *umap,
	__isl_keep isl_schedule_node *node)
{
	isl_union_set *filter;
	int after;

	umap = isl_union_map_copy(umap);
	filter = isl_schedule_node_filter_get_filter(node);
	umap = isl_union_map_intersect_domain(umap, isl_union_set_copy(filter));
	umap = isl_union_map_intersect_range(umap, filter);

	after = after_in_child(umap, node);

	isl_union_map_free(umap);

	return after;
}

/* Is any domain element of "umap" scheduled after any of
 * the corresponding image elements by the tree rooted at
 * the set node "node"?
 *
 * This is only the case if this condition holds in any
 * of the (filter) children of the set node.
 * In particular, if the domain and the range of "umap"
 * are contained in different children, then the condition
 * does not hold.
 */
static int after_in_set(__isl_keep isl_union_map *umap,
	__isl_keep isl_schedule_node *node)
{
	int i, n;

	n = isl_schedule_node_n_children(node);
	for (i = 0; i < n; ++i) {
		isl_schedule_node *child;
		int after;

		child = isl_schedule_node_get_child(node, i);
		after = after_in_tree(umap, child);
		isl_schedule_node_free(child);

		if (after < 0 || after)
			return after;
	}

	return 0;
}

/* Return the filter of child "i" of "node".
 */
static __isl_give isl_union_set *child_filter(
	__isl_keep isl_schedule_node *node, int i)
{
	isl_schedule_node *child;
	isl_union_set *filter;

	child = isl_schedule_node_get_child(node, i);
	filter = isl_schedule_node_filter_get_filter(child);
	isl_schedule_node_free(child);

	return filter;
}

/* Is any domain element of "umap" scheduled after any of
 * the corresponding image elements by the tree rooted at
 * the sequence node "node"?
 *
 * This happens in particular if any domain element is
 * contained in a later child than one containing a range element or
 * if the condition holds within a given child in the sequence.
 * The later part of the condition is checked by after_in_set.
 */
static int after_in_sequence(__isl_keep isl_union_map *umap,
	__isl_keep isl_schedule_node *node)
{
	int i, j, n;
	isl_union_map *umap_i;
	int empty, after = 0;

	n = isl_schedule_node_n_children(node);
	for (i = 1; i < n; ++i) {
		isl_union_set *filter_i;

		umap_i = isl_union_map_copy(umap);
		filter_i = child_filter(node, i);
		umap_i = isl_union_map_intersect_domain(umap_i, filter_i);
		empty = isl_union_map_is_empty(umap_i);
		if (empty < 0)
			goto error;
		if (empty) {
			isl_union_map_free(umap_i);
			continue;
		}

		for (j = 0; j < i; ++j) {
			isl_union_set *filter_j;
			isl_union_map *umap_ij;

			umap_ij = isl_union_map_copy(umap_i);
			filter_j = child_filter(node, j);
			umap_ij = isl_union_map_intersect_range(umap_ij,
								filter_j);
			empty = isl_union_map_is_empty(umap_ij);
			isl_union_map_free(umap_ij);

			if (empty < 0)
				goto error;
			if (!empty)
				after = 1;
			if (after)
				break;
		}

		isl_union_map_free(umap_i);
		if (after)
			break;
	}

	if (after < 0 || after)
		return after;

	return after_in_set(umap, node);
error:
	isl_union_map_free(umap_i);
	return -1;
}

/* Is any domain element of "umap" scheduled after any of
 * the corresponding image elements by the tree rooted at "node"?
 *
 * If "umap" is empty, then clearly there is no such element.
 * Otherwise, consider the different types of nodes separately.
 */
static int after_in_tree(__isl_keep isl_union_map *umap,
	__isl_keep isl_schedule_node *node)
{
	int empty;
	enum isl_schedule_node_type type;

	empty = isl_union_map_is_empty(umap);
	if (empty < 0)
		return -1;
	if (empty)
		return 0;
	if (!node)
		return -1;

	type = isl_schedule_node_get_type(node);
	switch (type) {
	case isl_schedule_node_error:
		return -1;
	case isl_schedule_node_leaf:
		return 0;
	case isl_schedule_node_band:
		return after_in_band(umap, node);
	case isl_schedule_node_domain:
		isl_die(isl_schedule_node_get_ctx(node), isl_error_internal,
			"unexpected internal domain node", return -1);
	case isl_schedule_node_context:
		return after_in_context(umap, node);
	case isl_schedule_node_expansion:
		return after_in_expansion(umap, node);
	case isl_schedule_node_extension:
		return after_in_extension(umap, node);
	case isl_schedule_node_filter:
		return after_in_filter(umap, node);
	case isl_schedule_node_guard:
	case isl_schedule_node_mark:
		return after_in_child(umap, node);
	case isl_schedule_node_set:
		return after_in_set(umap, node);
	case isl_schedule_node_sequence:
		return after_in_sequence(umap, node);
	}

	return 1;
}

/* Is any domain element of "map1" scheduled after any domain
 * element of "map2" by the subtree underneath the current band node,
 * while at the same time being scheduled together by the current
 * band node, i.e., by "map1" and "map2?
 *
 * If the child of the current band node is a leaf, then
 * no element can be scheduled after any other element.
 *
 * Otherwise, we construct a relation between domain elements
 * of "map1" and domain elements of "map2" that are scheduled
 * together and then check if the subtree underneath the current
 * band node determines their relative order.
 */
static int after_in_subtree(__isl_keep isl_ast_build *build,
	__isl_keep isl_map *map1, __isl_keep isl_map *map2)
{
	isl_schedule_node *node;
	isl_map *map;
	isl_union_map *umap;
	int after;

	node = isl_ast_build_get_schedule_node(build);
	if (!node)
		return -1;
	node = isl_schedule_node_child(node, 0);
	if (isl_schedule_node_get_type(node) == isl_schedule_node_leaf) {
		isl_schedule_node_free(node);
		return 0;
	}
	map = isl_map_copy(map2);
	map = isl_map_apply_domain(map, isl_map_copy(map1));
	umap = isl_union_map_from_map(map);
	after = after_in_tree(umap, node);
	isl_union_map_free(umap);
	isl_schedule_node_free(node);
	return after;
}

/* Internal data for any_scheduled_after.
 *
 * "build" is the build in which the AST is constructed.
 * "depth" is the number of loops that have already been generated
 * "group_coscheduled" is a local copy of options->ast_build_group_coscheduled
 * "domain" is an array of set-map pairs corresponding to the different
 * iteration domains.  The set is the schedule domain, i.e., the domain
 * of the inverse schedule, while the map is the inverse schedule itself.
 */
struct isl_any_scheduled_after_data {
	isl_ast_build *build;
	int depth;
	int group_coscheduled;
	struct isl_set_map_pair *domain;
};

/* Is any element of domain "i" scheduled after any element of domain "j"
 * (for a common iteration of the first data->depth loops)?
 *
 * data->domain[i].set contains the domain of the inverse schedule
 * for domain "i", i.e., elements in the schedule domain.
 *
 * If we are inside a band of a schedule tree and there is a pair
 * of elements in the two domains that is schedule together by
 * the current band, then we check if any element of "i" may be schedule
 * after element of "j" by the descendants of the band node.
 *
 * If data->group_coscheduled is set, then we also return 1 if there
 * is any pair of elements in the two domains that are scheduled together.
 */
static isl_bool any_scheduled_after(int i, int j, void *user)
{
	struct isl_any_scheduled_after_data *data = user;
	int dim = isl_set_dim(data->domain[i].set, isl_dim_set);
	int pos;

	for (pos = data->depth; pos < dim; ++pos) {
		int follows;

		follows = isl_set_follows_at(data->domain[i].set,
						data->domain[j].set, pos);

		if (follows < -1)
			return isl_bool_error;
		if (follows > 0)
			return isl_bool_true;
		if (follows < 0)
			return isl_bool_false;
	}

	if (isl_ast_build_has_schedule_node(data->build)) {
		int after;

		after = after_in_subtree(data->build, data->domain[i].map,
					    data->domain[j].map);
		if (after < 0 || after)
			return after;
	}

	return data->group_coscheduled;
}

/* Look for independent components at the current depth and generate code
 * for each component separately.  The resulting lists of grafts are
 * merged in an attempt to combine grafts with identical guards.
 *
 * Code for two domains can be generated separately if all the elements
 * of one domain are scheduled before (or together with) all the elements
 * of the other domain.  We therefore consider the graph with as nodes
 * the domains and an edge between two nodes if any element of the first
 * node is scheduled after any element of the second node.
 * If the ast_build_group_coscheduled is set, then we also add an edge if
 * there is any pair of elements in the two domains that are scheduled
 * together.
 * Code is then generated (by generate_component)
 * for each of the strongly connected components in this graph
 * in their topological order.
 *
 * Since the test is performed on the domain of the inverse schedules of
 * the different domains, we precompute these domains and store
 * them in data.domain.
 */
static __isl_give isl_ast_graft_list *generate_components(
	__isl_take isl_union_map *executed, __isl_take isl_ast_build *build)
{
	int i;
	isl_ctx *ctx = isl_ast_build_get_ctx(build);
	int n = isl_union_map_n_map(executed);
	struct isl_any_scheduled_after_data data;
	struct isl_set_map_pair *next;
	struct isl_tarjan_graph *g = NULL;
	isl_ast_graft_list *list = NULL;
	int n_domain = 0;

	data.domain = isl_calloc_array(ctx, struct isl_set_map_pair, n);
	if (!data.domain)
		goto error;
	n_domain = n;

	next = data.domain;
	if (isl_union_map_foreach_map(executed, &extract_domain, &next) < 0)
		goto error;

	if (!build)
		goto error;
	data.build = build;
	data.depth = isl_ast_build_get_depth(build);
	data.group_coscheduled = isl_options_get_ast_build_group_coscheduled(ctx);
	g = isl_tarjan_graph_init(ctx, n, &any_scheduled_after, &data);
	if (!g)
		goto error;

	list = isl_ast_graft_list_alloc(ctx, 0);

	i = 0;
	while (list && n) {
		isl_ast_graft_list *list_c;
		int first = i;

		if (g->order[i] == -1)
			isl_die(ctx, isl_error_internal, "cannot happen",
				goto error);
		++i; --n;
		while (g->order[i] != -1) {
			++i; --n;
		}

		list_c = generate_component(data.domain,
					    g->order + first, i - first,
					    isl_ast_build_copy(build));
		list = isl_ast_graft_list_merge(list, list_c, build);

		++i;
	}

	if (0)
error:		list = isl_ast_graft_list_free(list);
	isl_tarjan_graph_free(g);
	for (i = 0; i < n_domain; ++i) {
		isl_map_free(data.domain[i].map);
		isl_set_free(data.domain[i].set);
	}
	free(data.domain);
	isl_union_map_free(executed);
	isl_ast_build_free(build);

	return list;
}

/* Generate code for the next level (and all inner levels).
 *
 * If "executed" is empty, i.e., no code needs to be generated,
 * then we return an empty list.
 *
 * If we have already generated code for all loop levels, then we pass
 * control to generate_inner_level.
 *
 * If "executed" lives in a single space, i.e., if code needs to be
 * generated for a single domain, then there can only be a single
 * component and we go directly to generate_shifted_component.
 * Otherwise, we call generate_components to detect the components
 * and to call generate_component on each of them separately.
 */
static __isl_give isl_ast_graft_list *generate_next_level(
	__isl_take isl_union_map *executed, __isl_take isl_ast_build *build)
{
	int depth;

	if (!build || !executed)
		goto error;

	if (isl_union_map_is_empty(executed)) {
		isl_ctx *ctx = isl_ast_build_get_ctx(build);
		isl_union_map_free(executed);
		isl_ast_build_free(build);
		return isl_ast_graft_list_alloc(ctx, 0);
	}

	depth = isl_ast_build_get_depth(build);
	if (depth >= isl_ast_build_dim(build, isl_dim_set))
		return generate_inner_level(executed, build);

	if (isl_union_map_n_map(executed) == 1)
		return generate_shifted_component(executed, build);

	return generate_components(executed, build);
error:
	isl_union_map_free(executed);
	isl_ast_build_free(build);
	return NULL;
}

/* Internal data structure used by isl_ast_build_node_from_schedule_map.
 * internal, executed and build are the inputs to generate_code.
 * list collects the output.
 */
struct isl_generate_code_data {
	int internal;
	isl_union_map *executed;
	isl_ast_build *build;

	isl_ast_graft_list *list;
};

/* Given an inverse schedule in terms of the external build schedule, i.e.,
 *
 *	[E -> S] -> D
 *
 * with E the external build schedule and S the additional schedule "space",
 * reformulate the inverse schedule in terms of the internal schedule domain,
 * i.e., return
 *
 *	[I -> S] -> D
 *
 * We first obtain a mapping
 *
 *	I -> E
 *
 * take the inverse and the product with S -> S, resulting in
 *
 *	[I -> S] -> [E -> S]
 *
 * Applying the map to the input produces the desired result.
 */
static __isl_give isl_union_map *internal_executed(
	__isl_take isl_union_map *executed, __isl_keep isl_space *space,
	__isl_keep isl_ast_build *build)
{
	isl_map *id, *proj;

	proj = isl_ast_build_get_schedule_map(build);
	proj = isl_map_reverse(proj);
	space = isl_space_map_from_set(isl_space_copy(space));
	id = isl_map_identity(space);
	proj = isl_map_product(proj, id);
	executed = isl_union_map_apply_domain(executed,
						isl_union_map_from_map(proj));
	return executed;
}

/* Generate an AST that visits the elements in the range of data->executed
 * in the relative order specified by the corresponding domain element(s)
 * for those domain elements that belong to "set".
 * Add the result to data->list.
 *
 * The caller ensures that "set" is a universe domain.
 * "space" is the space of the additional part of the schedule.
 * It is equal to the space of "set" if build->domain is parametric.
 * Otherwise, it is equal to the range of the wrapped space of "set".
 *
 * If the build space is not parametric and
 * if isl_ast_build_node_from_schedule_map
 * was called from an outside user (data->internal not set), then
 * the (inverse) schedule refers to the external build domain and needs to
 * be transformed to refer to the internal build domain.
 *
 * If the build space is parametric, then we add some of the parameter
 * constraints to the executed relation.  Adding these constraints
 * allows for an earlier detection of conflicts in some cases.
 * However, we do not want to divide the executed relation into
 * more disjuncts than necessary.  We therefore approximate
 * the constraints on the parameters by a single disjunct set.
 *
 * The build is extended to include the additional part of the schedule.
 * If the original build space was not parametric, then the options
 * in data->build refer only to the additional part of the schedule
 * and they need to be adjusted to refer to the complete AST build
 * domain.
 *
 * After having adjusted inverse schedule and build, we start generating
 * code with the outer loop of the current code generation
 * in generate_next_level.
 *
 * If the original build space was not parametric, we undo the embedding
 * on the resulting isl_ast_node_list so that it can be used within
 * the outer AST build.
 */
static isl_stat generate_code_in_space(struct isl_generate_code_data *data,
	__isl_take isl_set *set, __isl_take isl_space *space)
{
	isl_union_map *executed;
	isl_ast_build *build;
	isl_ast_graft_list *list;
	int embed;

	executed = isl_union_map_copy(data->executed);
	executed = isl_union_map_intersect_domain(executed,
						 isl_union_set_from_set(set));

	embed = !isl_set_is_params(data->build->domain);
	if (embed && !data->internal)
		executed = internal_executed(executed, space, data->build);
	if (!embed) {
		isl_set *domain;
		domain = isl_ast_build_get_domain(data->build);
		domain = isl_set_from_basic_set(isl_set_simple_hull(domain));
		executed = isl_union_map_intersect_params(executed, domain);
	}

	build = isl_ast_build_copy(data->build);
	build = isl_ast_build_product(build, space);

	list = generate_next_level(executed, build);

	list = isl_ast_graft_list_unembed(list, embed);

	data->list = isl_ast_graft_list_concat(data->list, list);

	return isl_stat_ok;
}

/* Generate an AST that visits the elements in the range of data->executed
 * in the relative order specified by the corresponding domain element(s)
 * for those domain elements that belong to "set".
 * Add the result to data->list.
 *
 * The caller ensures that "set" is a universe domain.
 *
 * If the build space S is not parametric, then the space of "set"
 * need to be a wrapped relation with S as domain.  That is, it needs
 * to be of the form
 *
 *	[S -> T]
 *
 * Check this property and pass control to generate_code_in_space
 * passing along T.
 * If the build space is not parametric, then T is the space of "set".
 */
static isl_stat generate_code_set(__isl_take isl_set *set, void *user)
{
	struct isl_generate_code_data *data = user;
	isl_space *space, *build_space;
	int is_domain;

	space = isl_set_get_space(set);

	if (isl_set_is_params(data->build->domain))
		return generate_code_in_space(data, set, space);

	build_space = isl_ast_build_get_space(data->build, data->internal);
	space = isl_space_unwrap(space);
	is_domain = isl_space_is_domain(build_space, space);
	isl_space_free(build_space);
	space = isl_space_range(space);

	if (is_domain < 0)
		goto error;
	if (!is_domain)
		isl_die(isl_set_get_ctx(set), isl_error_invalid,
			"invalid nested schedule space", goto error);

	return generate_code_in_space(data, set, space);
error:
	isl_set_free(set);
	isl_space_free(space);
	return isl_stat_error;
}

/* Generate an AST that visits the elements in the range of "executed"
 * in the relative order specified by the corresponding domain element(s).
 *
 * "build" is an isl_ast_build that has either been constructed by
 * isl_ast_build_from_context or passed to a callback set by
 * isl_ast_build_set_create_leaf.
 * In the first case, the space of the isl_ast_build is typically
 * a parametric space, although this is currently not enforced.
 * In the second case, the space is never a parametric space.
 * If the space S is not parametric, then the domain space(s) of "executed"
 * need to be wrapped relations with S as domain.
 *
 * If the domain of "executed" consists of several spaces, then an AST
 * is generated for each of them (in arbitrary order) and the results
 * are concatenated.
 *
 * If "internal" is set, then the domain "S" above refers to the internal
 * schedule domain representation.  Otherwise, it refers to the external
 * representation, as returned by isl_ast_build_get_schedule_space.
 *
 * We essentially run over all the spaces in the domain of "executed"
 * and call generate_code_set on each of them.
 */
static __isl_give isl_ast_graft_list *generate_code(
	__isl_take isl_union_map *executed, __isl_take isl_ast_build *build,
	int internal)
{
	isl_ctx *ctx;
	struct isl_generate_code_data data = { 0 };
	isl_space *space;
	isl_union_set *schedule_domain;
	isl_union_map *universe;

	if (!build)
		goto error;
	space = isl_ast_build_get_space(build, 1);
	space = isl_space_align_params(space,
				    isl_union_map_get_space(executed));
	space = isl_space_align_params(space,
				    isl_union_map_get_space(build->options));
	build = isl_ast_build_align_params(build, isl_space_copy(space));
	executed = isl_union_map_align_params(executed, space);
	if (!executed || !build)
		goto error;

	ctx = isl_ast_build_get_ctx(build);

	data.internal = internal;
	data.executed = executed;
	data.build = build;
	data.list = isl_ast_graft_list_alloc(ctx, 0);

	universe = isl_union_map_universe(isl_union_map_copy(executed));
	schedule_domain = isl_union_map_domain(universe);
	if (isl_union_set_foreach_set(schedule_domain, &generate_code_set,
					&data) < 0)
		data.list = isl_ast_graft_list_free(data.list);

	isl_union_set_free(schedule_domain);
	isl_union_map_free(executed);

	isl_ast_build_free(build);
	return data.list;
error:
	isl_union_map_free(executed);
	isl_ast_build_free(build);
	return NULL;
}

/* Generate an AST that visits the elements in the domain of "schedule"
 * in the relative order specified by the corresponding image element(s).
 *
 * "build" is an isl_ast_build that has either been constructed by
 * isl_ast_build_from_context or passed to a callback set by
 * isl_ast_build_set_create_leaf.
 * In the first case, the space of the isl_ast_build is typically
 * a parametric space, although this is currently not enforced.
 * In the second case, the space is never a parametric space.
 * If the space S is not parametric, then the range space(s) of "schedule"
 * need to be wrapped relations with S as domain.
 *
 * If the range of "schedule" consists of several spaces, then an AST
 * is generated for each of them (in arbitrary order) and the results
 * are concatenated.
 *
 * We first initialize the local copies of the relevant options.
 * We do this here rather than when the isl_ast_build is created
 * because the options may have changed between the construction
 * of the isl_ast_build and the call to isl_generate_code.
 *
 * The main computation is performed on an inverse schedule (with
 * the schedule domain in the domain and the elements to be executed
 * in the range) called "executed".
 */
__isl_give isl_ast_node *isl_ast_build_node_from_schedule_map(
	__isl_keep isl_ast_build *build, __isl_take isl_union_map *schedule)
{
	isl_ast_graft_list *list;
	isl_ast_node *node;
	isl_union_map *executed;

	build = isl_ast_build_copy(build);
	build = isl_ast_build_set_single_valued(build, 0);
	schedule = isl_union_map_coalesce(schedule);
	schedule = isl_union_map_remove_redundancies(schedule);
	executed = isl_union_map_reverse(schedule);
	list = generate_code(executed, isl_ast_build_copy(build), 0);
	node = isl_ast_node_from_graft_list(list, build);
	isl_ast_build_free(build);

	return node;
}

/* The old name for isl_ast_build_node_from_schedule_map.
 * It is being kept for backward compatibility, but
 * it will be removed in the future.
 */
__isl_give isl_ast_node *isl_ast_build_ast_from_schedule(
	__isl_keep isl_ast_build *build, __isl_take isl_union_map *schedule)
{
	return isl_ast_build_node_from_schedule_map(build, schedule);
}

/* Generate an AST that visits the elements in the domain of "executed"
 * in the relative order specified by the band node "node" and its descendants.
 *
 * The relation "executed" maps the outer generated loop iterators
 * to the domain elements executed by those iterations.
 *
 * If the band is empty, we continue with its descendants.
 * Otherwise, we extend the build and the inverse schedule with
 * the additional space/partial schedule and continue generating
 * an AST in generate_next_level.
 * As soon as we have extended the inverse schedule with the additional
 * partial schedule, we look for equalities that may exists between
 * the old and the new part.
 */
static __isl_give isl_ast_graft_list *build_ast_from_band(
	__isl_take isl_ast_build *build, __isl_take isl_schedule_node *node,
	__isl_take isl_union_map *executed)
{
	isl_space *space;
	isl_multi_union_pw_aff *extra;
	isl_union_map *extra_umap;
	isl_ast_graft_list *list;
	unsigned n1, n2;

	if (!build || !node || !executed)
		goto error;

	if (isl_schedule_node_band_n_member(node) == 0)
		return build_ast_from_child(build, node, executed);

	extra = isl_schedule_node_band_get_partial_schedule(node);
	extra = isl_multi_union_pw_aff_align_params(extra,
				isl_ast_build_get_space(build, 1));
	space = isl_multi_union_pw_aff_get_space(extra);

	extra_umap = isl_union_map_from_multi_union_pw_aff(extra);
	extra_umap = isl_union_map_reverse(extra_umap);

	executed = isl_union_map_domain_product(executed, extra_umap);
	executed = isl_union_map_detect_equalities(executed);

	n1 = isl_ast_build_dim(build, isl_dim_param);
	build = isl_ast_build_product(build, space);
	n2 = isl_ast_build_dim(build, isl_dim_param);
	if (n2 > n1)
		isl_die(isl_ast_build_get_ctx(build), isl_error_invalid,
			"band node is not allowed to introduce new parameters",
			build = isl_ast_build_free(build));
	build = isl_ast_build_set_schedule_node(build, node);

	list = generate_next_level(executed, build);

	list = isl_ast_graft_list_unembed(list, 1);

	return list;
error:
	isl_schedule_node_free(node);
	isl_union_map_free(executed);
	isl_ast_build_free(build);
	return NULL;
}

/* Hoist a list of grafts (in practice containing a single graft)
 * from "sub_build" (which includes extra context information)
 * to "build".
 *
 * In particular, project out all additional parameters introduced
 * by the context node from the enforced constraints and the guard
 * of the single graft.
 */
static __isl_give isl_ast_graft_list *hoist_out_of_context(
	__isl_take isl_ast_graft_list *list, __isl_keep isl_ast_build *build,
	__isl_keep isl_ast_build *sub_build)
{
	isl_ast_graft *graft;
	isl_basic_set *enforced;
	isl_set *guard;
	unsigned n_param, extra_param;

	if (!build || !sub_build)
		return isl_ast_graft_list_free(list);

	n_param = isl_ast_build_dim(build, isl_dim_param);
	extra_param = isl_ast_build_dim(sub_build, isl_dim_param);

	if (extra_param == n_param)
		return list;

	extra_param -= n_param;
	enforced = isl_ast_graft_list_extract_shared_enforced(list, sub_build);
	enforced = isl_basic_set_project_out(enforced, isl_dim_param,
							n_param, extra_param);
	enforced = isl_basic_set_remove_unknown_divs(enforced);
	guard = isl_ast_graft_list_extract_hoistable_guard(list, sub_build);
	guard = isl_set_remove_divs_involving_dims(guard, isl_dim_param,
							n_param, extra_param);
	guard = isl_set_project_out(guard, isl_dim_param, n_param, extra_param);
	guard = isl_set_compute_divs(guard);
	graft = isl_ast_graft_alloc_from_children(list, guard, enforced,
							build, sub_build);
	list = isl_ast_graft_list_from_ast_graft(graft);

	return list;
}

/* Generate an AST that visits the elements in the domain of "executed"
 * in the relative order specified by the context node "node"
 * and its descendants.
 *
 * The relation "executed" maps the outer generated loop iterators
 * to the domain elements executed by those iterations.
 *
 * The context node may introduce additional parameters as well as
 * constraints on the outer schedule dimenions or original parameters.
 *
 * We add the extra parameters to a new build and the context
 * constraints to both the build and (as a single disjunct)
 * to the domain of "executed".  Since the context constraints
 * are specified in terms of the input schedule, we first need
 * to map them to the internal schedule domain.
 *
 * After constructing the AST from the descendants of "node",
 * we combine the list of grafts into a single graft within
 * the new build, in order to be able to exploit the additional
 * context constraints during this combination.
 *
 * Additionally, if the current node is the outermost node in
 * the schedule tree (apart from the root domain node), we generate
 * all pending guards, again to be able to exploit the additional
 * context constraints.  We currently do not do this for internal
 * context nodes since we may still want to hoist conditions
 * to outer AST nodes.
 *
 * If the context node introduced any new parameters, then they
 * are removed from the set of enforced constraints and guard
 * in hoist_out_of_context.
 */
static __isl_give isl_ast_graft_list *build_ast_from_context(
	__isl_take isl_ast_build *build, __isl_take isl_schedule_node *node,
	__isl_take isl_union_map *executed)
{
	isl_set *context;
	isl_space *space;
	isl_multi_aff *internal2input;
	isl_ast_build *sub_build;
	isl_ast_graft_list *list;
	int n, depth;

	depth = isl_schedule_node_get_tree_depth(node);
	space = isl_ast_build_get_space(build, 1);
	context = isl_schedule_node_context_get_context(node);
	context = isl_set_align_params(context, space);
	sub_build = isl_ast_build_copy(build);
	space = isl_set_get_space(context);
	sub_build = isl_ast_build_align_params(sub_build, space);
	internal2input = isl_ast_build_get_internal2input(sub_build);
	context = isl_set_preimage_multi_aff(context, internal2input);
	sub_build = isl_ast_build_restrict_generated(sub_build,
					isl_set_copy(context));
	context = isl_set_from_basic_set(isl_set_simple_hull(context));
	executed = isl_union_map_intersect_domain(executed,
					isl_union_set_from_set(context));

	list = build_ast_from_child(isl_ast_build_copy(sub_build),
						node, executed);
	n = isl_ast_graft_list_n_ast_graft(list);
	if (n < 0)
		list = isl_ast_graft_list_free(list);

	list = isl_ast_graft_list_fuse(list, sub_build);
	if (depth == 1)
		list = isl_ast_graft_list_insert_pending_guard_nodes(list,
								sub_build);
	if (n >= 1)
		list = hoist_out_of_context(list, build, sub_build);

	isl_ast_build_free(build);
	isl_ast_build_free(sub_build);

	return list;
}

/* Generate an AST that visits the elements in the domain of "executed"
 * in the relative order specified by the expansion node "node" and
 * its descendants.
 *
 * The relation "executed" maps the outer generated loop iterators
 * to the domain elements executed by those iterations.
 *
 * We expand the domain elements by the expansion and
 * continue with the descendants of the node.
 */
static __isl_give isl_ast_graft_list *build_ast_from_expansion(
	__isl_take isl_ast_build *build, __isl_take isl_schedule_node *node,
	__isl_take isl_union_map *executed)
{
	isl_union_map *expansion;
	unsigned n1, n2;

	expansion = isl_schedule_node_expansion_get_expansion(node);
	expansion = isl_union_map_align_params(expansion,
				isl_union_map_get_space(executed));

	n1 = isl_union_map_dim(executed, isl_dim_param);
	executed = isl_union_map_apply_range(executed, expansion);
	n2 = isl_union_map_dim(executed, isl_dim_param);
	if (n2 > n1)
		isl_die(isl_ast_build_get_ctx(build), isl_error_invalid,
			"expansion node is not allowed to introduce "
			"new parameters", goto error);

	return build_ast_from_child(build, node, executed);
error:
	isl_ast_build_free(build);
	isl_schedule_node_free(node);
	isl_union_map_free(executed);
	return NULL;
}

/* Generate an AST that visits the elements in the domain of "executed"
 * in the relative order specified by the extension node "node" and
 * its descendants.
 *
 * The relation "executed" maps the outer generated loop iterators
 * to the domain elements executed by those iterations.
 *
 * Extend the inverse schedule with the extension applied to current
 * set of generated constraints.  Since the extension if formulated
 * in terms of the input schedule, it first needs to be transformed
 * to refer to the internal schedule.
 */
static __isl_give isl_ast_graft_list *build_ast_from_extension(
	__isl_take isl_ast_build *build, __isl_take isl_schedule_node *node,
	__isl_take isl_union_map *executed)
{
	isl_union_set *schedule_domain;
	isl_union_map *extension;
	isl_set *set;

	set = isl_ast_build_get_generated(build);
	set = isl_set_from_basic_set(isl_set_simple_hull(set));
	schedule_domain = isl_union_set_from_set(set);

	extension = isl_schedule_node_extension_get_extension(node);

	extension = isl_union_map_preimage_domain_multi_aff(extension,
			isl_multi_aff_copy(build->internal2input));
	extension = isl_union_map_intersect_domain(extension, schedule_domain);
	extension = isl_ast_build_substitute_values_union_map_domain(build,
								    extension);
	executed = isl_union_map_union(executed, extension);

	return build_ast_from_child(build, node, executed);
}

/* Generate an AST that visits the elements in the domain of "executed"
 * in the relative order specified by the filter node "node" and
 * its descendants.
 *
 * The relation "executed" maps the outer generated loop iterators
 * to the domain elements executed by those iterations.
 *
 * We simply intersect the iteration domain (i.e., the range of "executed")
 * with the filter and continue with the descendants of the node,
 * unless the resulting inverse schedule is empty, in which
 * case we return an empty list.
 */
static __isl_give isl_ast_graft_list *build_ast_from_filter(
	__isl_take isl_ast_build *build, __isl_take isl_schedule_node *node,
	__isl_take isl_union_map *executed)
{
	isl_ctx *ctx;
	isl_union_set *filter;
	isl_ast_graft_list *list;
	int empty;
	unsigned n1, n2;

	if (!build || !node || !executed)
		goto error;

	filter = isl_schedule_node_filter_get_filter(node);
	filter = isl_union_set_align_params(filter,
				isl_union_map_get_space(executed));
	n1 = isl_union_map_dim(executed, isl_dim_param);
	executed = isl_union_map_intersect_range(executed, filter);
	n2 = isl_union_map_dim(executed, isl_dim_param);
	if (n2 > n1)
		isl_die(isl_ast_build_get_ctx(build), isl_error_invalid,
			"filter node is not allowed to introduce "
			"new parameters", goto error);

	empty = isl_union_map_is_empty(executed);
	if (empty < 0)
		goto error;
	if (!empty)
		return build_ast_from_child(build, node, executed);

	ctx = isl_ast_build_get_ctx(build);
	list = isl_ast_graft_list_alloc(ctx, 0);
	isl_ast_build_free(build);
	isl_schedule_node_free(node);
	isl_union_map_free(executed);
	return list;
error:
	isl_ast_build_free(build);
	isl_schedule_node_free(node);
	isl_union_map_free(executed);
	return NULL;
}

/* Generate an AST that visits the elements in the domain of "executed"
 * in the relative order specified by the guard node "node" and
 * its descendants.
 *
 * The relation "executed" maps the outer generated loop iterators
 * to the domain elements executed by those iterations.
 *
 * Ensure that the associated guard is enforced by the outer AST
 * constructs by adding it to the guard of the graft.
 * Since we know that we will enforce the guard, we can also include it
 * in the generated constraints used to construct an AST for
 * the descendant nodes.
 */
static __isl_give isl_ast_graft_list *build_ast_from_guard(
	__isl_take isl_ast_build *build, __isl_take isl_schedule_node *node,
	__isl_take isl_union_map *executed)
{
	isl_space *space;
	isl_set *guard, *hoisted;
	isl_basic_set *enforced;
	isl_ast_build *sub_build;
	isl_ast_graft *graft;
	isl_ast_graft_list *list;
	unsigned n1, n2;

	space = isl_ast_build_get_space(build, 1);
	guard = isl_schedule_node_guard_get_guard(node);
	n1 = isl_space_dim(space, isl_dim_param);
	guard = isl_set_align_params(guard, space);
	n2 = isl_set_dim(guard, isl_dim_param);
	if (n2 > n1)
		isl_die(isl_ast_build_get_ctx(build), isl_error_invalid,
			"guard node is not allowed to introduce "
			"new parameters", guard = isl_set_free(guard));
	guard = isl_set_preimage_multi_aff(guard,
			isl_multi_aff_copy(build->internal2input));
	guard = isl_ast_build_specialize(build, guard);
	guard = isl_set_gist(guard, isl_set_copy(build->generated));

	sub_build = isl_ast_build_copy(build);
	sub_build = isl_ast_build_restrict_generated(sub_build,
							isl_set_copy(guard));

	list = build_ast_from_child(isl_ast_build_copy(sub_build),
							node, executed);

	hoisted = isl_ast_graft_list_extract_hoistable_guard(list, sub_build);
	if (isl_set_n_basic_set(hoisted) > 1)
		list = isl_ast_graft_list_gist_guards(list,
						    isl_set_copy(hoisted));
	guard = isl_set_intersect(guard, hoisted);
	enforced = extract_shared_enforced(list, build);
	graft = isl_ast_graft_alloc_from_children(list, guard, enforced,
						    build, sub_build);

	isl_ast_build_free(sub_build);
	isl_ast_build_free(build);
	return isl_ast_graft_list_from_ast_graft(graft);
}

/* Call the before_each_mark callback, if requested by the user.
 *
 * Return 0 on success and -1 on error.
 *
 * The caller is responsible for recording the current inverse schedule
 * in "build".
 */
static isl_stat before_each_mark(__isl_keep isl_id *mark,
	__isl_keep isl_ast_build *build)
{
	if (!build)
		return isl_stat_error;
	if (!build->before_each_mark)
		return isl_stat_ok;
	return build->before_each_mark(mark, build,
					build->before_each_mark_user);
}

/* Call the after_each_mark callback, if requested by the user.
 *
 * The caller is responsible for recording the current inverse schedule
 * in "build".
 */
static __isl_give isl_ast_graft *after_each_mark(
	__isl_take isl_ast_graft *graft, __isl_keep isl_ast_build *build)
{
	if (!graft || !build)
		return isl_ast_graft_free(graft);
	if (!build->after_each_mark)
		return graft;
	graft->node = build->after_each_mark(graft->node, build,
						build->after_each_mark_user);
	if (!graft->node)
		return isl_ast_graft_free(graft);
	return graft;
}


/* Generate an AST that visits the elements in the domain of "executed"
 * in the relative order specified by the mark node "node" and
 * its descendants.
 *
 * The relation "executed" maps the outer generated loop iterators
 * to the domain elements executed by those iterations.

 * Since we may be calling before_each_mark and after_each_mark
 * callbacks, we record the current inverse schedule in the build.
 *
 * We generate an AST for the child of the mark node, combine
 * the graft list into a single graft and then insert the mark
 * in the AST of that single graft.
 */
static __isl_give isl_ast_graft_list *build_ast_from_mark(
	__isl_take isl_ast_build *build, __isl_take isl_schedule_node *node,
	__isl_take isl_union_map *executed)
{
	isl_id *mark;
	isl_ast_graft *graft;
	isl_ast_graft_list *list;
	int n;

	build = isl_ast_build_set_executed(build, isl_union_map_copy(executed));

	mark = isl_schedule_node_mark_get_id(node);
	if (before_each_mark(mark, build) < 0)
		node = isl_schedule_node_free(node);

	list = build_ast_from_child(isl_ast_build_copy(build), node, executed);
	list = isl_ast_graft_list_fuse(list, build);
	n = isl_ast_graft_list_n_ast_graft(list);
	if (n < 0)
		list = isl_ast_graft_list_free(list);
	if (n == 0) {
		isl_id_free(mark);
	} else {
		graft = isl_ast_graft_list_get_ast_graft(list, 0);
		graft = isl_ast_graft_insert_mark(graft, mark);
		graft = after_each_mark(graft, build);
		list = isl_ast_graft_list_set_ast_graft(list, 0, graft);
	}
	isl_ast_build_free(build);

	return list;
}

static __isl_give isl_ast_graft_list *build_ast_from_schedule_node(
	__isl_take isl_ast_build *build, __isl_take isl_schedule_node *node,
	__isl_take isl_union_map *executed);

/* Generate an AST that visits the elements in the domain of "executed"
 * in the relative order specified by the sequence (or set) node "node" and
 * its descendants.
 *
 * The relation "executed" maps the outer generated loop iterators
 * to the domain elements executed by those iterations.
 *
 * We simply generate an AST for each of the children and concatenate
 * the results.
 */
static __isl_give isl_ast_graft_list *build_ast_from_sequence(
	__isl_take isl_ast_build *build, __isl_take isl_schedule_node *node,
	__isl_take isl_union_map *executed)
{
	int i, n;
	isl_ctx *ctx;
	isl_ast_graft_list *list;

	ctx = isl_ast_build_get_ctx(build);
	list = isl_ast_graft_list_alloc(ctx, 0);

	n = isl_schedule_node_n_children(node);
	for (i = 0; i < n; ++i) {
		isl_schedule_node *child;
		isl_ast_graft_list *list_i;

		child = isl_schedule_node_get_child(node, i);
		list_i = build_ast_from_schedule_node(isl_ast_build_copy(build),
					child, isl_union_map_copy(executed));
		list = isl_ast_graft_list_concat(list, list_i);
	}
	isl_ast_build_free(build);
	isl_schedule_node_free(node);
	isl_union_map_free(executed);

	return list;
}

/* Generate an AST that visits the elements in the domain of "executed"
 * in the relative order specified by the node "node" and its descendants.
 *
 * The relation "executed" maps the outer generated loop iterators
 * to the domain elements executed by those iterations.
 *
 * If the node is a leaf, then we pass control to generate_inner_level.
 * Note that the current build does not refer to any band node, so
 * that generate_inner_level will not try to visit the child of
 * the leaf node.
 *
 * The other node types are handled in separate functions.
 * Set nodes are currently treated in the same way as sequence nodes.
 * The children of a set node may be executed in any order,
 * including the order of the children.
 */
static __isl_give isl_ast_graft_list *build_ast_from_schedule_node(
	__isl_take isl_ast_build *build, __isl_take isl_schedule_node *node,
	__isl_take isl_union_map *executed)
{
	enum isl_schedule_node_type type;

	type = isl_schedule_node_get_type(node);

	switch (type) {
	case isl_schedule_node_error:
		goto error;
	case isl_schedule_node_leaf:
		isl_schedule_node_free(node);
		return generate_inner_level(executed, build);
	case isl_schedule_node_band:
		return build_ast_from_band(build, node, executed);
	case isl_schedule_node_context:
		return build_ast_from_context(build, node, executed);
	case isl_schedule_node_domain:
		isl_die(isl_schedule_node_get_ctx(node), isl_error_unsupported,
			"unexpected internal domain node", goto error);
	case isl_schedule_node_expansion:
		return build_ast_from_expansion(build, node, executed);
	case isl_schedule_node_extension:
		return build_ast_from_extension(build, node, executed);
	case isl_schedule_node_filter:
		return build_ast_from_filter(build, node, executed);
	case isl_schedule_node_guard:
		return build_ast_from_guard(build, node, executed);
	case isl_schedule_node_mark:
		return build_ast_from_mark(build, node, executed);
	case isl_schedule_node_sequence:
	case isl_schedule_node_set:
		return build_ast_from_sequence(build, node, executed);
	}

	isl_die(isl_ast_build_get_ctx(build), isl_error_internal,
		"unhandled type", goto error);
error:
	isl_union_map_free(executed);
	isl_schedule_node_free(node);
	isl_ast_build_free(build);

	return NULL;
}

/* Generate an AST that visits the elements in the domain of "executed"
 * in the relative order specified by the (single) child of "node" and
 * its descendants.
 *
 * The relation "executed" maps the outer generated loop iterators
 * to the domain elements executed by those iterations.
 *
 * This function is never called on a leaf, set or sequence node,
 * so the node always has exactly one child.
 */
static __isl_give isl_ast_graft_list *build_ast_from_child(
	__isl_take isl_ast_build *build, __isl_take isl_schedule_node *node,
	__isl_take isl_union_map *executed)
{
	node = isl_schedule_node_child(node, 0);
	return build_ast_from_schedule_node(build, node, executed);
}

/* Generate an AST that visits the elements in the domain of the domain
 * node "node" in the relative order specified by its descendants.
 *
 * An initial inverse schedule is created that maps a zero-dimensional
 * schedule space to the node domain.
 * The input "build" is assumed to have a parametric domain and
 * is replaced by the same zero-dimensional schedule space.
 *
 * We also add some of the parameter constraints in the build domain
 * to the executed relation.  Adding these constraints
 * allows for an earlier detection of conflicts in some cases.
 * However, we do not want to divide the executed relation into
 * more disjuncts than necessary.  We therefore approximate
 * the constraints on the parameters by a single disjunct set.
 */
static __isl_give isl_ast_node *build_ast_from_domain(
	__isl_take isl_ast_build *build, __isl_take isl_schedule_node *node)
{
	isl_ctx *ctx;
	isl_union_set *domain, *schedule_domain;
	isl_union_map *executed;
	isl_space *space;
	isl_set *set;
	isl_ast_graft_list *list;
	isl_ast_node *ast;
	int is_params;

	if (!build)
		goto error;

	ctx = isl_ast_build_get_ctx(build);
	space = isl_ast_build_get_space(build, 1);
	is_params = isl_space_is_params(space);
	isl_space_free(space);
	if (is_params < 0)
		goto error;
	if (!is_params)
		isl_die(ctx, isl_error_unsupported,
			"expecting parametric initial context", goto error);

	domain = isl_schedule_node_domain_get_domain(node);
	domain = isl_union_set_coalesce(domain);

	space = isl_union_set_get_space(domain);
	space = isl_space_set_from_params(space);
	build = isl_ast_build_product(build, space);

	set = isl_ast_build_get_domain(build);
	set = isl_set_from_basic_set(isl_set_simple_hull(set));
	schedule_domain = isl_union_set_from_set(set);

	executed = isl_union_map_from_domain_and_range(schedule_domain, domain);
	list = build_ast_from_child(isl_ast_build_copy(build), node, executed);
	ast = isl_ast_node_from_graft_list(list, build);
	isl_ast_build_free(build);

	return ast;
error:
	isl_schedule_node_free(node);
	isl_ast_build_free(build);
	return NULL;
}

/* Generate an AST that visits the elements in the domain of "schedule"
 * in the relative order specified by the schedule tree.
 *
 * "build" is an isl_ast_build that has been created using
 * isl_ast_build_alloc or isl_ast_build_from_context based
 * on a parametric set.
 *
 * The construction starts at the root node of the schedule,
 * which is assumed to be a domain node.
 */
__isl_give isl_ast_node *isl_ast_build_node_from_schedule(
	__isl_keep isl_ast_build *build, __isl_take isl_schedule *schedule)
{
	isl_ctx *ctx;
	isl_schedule_node *node;

	if (!build || !schedule)
		goto error;

	ctx = isl_ast_build_get_ctx(build);

	node = isl_schedule_get_root(schedule);
	isl_schedule_free(schedule);

	build = isl_ast_build_copy(build);
	build = isl_ast_build_set_single_valued(build, 0);
	if (isl_schedule_node_get_type(node) != isl_schedule_node_domain)
		isl_die(ctx, isl_error_unsupported,
			"expecting root domain node",
			build = isl_ast_build_free(build));
	return build_ast_from_domain(build, node);
error:
	isl_schedule_free(schedule);
	return NULL;
}
