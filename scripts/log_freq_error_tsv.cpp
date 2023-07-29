#include <cstdio>

#include "js80p.hpp"
#include "dsp/math.cpp"

using namespace JS80P;


Number* build_log_freq_lookup_table_without_correction()
{
    Number* table = new Number[Math::LOG_BIQUAD_FILTER_FREQ_TABLE_SIZE];

    table[0] = Constants::BIQUAD_FILTER_FREQUENCY_MIN;

    for (int i = 1; i != Math::LOG_BIQUAD_FILTER_FREQ_TABLE_MAX_INDEX; ++i) {
        Number const idx = (Number)i;

        table[i] = Math::ratio_to_exact_log_biquad_filter_frequency(
            idx * Math::LOG_BIQUAD_FILTER_FREQ_TABLE_MAX_INDEX_INV
        );
    }

    table[Math::LOG_BIQUAD_FILTER_FREQ_TABLE_MAX_INDEX] = (
        Constants::BIQUAD_FILTER_FREQUENCY_MAX
    );

    return table;
}


int main(int argc, char* argv[])
{
    constexpr int resolution = 1000000;
    constexpr Number scale = 1.0 / (Number)(resolution - 1);
    constexpr int max_index = Math::LOG_BIQUAD_FILTER_FREQ_TABLE_MAX_INDEX;

    fprintf(
        stdout,
        "ratio\terror-without-correction\terror-with-correction\n"
    );

    Number const* table_with_correction = Math::log_biquad_filter_freq_table();
    Number sum_error_without_correction = 0.0;
    Number sum_error_with_correction = 0.0;

    Number* table_without_correction = build_log_freq_lookup_table_without_correction();

    for (int i = 0; i != resolution; ++i) {
        Number const ratio = scale * (Number)i;
        Number const index = ratio * Math::LOG_BIQUAD_FILTER_FREQ_SCALE;
        Number const exact = Math::ratio_to_exact_log_biquad_filter_frequency(ratio);
        Number const error_without_correction = (
            Math::lookup(table_without_correction, max_index, index) - exact
        );
        Number const error_with_correction = (
            Math::lookup(table_with_correction, max_index, index) - exact
        );

        fprintf(
            stdout,
            "%.10f\t%.10f\t%.10f\n",
            ratio,
            error_without_correction,
            error_with_correction
        );

        sum_error_with_correction += error_with_correction;
        sum_error_without_correction += error_without_correction;
    }

    fprintf(
        stdout,
        "sum:\t%.10f\t%.10f\n",
        sum_error_without_correction,
        sum_error_with_correction
    );

    delete[] table_without_correction;

    return 0;
}
