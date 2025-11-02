#include "gastask.h"

#define N_REPORTS	1000

static int	n_report_intervals;

static FILE	*fp;

void
add_report(unsigned gen)
{
	double	util_sum = 0, cost_sum = 0, fit_sum = 0, penalty_sum = 0;
	double	util_avg, cost_avg = -1, fit_avg = 0, penalty_avg = 0;
	double	util_min, util_max, cost_min = -1, cost_max;
	double fit_min = 1e18, fit_max = -1e18;
    double penalty_min = 1e18, penalty_max = -1e18;
	unsigned	n_valid_genes = 0;
	gene_t	*gene;
	struct list_head	*lp;
	int	i;

	if (fp == NULL)
		return;

#ifdef N_REPORTS
	if (n_report_intervals == 0) {
		n_report_intervals = max_gen / N_REPORTS;
		if (n_report_intervals == 0)
			n_report_intervals = 1;
	}
	else if (gen % n_report_intervals != 0 && gen != max_gen)
		return;
#endif
	for (i = 0; i < n_pops; i++) {
		gene = genes + i;
		util_sum += gene->util;
		if (gene->util <= 3.0) {
			cost_sum += gene->score;
			fit_sum += gene->fitness;
            penalty_sum += gene->constraint_penalty;

			if (gene->fitness < fit_min) fit_min = gene->fitness;
            if (gene->fitness > fit_max) fit_max = gene->fitness;

            if (gene->constraint_penalty < penalty_min) penalty_min = gene->constraint_penalty;
            if (gene->constraint_penalty > penalty_max) penalty_max = gene->constraint_penalty;

			n_valid_genes++;
		}
	}

	util_avg = util_sum / n_pops;
	if (n_valid_genes > 0){
		cost_avg = cost_sum / n_valid_genes;
		fit_avg = fit_sum / n_valid_genes;
        penalty_avg = penalty_sum / n_valid_genes;
	}

	gene = list_entry(genes_by_util.next, gene_t, list_util);
	util_min = gene->util;
	gene = list_entry(genes_by_util.prev, gene_t, list_util);
	util_max = gene->util;

	list_for_each (lp, &genes_by_score) {
		gene = list_entry(genes_by_score.next, gene_t, list_score);
		if (gene->util <= 3.0) {
			cost_min = gene->score;
			break;
		}
	}
	gene = list_entry(genes_by_score.prev, gene_t, list_score);
	cost_max = gene->score;
	if (cost_avg < 0)
		cost_avg = cost_max;
	if (cost_min < 0)
		cost_min = cost_max;
	fprintf(fp, "%u %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf\n", gen, 
		fit_min, fit_avg, fit_max,
        penalty_min, penalty_avg, penalty_max,
		cost_min, cost_avg, cost_max, 
		util_min, util_avg, util_max);
}

static void
save_task_infos(void)
{
	gene_t	*gene;
	int	i, n_offloading = 0, cpufreq0 = 0, cpufreq1 = 0, cpufreq2 = 0, cpufreq3 = 0, cpufreq4 = 0; 

	fp = fopen("task.txt", "w");
	if (fp == NULL){
		FATAL(2, "cannot open task.txt");
	}
	
	fprintf(fp, "# mem_idx cpufreq_idx cloud_idx offloadingratio_idx\n"); 
	gene = list_entry(genes_by_score.next, gene_t, list_score);
	if (gene->util > 2.0) {
		FATAL(2, "over-utilized gene: %lf", gene->util);
	}
	for (i = 0; i < n_tasks; i++) {
		fprintf(fp, "%u %u %u %u\n", (unsigned)gene->taskattrs_mem.attrs[i], (unsigned)gene->taskattrs_cpufreq.attrs[i],
		(unsigned)gene->taskattrs_cloud.attrs[i], (unsigned)gene->taskattrs_offloadingratio.attrs[i]); 
		if((unsigned)gene->taskattrs_offloadingratio.attrs[i] != 0){
			n_offloading++;
		}else{
			if((unsigned)gene->taskattrs_cpufreq.attrs[i] == 0) 
				cpufreq0++;
			else if ((unsigned)gene->taskattrs_cpufreq.attrs[i] == 1)
				cpufreq1++;
			else if ((unsigned)gene->taskattrs_cpufreq.attrs[i] == 2)
				cpufreq2++;
			else if ((unsigned)gene->taskattrs_cpufreq.attrs[i] == 3)
                cpufreq3++;
			else
				cpufreq4++;
		}	
	}
	fclose(fp);

	// Uncomment for Private mode; comment out for Mutual, Hybrid, or Public modes.
	// gene->util = gene->util / 2.0;

	printf("power: %.6lf util: %.6lf\n", gene->power, gene->util);
	printf("cpu power: %.6lf memory power: %.6lf network power: %.6lf\n", gene->cpu_power, gene->mem_power, gene->power_netcom); 
	printf("offloading ratio: %.6lf\n", n_offloading/(double)n_tasks); 
	printf("cpu frequency: \n1\t0.564\t0.327\t0.25\t0.182 \n"); 
	printf("%d\t%d\t%d\t%d\t%d \n", cpufreq0, cpufreq1, cpufreq2, cpufreq3, cpufreq4); 
	printf("period violation: %u\n", gene->period_violation);
	printf("score (cost): %.6lf\n", gene->total_cost);
	printf("cpu_memory elec cost: %.6lf\n", gene->cost_cpu_memory);
	printf("network elec cost: %.6lf\n", gene->cost_network);
	printf("base elec cost: %.6lf\n", gene->cost_base);
	printf("hw base cost: %.6lf\n", gene->cost_hw);
	printf("mec rental cost: %.6lf\n", gene->cost_rent);		
}

void
init_report(void)
{
	fp = fopen("report.txt", "w");
	if (fp == NULL) {
		FATAL(2, "cannot open report.txt");
	}
	fprintf(fp, "# generation fit_min fit_avg fit_max pen_min pen_avg pen_max cost_score_min cost_score_avg cost_score_max util_min util_avg util_max\n");
}

void
close_report(void)
{
	if (fp != NULL)
		fclose(fp);
	save_task_infos();
}

