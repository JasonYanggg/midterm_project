#ifndef CONFIG_H_
#define CONFIG_H_

// The number of labels (without negative)
#define label_num 2

struct Config {

  // This must be the same as seq_length in the src/model_train/config.py
  const int seq_length = 64;

  // The number of expected consecutive inferences for each gesture type.
  const int consecutiveInferenceThresholds[label_num] = {15, 15};

  const char* output_message[label_num] = {
        "TRIANGLE\n\r"
        "         *        \n\r"
        "       *   *      \n\r"
        "     *       *    \n\r"
        "   *           *  \n\r"
        " * * * * * * * * *\n\r",
        "LOVE\n\r"
        "     * *   * *     \n\r"
        "   *     *     *  \n\r"
        "  *             * \n\r"
        "    *         *   \n\r"
        "      *     *     \n\r"
        "         *        \n\r"};
        // "RING:\n\r"
        // "          *       \n\r"
        // "       *     *    \n\r"
        // "     *         *  \n\r"
        // "    *           * \n\r"
        // "     *         *  \n\r"
        // "       *     *    \n\r"
        // "          *       \n\r",
        // "SLOPE:\n\r"
        // "        *        \n\r"
        // "       *         \n\r"
        // "      *          \n\r"
        // "     *           \n\r"
        // "    *            \n\r"
        // "   *             \n\r"
        // "  *              \n\r"
        // " * * * * * * * * \n\r"};
};

Config config;
#endif // CONFIG_H_