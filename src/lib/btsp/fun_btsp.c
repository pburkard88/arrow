/**********************************************************doxygen*//** @file
 * @brief   Cost matrix functions for the symmetric BTSP.
 *
 * Cost matrix transformation functions for the symmetric bottleneck traveling
 * salesman problem (SBTSP).
 *
 * @author  John LaRusic
 * @ingroup lib
 ****************************************************************************/
#include "common.h"
#include "btsp.h"

/****************************************************************************
 * Private structures
 ****************************************************************************/
/**
 *  @brief  Info for shake type I cost matrix function.
 */
typedef struct btsp_shake_1_data
{
    int infinity;               /**< value to use for "infinity" */
    arrow_problem_info *info;   /**< problem info */
    int random_min;             /**< minimum random number generated */
    int random_max;             /**< maximum random number generated */
    int *random_list;           /**< random list of integers */
    int random_list_length;     /**< random list length */
} btsp_shake_1_data;


/****************************************************************************
 * Private function prototypes
 ****************************************************************************/
/**
 *  @brief  Retrieves cost between nodes i and j from the function.
 *  @param  fun [in] function structure
 *  @param  base_problem [in] problem structure
 *  @param  delta [in] delta parameter
 *  @param  i [in] id of start node
 *  @param  j [in] id of end node
 *  @return cost between node i and node j
 */
int
btsp_basic_get_cost(arrow_btsp_fun *fun, arrow_problem *base_problem,
                     int delta, int i, int j);
               
/**
 *  @brief  Initializes the function data.
 *  @param  fun [out] the function structure to initialize
 */
int
btsp_basic_initialize(arrow_btsp_fun *fun);
              
/**
 *  @brief  Destructs the function data.
 *  @param  fun [out] the function structure to destruct
 */
void
btsp_basic_destruct(arrow_btsp_fun *fun);

/**
 *  @brief  Determines if the given tour is feasible or not.
 *  @param  fun [in] function structure
 *  @param  problem [in] the problem to check against
 *  @param  delta [in] delta parameter
 *  @param  tour_length [in] the length of the given tour
 *  @param  tour [in] the tour in node-node format
 *  @return ARROW_TRUE if the tour is feasible, ARROW_FALSE if not
 */
int
btsp_basic_feasible(arrow_btsp_fun *fun, arrow_problem *problem,
                     int delta, double tour_length, int *tour);
                     
/**
 *  @brief  Retrieves cost between nodes i and j from the function.
 *  @param  fun [in] function structure
 *  @param  base_problem [in] problem structure
 *  @param  delta [in] delta parameter
 *  @param  i [in] id of start node
 *  @param  j [in] id of end node
 *  @return cost between node i and node j
 */
int
btsp_shake_1_get_cost(arrow_btsp_fun *fun, arrow_problem *base_problem,
                     int delta, int i, int j);

/**
 *  @brief  Initializes the function data.
 *  @param  fun [out] the function structure to initialize
 */
int
btsp_shake_1_initialize(arrow_btsp_fun *fun);
              
/**
 *  @brief  Destructs the function data.
 *  @param  fun [out] the function structure to destruct
 */
void
btsp_shake_1_destruct(arrow_btsp_fun *fun);


/****************************************************************************
 * Public function implementations
 ****************************************************************************/
int
arrow_btsp_fun_basic(int shallow, arrow_btsp_fun *fun)
{    
    fun->data = NULL;
    fun->shallow = shallow;
    fun->get_cost = btsp_basic_get_cost;
    fun->initialize = btsp_basic_initialize;
    fun->destruct = btsp_basic_destruct;
    fun->feasible = btsp_basic_feasible;
    return ARROW_SUCCESS;
}

int
arrow_btsp_fun_shake_1(int shallow, int infinity, 
                       int random_min, int random_max,
                       arrow_problem_info *info, arrow_btsp_fun *fun)
{
    fun->data = NULL;
    if((fun->data = malloc(sizeof(btsp_shake_1_data))) == NULL)
    {
        arrow_print_error("Could not allocate memory for btsp_shake_i_data");
        return ARROW_FAILURE;
    }
    
    btsp_shake_1_data *shake_data = (btsp_shake_1_data *)fun->data;
    shake_data->infinity = infinity;
    shake_data->info = info;
    
    shake_data->random_min = random_min;
    shake_data->random_max = random_max;
    shake_data->random_list_length = info->cost_list_length;
    
    if(!arrow_util_create_int_array(shake_data->random_list_length,
                                    &(shake_data->random_list)))
    {
        arrow_print_error("Could not allocate memory for random list");
        return ARROW_FAILURE;
    }
    
    fun->shallow = shallow;
    fun->get_cost = btsp_shake_1_get_cost;
    fun->initialize = btsp_shake_1_initialize;
    fun->destruct = btsp_shake_1_destruct;
    fun->feasible = btsp_basic_feasible;
    
    return ARROW_SUCCESS;
}

/****************************************************************************
 * Private function implementations
 ****************************************************************************/
int
btsp_basic_get_cost(arrow_btsp_fun *fun, arrow_problem *base_problem,
                     int delta, int i, int j)
{
    int cost = base_problem->get_cost(base_problem, i, j);
    
    if(cost < 0)
        return cost;
    else if(cost <= delta)
        return 0;
    else
        return cost;
}

int
btsp_basic_initialize(arrow_btsp_fun *fun)
{ 
    return ARROW_SUCCESS;
}

void
btsp_basic_destruct(arrow_btsp_fun *fun)
{ }

int
btsp_basic_feasible(arrow_btsp_fun *fun, arrow_problem *problem, 
                     int delta, double tour_length, int *tour)
{
    if(problem->fixed_edges > 0)
    {
        int i, u, v, cost;
        int fixed_edge_count = 0;
    
        for(i = 0; i < problem->size; i++)
        {
            u = tour[i];
            v = tour[(i + 1) % problem->size];
            cost = problem->get_cost(problem, u, v);
        
            if(cost > delta)
                return ARROW_FALSE;
           
            if(cost < 0)
                fixed_edge_count++;
        }

        if(fixed_edge_count < problem->fixed_edges)
            return ARROW_FALSE;
    }
    
    return (tour_length <= 0 ? ARROW_TRUE : ARROW_FALSE);
}

int
btsp_shake_1_get_cost(arrow_btsp_fun *fun, arrow_problem *base_problem,
                       int delta, int i, int j)
{
    btsp_shake_1_data *data = (btsp_shake_1_data *)fun->data;
    int cost = base_problem->get_cost(base_problem, i, j);
    
    if(cost < 0)
        return cost;
    else if(cost <= delta)
        return 0;
    else
    {
        int pos = -1;
        if(!arrow_problem_info_cost_index(data->info, cost, &pos))
        {
            arrow_print_error("Could not find cost in ordered cost list!");
            return data->infinity;
        }
        return cost + data->random_list[pos];
    }        
}

int
btsp_shake_1_initialize(arrow_btsp_fun *fun)
{ 
    int val;
    btsp_shake_1_data *data = (btsp_shake_1_data *)fun->data;
    
    /* Need to generate a new list of random numbers */
    arrow_bintree tree;
    arrow_bintree_init(&tree);
    while(tree.size < data->random_list_length)
    {
        val = arrow_util_random_between(data->random_min, data->random_max);
        arrow_bintree_insert(&tree, val);
    }
    arrow_bintree_to_array(&tree, data->random_list);
    
    return ARROW_SUCCESS;
}

void
btsp_shake_1_destruct(arrow_btsp_fun *fun)
{
    if(fun->data != NULL)
    {
        btsp_shake_1_data *data = (btsp_shake_1_data *)fun->data;
        free(data->random_list);
        
        free(fun->data);
        fun->data = NULL;
    }
}
