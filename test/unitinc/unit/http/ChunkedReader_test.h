
#ifndef CUPCAKE_CHUNKED_READER_TEST_H
#define CUPCAKE_CHUNKED_READER_TEST_H

bool test_chunkedreader_basic();
bool test_chunkedreader_empty();
bool test_chunkedreader_bad_data_line();
bool test_chunkedreader_extension();
bool test_chunkedreader_trailing_headers();

#endif // CUPCAKE_CHUNKED_READER_TEST_H
