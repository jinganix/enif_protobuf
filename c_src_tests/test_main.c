#include <stdio.h>
#include "test.h"

int run_test_ep_cache(void);
int run_test_ep_node(void);
int run_test_ep_encoder(void);
int run_test_ep_decoder(void);
int run_test_enif_protobuf(void);
int run_test_integration(void);
int run_test_wire_codec(void);
int run_test_codec_types(void);
int run_test_codec_features(void);
int run_test_codec_breadth(void);
int run_test_coverage_gaps(void);
int run_test_coverage_exhaustive(void);

int
main(void)
{
    int failed = 0;

    printf("test_ep_cache\n");
    if (run_test_ep_cache() != 0) {
        failed = 1;
    }

    printf("test_ep_node\n");
    if (run_test_ep_node() != 0) {
        failed = 1;
    }

    printf("test_ep_encoder\n");
    if (run_test_ep_encoder() != 0) {
        failed = 1;
    }

    printf("test_ep_decoder\n");
    if (run_test_ep_decoder() != 0) {
        failed = 1;
    }

    printf("test_enif_protobuf\n");
    if (run_test_enif_protobuf() != 0) {
        failed = 1;
    }

    printf("test_integration\n");
    if (run_test_integration() != 0) {
        failed = 1;
    }

    printf("test_wire_codec\n");
    if (run_test_wire_codec() != 0) {
        failed = 1;
    }

    printf("test_codec_types\n");
    if (run_test_codec_types() != 0) {
        failed = 1;
    }

    printf("test_codec_features\n");
    if (run_test_codec_features() != 0) {
        failed = 1;
    }

    printf("test_codec_breadth\n");
    if (run_test_codec_breadth() != 0) {
        failed = 1;
    }

    printf("test_coverage_gaps\n");
    if (run_test_coverage_gaps() != 0) {
        failed = 1;
    }

    printf("test_coverage_exhaustive\n");
    if (run_test_coverage_exhaustive() != 0) {
        failed = 1;
    }

    if (failed) {
        fprintf(stderr, "C unit tests failed\n");
        return 1;
    }

    printf("All C unit tests passed\n");
    return 0;
}
