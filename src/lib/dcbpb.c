/**********************************************************doxygen*//** @file
 *  @brief   Degree constarined bottleneck paths bound
 *
 *  Implemenation of the degree constrained bottleneck paths bound (DCBPB) 
 *  used as a lower bound for the Bottleneck TSP objective value.
 *
 *  @author  John LaRusic
 *  @ingroup lib
 ****************************************************************************/
#include "arrow.h"

/**
 *  @brief  Solves the all-pairs bottleneck paths problem (a simple
 *          modification of the Floyd-Warshall alg for all-pairs shortest
 *          paths).
 *  @param  problem [in] problem data
 *  @param  ignore [in] vertex number to ignore
 *  @param  b [out] array will hold bottleneck path value for each
 *              pair of source/sink nodes
 */
void
bottleneck_paths(arrow_problem *problem, int ignore, int **b);

/*
 *  @brief  Returns the max of the three values
 *  @param  i [in] first number
 *  @param  j [in] second number
 *  @param  k [in] third number
 *  @return the largest of i, j, k
 */
int
max(int i, int j, int k);

/****************************************************************************
 * Public function implemenations
 ****************************************************************************/
int
arrow_dcbpb_solve(arrow_problem *problem, arrow_bound_result *result)
{
    int ret = ARROW_SUCCESS;
    int i, j, k;
    int delta, bottleneck, max_tree, min_node;
    int in_cost, out_cost;
    int n = problem->size;
    double start_time, end_time;    
    
    start_time = arrow_util_zeit();
    
    int **b;
    int *b_space;
    if(!arrow_util_create_int_matrix(n, n, &b, &b_space))
    {
        ret = ARROW_FAILURE;
        goto CLEANUP;
    }
    bottleneck = INT_MIN;

    for(i = 0; i < n; i++)
    {
        bottleneck_paths(problem, i, b);
        min_node = INT_MAX;
        
        for(j = 0; j < n; j++)
        {
            if(j != i)
            {
                delta = INT_MIN;
                for(k = 0; k < n; k++)
                {
                    if((k != j) && (k != i))
                    {
                        if(b[j][k] > delta) delta = b[j][k];
                    }
                }
                
                for(k = 0; k < n; k++)
                {
                    if((k != i) && (k != j))
                    {
                        out_cost = problem->get_cost(problem, i, j);
                        in_cost = problem->get_cost(problem, k, i);
                        max_tree = max(delta, out_cost, in_cost);
                        if(max_tree < min_node) min_node = max_tree;
                    }
                }
            }
        }
        
        if(bottleneck < min_node)
            bottleneck = min_node;
    }
    end_time = arrow_util_zeit();

    result->obj_value = bottleneck;
    result->total_time = end_time - start_time;
    
CLEANUP:
    if(b_space != NULL) free(b_space);
    if(b != NULL) free(b);
    return ret;
}

/****************************************************************************
 * Private function implemenations
 ****************************************************************************/
void
bottleneck_paths(arrow_problem *problem, int ignore, int **b)
{
    int i, j, k, max;
    
    /* Initialize! */
    for(i = 0; i < problem->size; i++)
    {
        for(j = 0; j < problem->size; j++)
            b[i][j] = problem->get_cost(problem, i, j);
        b[i][i] = INT_MAX;
    }
    
    /* Dynamic programming technique GO! */
    for(k = 0; k < problem->size; k++)
    {
        if(k != ignore)
        {
            for(i = 0; i < problem->size; i++)
            {
                if((i != ignore) && (k != i))
                {
                    for(j = 0; j < problem->size; j++)
                    {
                        if((j != ignore) && (k != j) && (i != j))
                        {
                            max = (b[i][k] > b[k][j] ? b[i][k] : b[k][j]);
                            if(max < b[i][j])
                                b[i][j] = max;
                        }
                    }
                }
            }
        }
    }
}

int
max(int i, int j, int k)
{
    int max = i;
    if(max < j) max = j;
    if(max < k) max = k;
    return max;
}