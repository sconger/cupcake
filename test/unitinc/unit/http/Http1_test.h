
#ifndef CUPCAKE_HTTP1_TEST_H
#define CUPCAKE_HTTP1_TEST_H

bool test_http1_empty();
bool test_http1_contentlen_request();
bool test_http1_auto_contentlen_response();
bool test_http1_request_with_transfer_encoding();
bool test_http1_response_with_transfer_encoding();
bool test_http1_keepalive();

#endif // CUPCAKE_HTTP1_TEST_H
