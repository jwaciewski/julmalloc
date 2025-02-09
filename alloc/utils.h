#ifndef ALLOC_UTILS_H
#define ALLOC_UTILS_H
#include <sys/types.h>

/**
 * @brief Helper function for the ASSERT makro
 *
 * This function prints some error information in case the ASSERT macro fails.
 * Then, it calls abort()
 *
 * @param[in] file The file in which the assertion failed
 * @param[in] line The line in which the assertion failed
 * @param[in] expr The expression, "casted" to string, for printin
 */
void a_abort(char *file, int line, char *expr);

/**
 * @brief Rounds up number to nearest multiple
 *
 * This function takes an input number and rounds it up to the next multiple of
 * @p multiple (if @p multiple is larger than zero).
 *
 * @param[in] num_to_round The number to be rounded
 * @param[in] multiple The multiple to round by
 *
 * @return min_x(x*multiple>num_to_round)*multiple, that is the smallest number
 * which is larger than @p num_to_round being a multiple of @p multiple
 *
 * */
size_t round_up(size_t num_to_round, size_t multiple);

/**
 * @brief Rounds down number to nearest multiple
 *
 * This function takes an input number and rounds it down to the next multiple
 * of
 * @p multiple (if @p multiple is larger than zero).
 *
 * @param[in] num_to_round The number to be rounded
 * @param[in] multiple The multiple to round by
 *
 * @return max_x(x*multiple<num_to_round)*multiple, that is the larger number
 * which is smaller than @p num_to_round being a multiple of @p multiple
 *
 * */
size_t round_down(size_t num_to_round, size_t multiple);

#endif