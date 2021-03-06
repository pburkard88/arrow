/**********************************************************doxygen*//** @file
 * @brief   Constrained bottleneck spanning tree solver.
 *
 * Solves the constrained bottleneck spanning tree (CBST) problem the given 
 * input file.
 *
 * @author  John LaRusic
 * @ingroup bin
 ****************************************************************************/
#include "common.h"
#include "lb.h"

/* Global variables */
char *program_name;             /**< Program name */
char *input_file = NULL;        /**< Given input TSPLIB file */
char *xml_file = NULL;
double max_length = -1.0;

/* Program options */
#define NUM_OPTS 3
arrow_option options[NUM_OPTS] = 
{
    {'i', "input", "TSPLIB input file", 
        ARROW_OPTION_STRING, &input_file, ARROW_TRUE, ARROW_TRUE},
    {'x', "xml", "File to write XML output to",
        ARROW_OPTION_STRING, &xml_file, ARROW_FALSE, ARROW_TRUE},
    {'L', "length", "Maximum length of spanning tree",
        ARROW_OPTION_DOUBLE, &max_length, ARROW_FALSE, ARROW_TRUE}
};
char *desc = "Constrained bottleneck spanning tree solver";
char *usage = "-i tsplib.tsp [options] ";

/* Main method */
int 
main(int argc, char *argv[])
{   
    int ret = ARROW_SUCCESS;
    
    arrow_problem problem;
    arrow_problem_info info;
    arrow_bound_result result;
    
    /* Read program arguments */
    if(!arrow_options_parse(NUM_OPTS, options, desc, usage, argc, argv, NULL))
        return EXIT_FAILURE;
    if(max_length < 0.0)
    {
        arrow_print_error("No length or negative max. length given.");
        return EXIT_FAILURE;
    }

    /* Try and read the problem file and its info */
    if(!arrow_problem_read(input_file, &problem))
        return EXIT_FAILURE;
    if(!arrow_problem_info_get(&problem, ARROW_FALSE, &info))
        return EXIT_FAILURE;
    /* Note: Algorithm does not require hash table */
    
    /* Solve CBST */
    if(!arrow_cbst_solve(&problem, &info, max_length, &result))
    {
        arrow_print_error("Could not solve CBST on file.");
        return EXIT_FAILURE;
    }
    
    printf("\nCBST Solution: %d\n", result.obj_value);
    printf("Total Time: %5.2f\n", result.total_time);
    
    /* Print out XML file if required */
    if(xml_file != NULL)
    {
        FILE *xml;
        if(!(xml = fopen(xml_file, "w")))
        {
            arrow_print_error("Could not open xml file for writing");
            ret = ARROW_FAILURE;
            goto CLEANUP;
        }
        
        fprintf(xml, "<arrow_bound type=\"CBST\" ");
        fprintf(xml, "problem_file=\"%s\"", input_file);
        fprintf(xml, " command_args=\"");
        arrow_util_print_program_args(argc, argv, xml);
        fprintf(xml, "\">\n");
        fprintf(xml, "    <objective_value>%d</objective_value>\n", 
                result.obj_value);
        fprintf(xml, "    <total_time>%5.2f</total_time>\n", 
                result.total_time);
        fprintf(xml, "</arrow_bound>\n");
        
        fclose(xml);
    }
        
CLEANUP:
    arrow_problem_destruct(&problem);
    arrow_problem_info_destruct(&info);
    return ret;
}
