#ifndef BEAM_RULES_H
#define BEAM_RULES_H

#define NUM_BEAM_RULES 32

static struct rule_str {
#define FUNC_BEGIN 1
#define FUNC_END 2
#define END_OF_TABLE 3
	int function;
	int notelen;
	int time_num, time_denom;
	int duration;
} beam_rules_tab__[NUM_BEAM_RULES] = {
	{ FUNC_END, -1, 3, 2, 322560},
	{ FUNC_END, 40320, 3, 2, 161280},
	{ FUNC_END, 20160, 3, 2, 80640},
	{ FUNC_BEGIN, 80640, 3, 4, 161280},
	{ FUNC_END, -1, 3, 4, 483840},
	{ FUNC_BEGIN, 40320, 3, 4, 40320},
	{ FUNC_END, 40320, 3, 4, 161280},
	{ FUNC_END, 20160, 3, 4, 80640},
	{ FUNC_BEGIN, 40320, 3, 8, 80640},
	{ FUNC_END, -1, 3, 8, 241920},
	{ FUNC_END, -1, 4, 4, 322560},
	{ FUNC_END, 53760, 4, 4, 161280},
	{ FUNC_END, 40320, 4, 4, 161280},
	{ FUNC_END, 20160, 4, 4, 80640},
	{ FUNC_END, -1, 2, 4, 161280},
	{ FUNC_END, 53760, 2, 4, 161280},
	{ FUNC_END, 40320, 2, 4, 161280},
	{ FUNC_END, 20160, 2, 4, 80640},
	{ FUNC_END, -1, 4, 8, 161280},
	{ FUNC_END, 40320, 4, 8, 161280},
	{ FUNC_END, 20160, 4, 8, 80640},
	{ FUNC_END, -1, 4, 16, 80640},
	{ FUNC_END, -1, 6, 8, 241920},
	{ FUNC_END, 40320, 6, 8, 241920},
	{ FUNC_END, 20160, 6, 8, 80640},
	{ FUNC_END, -1, 9, 8, 241920},
	{ FUNC_END, 40320, 9, 8, 241920},
	{ FUNC_END, 20160, 9, 8, 80640},
	{ FUNC_END, -1, 12, 8, 241920},
	{ FUNC_END, 40320, 12, 8, 241920},
	{ FUNC_END, 20160, 12, 8, 80640},
	{ END_OF_TABLE, -1, -1, -1, -1}
};

#endif /* BEAM_RULES_H */
