#include "fraction.h"
#include <gtest/gtest.h>

#include <big_int.h>
#include <client_logger.h>
#include <client_logger_builder.h>
#include <operation_not_supported.h>

logger *create_logger(
    std::vector<std::pair<std::string, logger::severity>> const
        &output_file_streams_setup,
    bool use_console_stream = true,
    logger::severity console_stream_severity = logger::severity::debug) {
    logger_builder *builder = new client_logger_builder();

    if (use_console_stream) {
        builder->add_console_stream(console_stream_severity);
    }

    for (auto &output_file_stream_setup : output_file_streams_setup) {
        builder->add_file_stream(output_file_stream_setup.first,
                                 output_file_stream_setup.second);
    }

    logger *built_logger = builder->build();

    delete builder;

    return built_logger;
}

TEST(fraction, t1) {
    fraction f(1, 2);
    fraction root = f.root(2, fraction(1, 100));
    fraction r = root * root;
    ASSERT_EQ(f - r <  fraction(1, 100), true);
}

TEST(fraction, t2) {
    fraction f = fraction(2, 3) * fraction(9, 8);
    ASSERT_EQ(f, fraction(3, 4));
    f -= fraction(1, 2);
    ASSERT_EQ(f, fraction(1, 4));
    f /= fraction(3, 24);
    ASSERT_EQ(f, fraction(2, 1));
    f += -fraction(7, 5);
    ASSERT_EQ(f, fraction(3, 5));
}

