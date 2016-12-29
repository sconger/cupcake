
#ifndef CUPCAKE_HPACK_TEST_H
#define CUPCAKE_HPACK_TEST_H

bool test_hpack_indexed_header();
bool test_hpack_indexed_header_invalid();
bool test_hpack_incremental_indexing();
bool test_hpack_incremental_indexing_invalid();
bool test_hpack_without_indexing();
bool test_hpack_without_indexing_invalid();
bool test_hpack_table_size_change();

#endif // CUPCAKE_HPACK_TEST_H
