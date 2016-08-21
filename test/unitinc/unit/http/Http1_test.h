
#ifndef CUPCAKE_HTTP1_TEST_H
#define CUPCAKE_HTTP1_TEST_H

bool test_http1_empty();
bool test_http1_contentlen_request();
bool test_http1_contentlen_response();
bool test_http1_chunked_request();
bool test_http1_chunked_response();
bool test_http1_keepalive();

#endif // CUPCAKE_HTTP1_TEST_H
